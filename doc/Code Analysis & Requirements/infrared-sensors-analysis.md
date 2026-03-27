# Infrared Sensor Systems Analysis

## Overview

The air monitoring system includes infrared (IR) sensors for gas detection including CO2, C1 (CH4), and PID (Photo-Ionization Detector) sensors. These sensors use analog voltage outputs that are measured by MCP342x ADCs and processed through linear and exponential calibration functions with peak detection algorithms.

## Hardware Architecture

### I2C Interface Configuration

**I2C Addresses:**
- 0x6C: MCP342x ADC for IR sensors (IC108/IC116)
- 0x3A: PCF8574 I/O expander for IR power control (IC112/IC122)

**ADC Channel Mapping:**
- Channel 1: C1/CH4 sensor (U3)
- Channel 2: NH3 CAN sensor (U5) - when using SGX version
- Channel 3: PID sensor (U4)
- Channel 4: CO2 sensor (U2)

### Power Management

The IR sensors require a 90-second boot delay after power-on:
```cpp
irBootDelay = millis() + 90*1000; // 90sec boot delay
```

This delay ensures sensor stabilization before data collection begins.

## ADC Reading Process

### MCP342x Configuration

**ADC Settings:**
- Resolution: 18-bit (131071 counts = 2.048V)
- Gain: 1x
- Mode: One-shot conversion
- Reference: External 2.048V

### Voltage Conversion

```cpp
ADvolt = (ADcount * 2.048) / 131071;
```

### Data Collection Process

The `CollectIR()` function performs the following steps:

1. **Boot Delay Check**: Ensures sensors have completed initialization
2. **Channel Reading**: Sequentially reads each enabled sensor channel
3. **Data Accumulation**: Maintains running sums and sample counts
4. **Peak Tracking**: Updates maximum voltage values
5. **Error Handling**: Sets `_ir_err` flag on ADC communication failures

## Sensor-Specific Processing

### C1/CH4 Sensor (Channel 1)

**Data Variables:**
```cpp
float _c1_volt;        // Average voltage
float _c1_volt_max;    // Maximum voltage in sampling period
float _c1_sum;         // Accumulated voltage sum
unsigned short _c1_smp_count; // Sample count
```

**Peak Detection Variables:**
```cpp
float _c1_signal, _c1_base, _c1_top, _c1_mem;
float _c1_tcomp, _c1_dx, _c1_dx_trig, _c1_corr;
bool _c1_found;
```

### CO2 Sensor (Channel 4)

**Data Variables:**
```cpp
float _co2_volt;       // Average voltage
float _co2_volt_max;   // Maximum voltage
float _co2_sum;        // Accumulated sum
unsigned short _co2_smp_count; // Sample count
```

**Peak Detection Variables:**
```cpp
float _co2_signal, _co2_base, _co2_top, _co2_mem;
float _co2_tcomp, _co2_dx, _co2_dx_trig, _co2_corr;
bool _co2_found;
```

### PID Sensor (Channel 3)

**Data Variables:**
```cpp
float _pid_volt;       // Average voltage
float _pid_volt_max;   // Maximum voltage
float _pid_sum;        // Accumulated sum
unsigned short _pid_smp_count; // Sample count
```

**Peak Detection Variables:**
```cpp
float _pid_signal, _pid_base, _pid_top, _pid_mem;
float _pid_tcomp, _pid_dx, _pid_dx_trig, _pid_corr;
bool _pid_found;
```

## Calibration Equations and Algorithms

### Linear Calibration Method

For standard operation, the system uses linear regression to convert voltage to concentration:

**C1/CH4 Linear Calculation:**
```cpp
range = _c1_range;
avgX = (_c1_offset_volt + 2.0) / 2.0;
avgY = (0 + _c1_range) / 2.0;
ssX = pow((avgX - _c1_offset_volt), 2.0);
ssY = (avgX - _c1_offset_volt) * (avgY - 0);
slope = ssY / ssX;
intercept = avgY - (slope * avgX);

concentration = (slope * _c1_corr) + intercept;
```

**CO2 Linear Calculation:**
```cpp
range = _co2_range;
avgX = (_co2_offset_volt + 2.0) / 2.0;
avgY = (407.0 + _co2_range) / 2.0; // Baseline CO2 = 407ppm
ssX = pow((avgX - _co2_offset_volt), 2.0) + pow((avgX - 2.0), 2.0);
ssY = (avgX - _co2_offset_volt) * (avgY - 407.0) + ((avgX - 2.0) * (avgY - _co2_range));
slope = ssY / ssX;
intercept = avgY - (slope * avgX);

concentration = (slope * _co2_corr) + intercept;
```

### Peak Detection Mode

When `_offset_volt > 3.0V`, the system uses peak detection calibration:

```cpp
if (_offset_volt > 3) {
    concentration = _gain * _signal;
}
```

This mode uses the detected peak signal value rather than averaged voltage.

### PID Sensor Processing

**Standard Mode:**
```cpp
voltage_diff = _pid_corr - _pid_offset_volt;
if (voltage_diff < 0) { voltage_diff = 0; }
concentration = voltage_diff * _pid_gain; // volt * (ppm/volt)
```

**Linear Correction (Optional):**
```cpp
if ((_pid_lin_m != 0) && (_pid_lin_b != 0)) {
    corrected_value = _pid_lin_m * _pid_max + _pid_lin_b;
    if (corrected_value < 0) { corrected_value = 0; }
    _pid = corrected_value;
}
```

## Temperature Compensation

### Temperature Correction Formula

Temperature compensation is applied during calibration and processing:

```cpp
float dxtemp = _temperature - _temperature_mem;
offset = ADvolt - (dxtemp * tcomp);
if (offset < 0) { offset = 0; }
```

**Temperature Compensation Variables:**
- `_c1_tcomp`: C1/CH4 temperature compensation factor
- `_co2_tcomp`: CO2 temperature compensation factor  
- `_pid_tcomp`: PID temperature compensation factor

### Peak Detection Integration

The `formatIR()` function applies peak detection before concentration calculation:

```cpp
peakDetect("irc1", &_c1_corr, &_c1_volt_max, &_c1_signal, &_c1_base, 
          &_c1_top, &_c1_mem, &_c1_tcomp, &_c1_dx, &_c1_dx_trig, &_c1_found);

peakDetect("irco2", &_co2_corr, &_co2_volt_max, &_co2_signal, &_co2_base,
          &_co2_top, &_co2_mem, &_co2_tcomp, &_co2_dx, &_co2_dx_trig, &_co2_found);
```

## Calibration Procedures

### Zero Calibration (`zeroIR()`)

**Process:**
1. **Pause Data Collection**: Stop sampling during calibration
2. **Read Current Voltage**: Take ADC reading in clean air
3. **Apply Temperature Compensation**: Adjust for temperature differential
4. **Set Zero Point**: Store calibrated offset voltage
5. **Save Configuration**: Persist calibration to EEPROM

**Implementation:**
```cpp
bool zeroIR(byte model) {
    if (pauseCollect() == false) { return false; }
    
    float dxtemp = _temperature - _temperature_mem;
    IR_read(channel); // Read appropriate channel
    
    offset = ADvolt - (dxtemp * tcomp);
    if (offset < 0) { offset = 0; }
    
    // Store offset for specific sensor
    _sampling_status = true; // Resume sampling
    saveIRConfig(model);
    return true;
}
```

### Peak Detection Calibration (`calibIRdetect()`)

**Process:**
1. **Expose to Known Gas**: Apply standard gas concentration
2. **Wait for Peak Detection**: Monitor `_found` flag
3. **Calculate Gain**: `gain = ppmStd / _signal`
4. **Set Detection Mode**: `offset = 3.3V` (marker for peak mode)
5. **Save Parameters**: Store gain and trigger thresholds

**Typical Calibration Values:**
- **C1/CH4**: 50,000ppm standard → ~26µV/ppm sensitivity
- **CO2**: 30% standard → ~6.3µV/ppm sensitivity  
- **PID**: 40ppm standard → ~25mV/ppm sensitivity

### Span Calibration (`configureIR()`)

**Parameters:**
- **model**: Sensor type (0=C1, 1=CO2, 2=PID)
- **enable**: Enable/disable sensor
- **range**: Maximum expected concentration
- **gain**: Conversion factor (ppm/volt)

## Voltage-to-Concentration Conversions

### C1/CH4 Sensor

**Linear Mode:**
- Uses two-point linear regression between offset voltage and 2.0V
- Range: 0 to configured maximum (typically 50,000ppm)
- Baseline: 0ppm at offset voltage

**Peak Detection Mode:**
- Direct multiplication: `ppm = gain × signal_voltage`
- Gain typically ~26,000 ppm/V for 50,000ppm range

### CO2 Sensor

**Linear Mode:**
- Uses two-point linear regression with 407ppm baseline
- Range: 407ppm to configured maximum
- Accounts for atmospheric CO2 baseline

**Peak Detection Mode:**
- Direct multiplication: `ppm = gain × signal_voltage`
- Gain typically ~158,000 ppm/V for high-range sensors

### PID Sensor

**Standard Mode:**
- Simple linear: `ppm = (voltage - offset) × gain`
- Gain typically 40 ppm/V for standard PID sensors

**Linear Correction Mode:**
- Additional correction: `ppm = m × voltage_max + b`
- Applied when linear coefficients are configured

## Data Output Format

### Concentration Values
- `_c1`, `_co2`, `_pid`: Final concentrations in ppm
- `_c1_max`, `_co2_max`, `_pid_max`: Peak values during sampling

### Diagnostic Values
- `_c1_volt`, `_co2_volt`, `_pid_volt`: Average voltages
- `_c1_volt_max`, `_co2_volt_max`, `_pid_volt_max`: Maximum voltages
- Sample counts and error flags

### Engineering Mode

When `_is_engineering` is true:
- Returns raw voltage values instead of concentrations
- Bypasses all calibration calculations
- Useful for sensor diagnostics and calibration verification

## Error Handling

### Communication Errors
- ADC read failures set `_ir_err` flag
- Individual sensor errors tracked per channel
- Boot delay prevents premature readings

### Range Validation
- Negative concentrations clamped to zero
- Maximum values limited to configured ranges
- Out-of-range conditions logged

### Sensor Protection
- 90-second boot delay prevents unstable readings
- Temperature compensation prevents drift errors
- Peak detection validates signal quality

The infrared sensor system provides accurate gas concentration measurements through sophisticated calibration algorithms, temperature compensation, and peak detection for reliable environmental monitoring.