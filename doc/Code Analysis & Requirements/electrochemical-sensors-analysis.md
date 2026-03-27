# Electrochemical Sensor Processing Analysis

## Overview

The air monitoring system supports multiple electrochemical sensors for gas detection including H2S, O3, SO2, NO2, and NH3. These sensors use LMP91000 potentiostat chips for signal conditioning and MCP342x ADCs for voltage measurement. The system implements sophisticated peak detection algorithms and temperature compensation for accurate gas concentration measurements.

## Hardware Architecture

### LMP91000 Potentiostat Configuration

The system uses LMP91000 potentiostat chips to condition electrochemical sensor signals:

**I2C Addresses:**
- 0x48: LMP91000 potentiostat interface
- 0x6B: MCP342x ADC for SPEC sensors (IC103)
- 0x6A: MCP342x ADC for NH3 sensor (IC103)
- 0x3B: PCF8574 I/O expander for sensor control (IC107)

**Sensor Channel Mapping:**
- Channel 1: H2S (U4_U9)
- Channel 2: NO2 (U3_U8) 
- Channel 3: O3 (U1_U6)
- Channel 4: SO2 (U2_U7)

### Potentiostat Settings by Sensor

**H2S Configuration:**
- IC=8, CSbit=0, Gain=7KΩ, IntZ=50%, BiasSign=Positive, Bias=0%
- TIA: 7KΩ (SGX CAN) / 120KΩ (SPEC)
- Sensitivity: 1200nA/ppm (SGX) / 212nA/ppm (SPEC)

**NO2 Configuration:**
- IC=9, CSbit=1, Gain varies by sensor type
- PS4-NO2-2 (2ppm range): Gain=499KΩ, Sensitivity=45nA/ppm
- Standard (30ppm range): Gain=35KΩ, Sensitivity=30nA/ppm
- IntZ=50%, BiasSign=Negative, Bias=0-1%

**O3 Configuration:**
- IC=12, CSbit=2, Gain=35KΩ (SGX) / 499KΩ (SPEC)
- IntZ=50%, BiasSign=Negative, Bias=1%
- Sensitivity: 1000nA/ppm (SGX) / 60nA/ppm (SPEC)

**SO2 Configuration:**
- IC=3, CSbit=3, Gain=120KΩ (SGX) / 499KΩ (SPEC)
- IntZ=50%, BiasSign=Positive, Bias=0-10%
- Sensitivity: 400nA/ppm (SGX) / 25nA/ppm (SPEC)

**NH3 Configuration (SGX CAN only):**
- IC=16, CSbit=4, Gain=35KΩ
- IntZ=50%, BiasSign=Positive, Bias=0%
- Sensitivity: 40nA/ppm

## ADC Reading Process

### MCP342x ADC Configuration

The system uses 18-bit resolution ADCs with the following settings:
- Resolution: 18-bit (131071 counts = 2.048V)
- Gain: 1x
- Mode: One-shot conversion
- Reference: External 2.048V

### Voltage Conversion Formula

```cpp
AD_ECvolt = (AD_ECcount * 2.048) / 131071;
```

### Data Collection Process

The `collectEC()` function performs the following steps:

1. **Channel Selection**: Uses `setSPEC_ON(csbit)` to select the appropriate sensor
2. **ADC Reading**: Calls `SPEC_read(channel)` for voltage measurement
3. **Averaging**: Accumulates readings in sum variables and maintains sample counts
4. **Peak Tracking**: Updates maximum voltage values for each sensor
5. **Error Handling**: Sets error flags if ADC communication fails

## Calibration Equations and Algorithms

### Gain Factor Calculation

The system calculates gain factors using the M-Factor formula:

```cpp
float MFactor_SPEC(float sensitivity, float TIA) {
    float S1 = sensitivity * TIA * 0.000001; 
    return S1; // Volts per ppm
}

float gain = 1 / MFactor; // ppm per Volt
```

### Concentration Calculation

The `computeSPEC()` function calculates gas concentrations using:

```cpp
concentration = gain * (volt - offset);
if (concentration < 0) { concentration = 0; }
```

**Calibration Modes:**
1. **Standard Mode**: Uses averaged voltage (`_volt`)
2. **Peak Detection Mode**: Uses signal voltage when `offset > 3.0V`
3. **Engineering Mode**: Returns raw voltage values

### Temperature Compensation

Temperature compensation is applied during zero calibration:

```cpp
float dxtemp = _temperature - _temperature_mem;
offset = AD_ECvolt - (dxtemp * tcomp);
if (offset < 0) { offset = 0; }
```

**Temperature Compensation Variables:**
- `_h2s_tcomp`, `_o3_tcomp`, `_so2_tcomp`, `_no2_tcomp`, `_nh3_tcomp`
- Applied as voltage correction based on temperature differential

## Peak Detection Algorithm

### Peak Detection Variables (per sensor)

```cpp
float _signal;    // Current peak signal value
float _base;      // Baseline reference value  
float _top;       // Peak top value
float _mem;       // Memory/history value
float _tcomp;     // Temperature compensation factor
float _dx;        // Differential value
float _dx_trig;   // Trigger threshold for peak detection
float _corr;      // Temperature corrected value
bool _found;      // Peak detection flag
```

### Peak Detection Process

The system calls `peakDetect()` for each sensor during `formatEC()`:

```cpp
peakDetect("h2s", &_h2s_corr, &_h2s_volt_max, &_h2s_signal, 
          &_h2s_base, &_h2s_top, &_h2s_mem, &_h2s_tcomp, 
          &_h2s_dx, &_h2s_dx_trig, &_h2s_found);
```

The algorithm detects concentration peaks above baseline noise levels and maintains signal/base/top values for accurate measurement.

## Calibration Procedures

### Zero Calibration (`zeroSPEC()`)

1. **Pause Data Collection**: Stops sampling during calibration
2. **Read Current Voltage**: Takes ADC reading in clean air
3. **Apply Temperature Compensation**: Adjusts for temperature differential
4. **Set Offset**: Stores calibrated zero point
5. **Save Configuration**: Persists calibration to EEPROM

### Peak Detection Calibration (`calibSPECdetect()`)

1. **Expose to Known Gas**: Apply standard gas concentration
2. **Wait for Peak Detection**: Monitor `_found` flag
3. **Calculate Gain**: `gain = ppmStd / _signal`
4. **Set Detection Mode**: `offset = 3.3V` (marker for peak mode)
5. **Save Parameters**: Store gain and trigger thresholds

### Span Calibration (`configureSPEC()`)

1. **Calculate M-Factor**: Based on sensor sensitivity and TIA resistance
2. **Set Gain Factor**: `gain = 1 / MFactor`
3. **Configure Hardware**: Set potentiostat parameters
4. **Enable Sensor**: Activate data collection

## Linear Correction Factors

The system supports additional linear correction:

```cpp
if (_h2s_lin_m != 0) { _h2s = _h2s_lin_m * _h2s + _h2s_lin_b; }
if (_o3_lin_m != 0) { _o3 = _o3_lin_m * _o3 + _o3_lin_b; }
if (_so2_lin_m != 0) { _so2 = _so2_lin_m * _so2 + _so2_lin_b; }
if (_no2_lin_m != 0) { _no2 = _no2_lin_m * _no2 + _no2_lin_b; }
```

## NH3 Sensor Special Processing

### MICS6814 Multi-Gas Sensor

For non-SGX versions, NH3 uses a MICS6814 sensor with three channels:
- **RED Channel**: CO detection (reduction gases)
- **OX Channel**: NO2 detection (oxidation gases)  
- **NH3 Channel**: NH3 detection

### Resistance Calculation

```cpp
// Voltage divider: VSense = 2.048V * RLoad / (RLoad + RSense)
// Solve for RSense: RSense = RLoad * VSense / (2.048V - VSense)
fltRSense = (fltRLoad * fltVSense) / (2.048 - fltVSense);
```

### Gas Concentration Formulas

**CO Concentration:**
```cpp
ratio = fltRSense / _red_r0;
CO_ppm = pow(ratio, -1.172479) * 4.51885;
```

**NO2 Concentration:**
```cpp
ratio = fltRSense / _ox_r0;
NO2_ppm = pow(ratio, 0.9979) * 0.1516;
```

**NH3 Concentration:**
```cpp
ratio = fltRSense / _nh3_r0;
NH3_ppm = pow(ratio, -2.009362) * 0.46565;
```

## Error Handling and Protection

### Error Detection

- **I2C Communication Errors**: ADC read failures set `_spec_err`
- **Sensor Hardware Errors**: Individual sensor error flags (`_h2s_err`, etc.)
- **Range Validation**: Concentration bounds checking
- **Temperature Protection**: Sensor lockout at extreme temperatures

### Protection Mode

The `pStat_Protect()` function can put sensors in sleep mode:
- **Sleep Mode**: `setMode(0)` and `setFET(1)`
- **Active Mode**: `setFET(0)` and `setMode(3)`

### Data Validation

- Negative concentrations are clamped to zero
- Engineering mode bypasses concentration calculations
- Peak detection requires signal above trigger threshold
- Temperature compensation prevents drift errors

## Data Output Format

### Concentration Values
- `_h2s`, `_o3`, `_so2`, `_no2`, `_nh3`: Final concentrations in ppm
- `_h2s_max`, `_o3_max`, etc.: Peak voltage values during sampling period

### Diagnostic Values
- `_h2s_volt`, `_o3_volt`, etc.: Average voltages
- `_h2s_volt_max`, `_o3_volt_max`, etc.: Maximum voltages
- Error flags and sensor enable states

The electrochemical sensor system provides comprehensive gas detection with sophisticated calibration, temperature compensation, and peak detection algorithms for accurate environmental monitoring.