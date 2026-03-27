# VOC and Gas Sensors Analysis

## Overview

The air monitoring system includes volatile organic compound (VOC) sensors using either SGP30 or SGP40 sensors from Sensirion. These sensors provide gas detection capabilities with sophisticated signal processing including peak detection algorithms, exponential calibration functions, and temperature/humidity compensation.

## Hardware Architecture

### Sensor Types and I2C Configuration

**I2C Addresses:**
- 0x58: SGP30 sensor (discontinued but supported)
- 0x59: SGP40 sensor (current generation)

**Sensor Detection Priority:**
1. **SGP30**: Legacy sensor providing TVOC (ppb) and eCO2 (ppm) measurements
2. **SGP40**: Current sensor providing raw VOC signals for custom processing

### Sensor Capabilities

**SGP30 Features:**
- Total VOC (TVOC) in parts per billion (ppb)
- Equivalent CO2 (eCO2) in parts per million (ppm)
- Built-in baseline compensation
- Factory calibrated algorithms

**SGP40 Features:**
- Raw VOC signal output (16-bit counts)
- Temperature and humidity compensation
- Custom algorithm implementation
- Peak detection processing

## SGP30 Sensor Processing

### Data Collection Process

The SGP30 provides processed measurements through built-in algorithms:

```cpp
if (sgp30.IAQmeasure()) {
    _tvoc += sgp30.TVOC;    // Total VOC in ppb
    _eco2 += sgp30.eCO2;    // Equivalent CO2 in ppm
    _voc_sample_count++;
}
```

### SGP30 Data Variables

```cpp
float _tvoc;               // Total VOC concentration (ppb)
float _eco2;               // Equivalent CO2 concentration (ppm)
bool _is_sgp30_on;         // SGP30 sensor active flag
```

### SGP30 Output Format

```
TVOC: 125ppb    eCO2: 450ppm
```

## SGP40 Sensor Processing

### Raw Signal Measurement

The SGP40 provides raw sensor signals that require custom processing:

```cpp
void sgp40LowPower(float temp = 25, float rhumidity = 50) {
    uint16_t compensationRh, compensationT;
    uint16_t srawVoc = 0;
    
    // Convert temperature and humidity to sensor format
    compensationRh = rhumidity * (65535.0 / 100.0);
    compensationT = (temp + 45.0) * (65535.0 / 175);
    
    // Measure raw VOC signal with compensation
    error = sgp40.measureRawSignal(compensationRh, compensationT, srawVoc);
    
    if (error == 0) {
        // Invert signal for peak detection (negative peaks become positive)
        srawVoc = 65535.0 - srawVoc;
        
        _voc_raw += srawVoc;
        if (srawVoc > _voc_raw_max) { _voc_raw_max = srawVoc; }
        _voc_sample_count++;
    }
}
```

### Temperature and Humidity Compensation

**Compensation Formula:**
```cpp
// Humidity: 0-100% RH → 0x0000-0xFFFF
compensationRh = rhumidity * (65535.0 / 100.0);

// Temperature: -45°C to +130°C → 0x0000-0xFFFF  
compensationT = (temp + 45.0) * (65535.0 / 175);
```

**Compensation Application:**
- Uses real-time temperature and humidity from BME280/BME680
- Fallback to default values (25°C, 50%RH) if environmental data unavailable
- Validates environmental data ranges before application

### SGP40 Data Variables

```cpp
float _voc_raw;            // Average raw VOC signal
float _voc_raw_max;        // Maximum raw VOC signal
float _voc;                // Processed VOC concentration
float _voc_gain;           // Calibration gain factor
float _voc_offset;         // Calibration offset
bool _is_sgp40_on;         // SGP40 sensor active flag
```

## Peak Detection Algorithm

### Peak Detection Variables

```cpp
float _voc_signal;         // Current peak signal value
float _voc_base;           // Baseline reference value
float _voc_top;            // Peak top value
float _voc_mem;            // Memory/history value
float _voc_tcomp;          // Temperature compensation factor
float _voc_dx;             // Differential value
float _voc_dx_trig;        // Trigger threshold for peak detection
float _voc_corr;           // Temperature corrected value
bool _voc_found;           // Peak detection flag
```

### Peak Detection Process

The system applies peak detection during data formatting:

```cpp
void formatVOC() {
    if (_voc_sample_count > 0) {
        _voc_raw = _voc_raw / _voc_sample_count;
        
        // Apply peak detection algorithm
        peakDetect("voc", &_voc_corr, &_voc_raw_max, &_voc_signal, 
                  &_voc_base, &_voc_top, &_voc_mem, &_voc_tcomp, 
                  &_voc_dx, &_voc_dx_trig, &_voc_found);
        
        // Calculate final VOC concentration
        _voc = computeVOC();
    }
}
```

## Calibration and Conversion Functions

### Multiple Calibration Methods

The system supports three different calibration approaches:

1. **Linear Calibration**
2. **Exponential Calibration** 
3. **Peak Detection Calibration**

### Linear Calibration

```cpp
if (_voc_lin_m != 0) {
    concentration = _voc_lin_m * x + _voc_lin_b;
    Serial.printf("VOC LIN(x%e): %e (m%e, b%e)\r\n", x, concentration, _voc_lin_m, _voc_lin_b);
}
```

**Linear Variables:**
```cpp
float _voc_lin_m;          // Linear slope coefficient
float _voc_lin_b;          // Linear intercept coefficient
```

### Exponential Calibration

Based on SGP40 response curve fitting from datasheet:

```cpp
float computeExponential(float A, float b, float x, float C) {
    float exp_input = b * x;
    
    // Overflow protection
    if (exp_input > 88.7) {
        Serial.println("Warning: Exponential overflow, returning INFINITY");
        return INFINITY;
    } else if (exp_input < -103.0) {
        Serial.println("Warning: Exponential underflow, returning 0.0");
        return 0.0;
    }
    
    return A * exp(b * x) + C;
}
```

**Exponential Variables:**
```cpp
float _voc_exp_A;          // Exponential amplitude coefficient
float _voc_exp_b;          // Exponential rate coefficient  
float _voc_exp_C;          // Exponential offset coefficient
```

**Default Exponential Parameters:**
- A: 0.5177
- b: 0.0007834
- C: -0.03093

### Peak Detection Calibration

```cpp
if (_voc_offset >= 0) {
    // Standard offset calibration
    x = _voc_raw_max - _voc_offset;
} else {
    // Peak detection mode (_voc_offset = -1)
    x = _voc_signal;
}
```

### Concentration Calculation Logic

```cpp
float computeVOC(void) {
    float x, gain, concentration;
    
    if (_is_engineering) {
        // Engineering mode: return raw values
        concentration = _voc_raw;
    } else {
        gain = _voc_gain; // ppm/count
        
        // Determine input signal source
        if (_voc_offset >= 0) {
            x = _voc_raw_max - _voc_offset;
        } else {
            x = _voc_signal; // Peak detection mode
        }
        
        // Apply calibration method
        if ((_voc_exp_A != 0) || (_voc_exp_b != 0) || (_voc_exp_C != 0)) {
            // Exponential calibration
            concentration = computeExponential(_voc_exp_A, _voc_exp_b, x, _voc_exp_C);
        } else if (_voc_lin_m != 0) {
            // Linear calibration
            concentration = _voc_lin_m * x + _voc_lin_b;
        } else {
            // Simple gain calibration
            concentration = gain * x;
        }
    }
    
    // Clamp negative values
    if (concentration < 0) {
        concentration = 0;
        Serial.println("Warning: Negative VOC concentration, setting to 0");
    }
    
    return concentration;
}
```

## Power Management

### Sleep Mode Control

```cpp
void sleepVOC() {
    if (_is_sgp40_on) {
        sgp40.turnHeaterOff();
    }
}
```

The SGP40 includes an internal heater that can be disabled during sleep periods to save power.

### Continuous Operation

Unlike particulate matter sensors, VOC sensors typically require continuous operation for:
- Baseline stability
- Temperature compensation accuracy
- Peak detection algorithm effectiveness

## Error Handling and Validation

### Communication Error Detection

```cpp
uint16_t error = sgp40.measureRawSignal(compensationRh, compensationT, srawVoc);
if (error != 0) {
    sgp40PrintErr("Err2 SGP40 reading: ", error);
    _voc_err = true;
    return;
}
```

### Error Reporting Function

```cpp
void sgp40PrintErr(String output, uint16_t error) {
    char errorMessage[256];
    Serial.print(output);
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
}
```

### Range Validation

```cpp
// Validate environmental compensation inputs
if ((_bme_err == false) && (_temperature > -40) && (_temperature < 80) && 
    (_humidity > 0) && (_humidity < 100)) {
    sgp40LowPower(_temperature, _humidity);
} else {
    sgp40LowPower(); // Use default compensation
}
```

### Signal Range Protection

```cpp
if (srawVoc > 65535.0) { srawVoc = 65535.0; }
```

## Sensor Initialization

### Auto-Detection Process

```cpp
void setupVOC() {
    _voc_i2c = false;
    clearVOC();
    
    if (_voc_enable == false) { return; }
    
    // Check I2C communication
    if ((_tcaError & 0x04) == 0x04) {
        Serial.println("VOC not found");
        return;
    }
    _voc_i2c = true;
    
    // Try SGP30 first
    if (sgp30.begin()) {
        initDetect(&_voc_signal, &_voc_base, &_voc_mem, &_voc_found);
        Serial.print("Found VOC SGP30 #");
        Serial.print(sgp30.serialnumber[0], HEX);
        Serial.print(sgp30.serialnumber[1], HEX);
        Serial.println(sgp30.serialnumber[2], HEX);
        _is_sgp30_on = true;
        return;
    }
    
    // Try SGP40 if SGP30 not found
    sgp40.begin(Wire);
    uint16_t serialNumber[3];
    uint16_t testResult;
    
    if (sgp40.getSerialNumber(serialNumber, 3) == 0) {
        if ((sgp40.executeSelfTest(testResult) == 0) && (testResult == 0xD400)) {
            initDetect(&_voc_signal, &_voc_base, &_voc_mem, &_voc_found);
            Serial.println("Found VOC SGP40");
            _is_sgp40_on = true;
        } else {
            Serial.println("VOC SGP40 test failed");
        }
    } else {
        Serial.println("VOC SGP40 not found");
    }
}
```

### Self-Test Validation

The SGP40 includes a self-test function:
- Expected test result: 0xD400
- Validates sensor functionality before operation
- Prevents operation with faulty sensors

## Data Output Format

### SGP30 Output
- **TVOC**: Total Volatile Organic Compounds (ppb)
- **eCO2**: Equivalent CO2 concentration (ppm)

### SGP40 Output  
- **VOC**: Processed VOC concentration (custom units)
- **Raw Signal**: 16-bit sensor counts
- **Peak Values**: Maximum detected signals

### Serial Output Examples

**SGP30:**
```
TVOC: 125ppb    eCO2: 450ppm
```

**SGP40:**
```
srawVOC:32145 - Temp 23.45C / 0x6789 - Humidity 45.67RH / 0x7532
VOC EXP(x3.214500e+04): 1.234500e+02 (A5.177000e-01, b7.834000e-04, C-3.093000e-02)
```

### Engineering Mode

When `_is_engineering` is true:
- Returns raw sensor values without processing
- Bypasses all calibration calculations
- Useful for sensor diagnostics and calibration development

## Advanced Features

### Non-Compensated Measurements

For diagnostic purposes, the system can take measurements without temperature/humidity compensation:

```cpp
void sgp40nocomp() {
    uint16_t srawVoc = 0;
    uint16_t error = sgp40.measureRawSignal(0x8000, 0x6666, srawVoc);
    // 0x8000 = 50% RH, 0x6666 = 25°C (default conditions)
}
```

### Multiple Algorithm Support

The system can simultaneously apply different processing methods:
1. Peak detection for transient events
2. Exponential fitting for concentration curves
3. Linear correction for calibration adjustments

### Temperature Compensation Integration

VOC measurements integrate with the environmental sensor system:
- Real-time temperature and humidity compensation
- Validation of environmental data quality
- Fallback to default conditions when needed

The VOC sensor system provides sophisticated gas detection with multiple calibration methods, advanced signal processing, and robust error handling for accurate volatile organic compound monitoring.