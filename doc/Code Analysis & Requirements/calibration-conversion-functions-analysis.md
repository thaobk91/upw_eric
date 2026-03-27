# Calibration and Conversion Functions Analysis

## Overview

The air monitoring system implements comprehensive calibration and conversion functions to transform raw sensor voltages into meaningful concentration values. These functions include linear conversions, exponential transformations, and multi-stage calibration procedures stored in EEPROM.

## Core Conversion Formula (computeSPEC)

### Universal Sensor Conversion
```cpp
concentration = gain * (volt - offset);
if (concentration < 0) { concentration = 0; }
if (_is_engineering) { concentration = volt; }
```

### Function Implementation
```cpp
float computeSPEC(byte model) {
    float offset, volt, gain, concentration;
    
    switch (model) {
        case 0: // H2S
            offset = _h2s_offset_volt;
            gain = _h2s_gain;
            volt = _h2s_corr; // Temperature compensated voltage
            if (_is_engineering) { volt = _h2s_volt; } // Raw voltage in engineering mode
            if ((offset > 3) && (_is_engineering == false)) {
                offset = 0;
                volt = _h2s_signal; // Use peak detection signal
            }
            break;
        // Similar cases for O3, SO2, NO2, NH3...
    }
    
    concentration = gain * (volt - offset);
    if (concentration < 0) { concentration = 0; }
    if (_is_engineering) { concentration = volt; }
    
    return concentration;
}
```

## Calibration Parameter Storage (EEPROM)

### EEPROM Page Allocation
- **Page 1**: C1 (CH4) sensor calibration
- **Page 2**: CO2 sensor calibration  
- **Page 3**: PID sensor calibration
- **Page 4**: NH3 sensor calibration
- **Page 5**: H2S sensor calibration
- **Page 6**: NO2 sensor calibration
- **Page 7**: SO2 sensor calibration
- **Page 8**: O3 sensor calibration
- **Page 9**: Temperature compensation parameters
- **Page 10**: VOC sensor calibration

### Calibration Data Format
```cpp
// Format: "SENSOR_NAME enable range gain offset_volt tcomp dx_trig"
sprintf(_EEpagebuffer, "H2S %u %f %f %f %f %f", 
        _h2s_enable, fRange, _h2s_gain, _h2s_offset_volt, _h2s_tcomp, _h2s_dx_trig);
```

### Parameter Definitions
- **enable**: Sensor enable/disable flag (boolean)
- **range**: Sensor measurement range (ppm or ppb)
- **gain**: Conversion gain factor (ppm/V or ppb/V)
- **offset_volt**: Zero-point offset voltage (V)
- **tcomp**: Temperature compensation coefficient (V/°C)
- **dx_trig**: Peak detection trigger threshold (V)

### EEPROM Data Parsing
```cpp
int scanCalib(char *_name, bool *_enable, float *_range, float *_gain, 
              float *_offset, float *_tComp, float *_dxTrigger) {
    // Parse space-separated calibration string
    strcpy(_name, strings[0]);
    *_enable = atoi(strings[1]);
    *_range = atof(strings[2]);
    *_gain = atof(strings[3]);
    *_offset = atof(strings[4]);
    *_tComp = atof(strings[5]);
    *_dxTrigger = atof(strings[6]);
    return index;
}
```

## Gain Formatting and Display

### Gain Conversion Function
```cpp
float formatGain(float fGain) {
    if (fGain == 0) { return 0.0; }
    return ((1 / fGain) * 1000);
}
```

### Purpose
- Converts internal gain values to user-friendly display format
- Formula: `display_gain = (1 / internal_gain) × 1000`
- Used for serial output and debugging displays

## Linear Conversion Functions (y = mx + b)

### Implementation Pattern
```cpp
// Generic linear conversion: y = mx + b
if (_sensor_lin_m != 0) {
    corrected_value = _sensor_lin_m * original_value + _sensor_lin_b;
}
```

### Sensor-Specific Linear Corrections

#### Electrochemical Sensors
```cpp
// Applied after computeSPEC calculation
if (_h2s_lin_m != 0) { _h2s = _h2s_lin_m * _h2s + _h2s_lin_b; }
if (_o3_lin_m != 0) { _o3 = _o3_lin_m * _o3 + _o3_lin_b; }
if (_so2_lin_m != 0) { _so2 = _so2_lin_m * _so2 + _so2_lin_b; }
if (_no2_lin_m != 0) { _no2 = _no2_lin_m * _no2 + _no2_lin_b; }
```

#### PID Sensor
```cpp
// Applied to voltage reading
if ((_pid_lin_m != 0) && (_pid_lin_b != 0)) {
    S1 = _pid_lin_m * _pid_volt_max + _pid_lin_b;
    if (S1 < 0) { S1 = 0; }
    _pid = S1;
}
```

#### VOC Sensor
```cpp
// Applied within computeVOC function
if (_voc_lin_m != 0) {
    concentration = _voc_lin_m * x + _voc_lin_b;
    Serial.printf("VOC LIN(x%e): %e (m%e, b%e)\r\n", x, concentration, _voc_lin_m, _voc_lin_b);
}
```

### Configuration Management
```cpp
// Check if linear parameters exist in config.json
if (_config_flash["hardware"]["sensors"]["h2s_lin_m"].isNull() || 
    _config_flash["hardware"]["sensors"]["h2s_lin_b"].isNull()) {
    // Set default values
    _h2s_lin_m = 0;
    _h2s_lin_b = 0;
    // Create entries in config.json
    _config_flash["hardware"]["sensors"]["h2s_lin_m"] = _h2s_lin_m;
    _config_flash["hardware"]["sensors"]["h2s_lin_b"] = _h2s_lin_b;
} else {
    // Load from configuration
    _h2s_lin_m = _config_flash["hardware"]["sensors"]["h2s_lin_m"];
    _h2s_lin_b = _config_flash["hardware"]["sensors"]["h2s_lin_b"];
}
```

## Exponential Conversion Functions

### VOC Exponential Model
```cpp
// Function: y = A * e^(b * x) + C
float computeExponential(float A, float b, float x, float C) {
    float exp_input = b * x;
    
    // Prevent overflow/underflow
    if (exp_input > 50) { exp_input = 50; }
    if (exp_input < -50) { exp_input = -50; }
    
    float result = A * exp(exp_input) + C;
    return result;
}
```

### VOC Conversion Implementation
```cpp
float computeVOC(void) {
    float x, gain, concentration;
    
    if (_is_engineering) {
        concentration = _voc_raw_max; // Raw ADC value
    } else {
        gain = _voc_gain;
        if (_voc_offset >= 0) {
            // Standard gain conversion
            x = _voc_raw_max - _voc_offset;
            concentration = gain * x;
        } else {
            // Peak detection mode
            x = _voc_signal;
            concentration = gain * x;
        }
        
        // Apply exponential correction if configured
        if ((_voc_exp_A != 0) && (_voc_exp_b != 0)) {
            concentration = computeExponential(_voc_exp_A, _voc_exp_b, x, _voc_exp_C);
            Serial.printf("VOC EXP(x%e): %e (A%e, b%e, C%e)\r\n", x, concentration, _voc_exp_A, _voc_exp_b, _voc_exp_C);
        }
        
        // Apply linear correction if configured
        if (_voc_lin_m != 0) {
            concentration = _voc_lin_m * x + _voc_lin_b;
            Serial.printf("VOC LIN(x%e): %e (m%e, b%e)\r\n", x, concentration, _voc_lin_m, _voc_lin_b);
        }
    }
    
    return concentration;
}
```

### Exponential Parameters
- **A**: Amplitude coefficient
- **b**: Exponential rate coefficient  
- **C**: Offset constant
- **x**: Input variable (corrected voltage or signal)

## Infrared Sensor Conversions

### CO2 Sensor Conversion
```cpp
void computeCO2(void) {
    float range, avgX, avgY, ssX, ssY, slope, intercept, Y;
    
    _co2_max = _co2_volt_max;
    if (_is_engineering) {
        _co2 = _co2_volt_max; // Engineering mode: return voltage
    } else {
        // Apply calibration curve conversion
        // Implementation uses statistical regression for calibration
        range = _co2_range;
        // ... statistical calculations for slope and intercept
        Y = slope * _co2_volt_max + intercept;
        if (Y < 0) { Y = 0; }
        _co2 = Y;
    }
}
```

### C1 (CH4) Sensor Conversion
```cpp
void computeC1(void) {
    float range, avgX, avgY, ssX, ssY, slope, intercept, Y;
    
    _c1_max = _c1_volt_max;
    if (_is_engineering) {
        _c1 = _c1_volt_max; // Engineering mode: return voltage
    } else {
        // Apply calibration curve conversion
        range = _c1_range;
        // ... statistical calculations for slope and intercept
        Y = slope * _c1_volt_max + intercept;
        if (Y < 0) { Y = 0; }
        _c1 = Y;
    }
}
```

### PID Sensor Conversion
```cpp
void computePID(void) {
    float S1;
    
    _pid_max = _pid_volt_max;
    
    // Apply linear correction if configured
    if ((_pid_lin_m != 0) && (_pid_lin_b != 0)) {
        S1 = _pid_lin_m * _pid_volt_max + _pid_lin_b;
        if (S1 < 0) { S1 = 0; }
        _pid = S1;
    } else {
        // Standard conversion (implementation varies)
        _pid = _pid_volt_max; // Placeholder
    }
}
```

## Calibration Modes and Voltage Selection

### Engineering Mode vs Normal Mode
```cpp
// In computeSPEC function:
if (_is_engineering) {
    volt = _sensor_volt; // Raw voltage reading
    concentration = volt; // Return voltage directly
} else {
    volt = _sensor_corr; // Temperature compensated voltage
    // Apply full conversion chain
}
```

### Peak Detection vs Continuous Mode
```cpp
// Offset threshold determines mode
if ((offset > 3) && (_is_engineering == false)) {
    offset = 0;
    volt = _sensor_signal; // Use peak detection signal
} else {
    volt = _sensor_corr; // Use continuous corrected voltage
}
```

### Voltage Source Selection Priority
1. **Engineering Mode**: Raw voltage (`_sensor_volt`)
2. **Peak Detection Mode**: Peak signal (`_sensor_signal`) when offset > 3V
3. **Normal Mode**: Temperature compensated voltage (`_sensor_corr`)

## Configuration Integration

### JSON Configuration Loading
```cpp
// Load calibration parameters from config.json
_h2s_gain = _config_flash["hardware"]["sensors"]["h2s_gain"];
_h2s_offset_volt = _config_flash["hardware"]["sensors"]["h2s_offset"];
_h2s_tcomp = _config_flash["hardware"]["sensors"]["h2s_tcomp"];
_h2s_dx_trig = _config_flash["hardware"]["sensors"]["h2s_trig"];
```

### Dynamic Parameter Updates
```cpp
// MQTT command for updating sensor parameters
if (strcmp(command,"updateSensorJson")==0) {
    // {"msg":"updateSensorJson", "key":"voc_lin_m", "value":0.0001}
    if (_user_cmd.containsKey("key") && _user_cmd.containsKey("value")) {
        const char* key = _user_cmd["key"];
        // Update configuration parameter dynamically
    }
}
```

## Error Handling and Validation

### Negative Value Protection
```cpp
if (concentration < 0) { concentration = 0; }
if (S1 < 0) { S1 = 0; }
if (offset < 0) { offset = 0; }
```

### Division by Zero Protection
```cpp
if (fGain == 0) { return 0.0; }
if (_voc_lin_m != 0) { /* apply linear correction */ }
```

### Parameter Validation
```cpp
// Exponential function overflow protection
if (exp_input > 50) { exp_input = 50; }
if (exp_input < -50) { exp_input = -50; }
```

## Calibration Procedures

### Zero-Point Calibration
1. **Stabilization**: Allow sensor to reach thermal equilibrium
2. **Clean Air Exposure**: Expose sensor to zero-concentration environment
3. **Voltage Measurement**: Record baseline voltage reading
4. **Temperature Compensation**: Apply temperature differential correction
5. **Offset Storage**: Save corrected offset to EEPROM

### Span Calibration
1. **Known Concentration**: Expose sensor to certified reference gas
2. **Voltage Measurement**: Record sensor response voltage
3. **Gain Calculation**: Calculate gain = concentration / (voltage - offset)
4. **Validation**: Verify linearity across measurement range
5. **Parameter Storage**: Save gain to EEPROM and configuration

### Multi-Point Calibration
1. **Data Collection**: Measure sensor response at multiple known concentrations
2. **Curve Fitting**: Apply linear regression or exponential fitting
3. **Parameter Extraction**: Calculate slope, intercept, or exponential coefficients
4. **Validation**: Verify accuracy across full measurement range
5. **Implementation**: Store parameters and apply appropriate conversion function

## Conversion Chain Summary

### Standard Electrochemical Sensors
1. **Raw ADC Reading** → **Voltage Conversion**
2. **Temperature Compensation** → **Corrected Voltage**
3. **Peak Detection** (if enabled) → **Signal Extraction**
4. **Linear Conversion** → `gain × (voltage - offset)`
5. **Linear Correction** (if configured) → `m × result + b`
6. **Final Concentration**

### VOC Sensors
1. **Raw ADC Reading** → **Voltage/Count Conversion**
2. **Offset Subtraction** → **Corrected Signal**
3. **Gain Application** → **Preliminary Concentration**
4. **Exponential Correction** (if configured) → `A × e^(b×x) + C`
5. **Linear Correction** (if configured) → `m × x + b`
6. **Final Concentration**

### Infrared Sensors
1. **Raw ADC Reading** → **Voltage Conversion**
2. **Temperature Compensation** → **Corrected Voltage**
3. **Statistical Calibration** → **Regression-Based Conversion**
4. **Linear Correction** (if configured) → `m × result + b`
5. **Final Concentration**