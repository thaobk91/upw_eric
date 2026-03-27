# Temperature Compensation Analysis

## Overview

The air monitoring system implements comprehensive temperature compensation across all sensor types to correct for temperature-induced drift in sensor readings. This compensation is critical for maintaining measurement accuracy across varying environmental conditions.

## Core Temperature Compensation Formula

### Universal Compensation Equation
```cpp
corrected_voltage = raw_voltage - ((current_temperature - reference_temperature) * temperature_coefficient)
```

### Implementation in Peak Detection
```cpp
*_volt_corr = *_volt_max - ((_temperature - _temperature_mem) * *_tcomp);
if (*_volt_corr < 0) { *_volt_corr = 0; }
```

### Implementation in Sensor Calibration
```cpp
offset = ADvolt - (dxtemp * tcomp);
if (offset < 0) { offset = 0; }
```

Where:
- `dxtemp = _temperature - _temperature_mem` (temperature differential)
- `tcomp` = sensor-specific temperature compensation coefficient
- `_temperature_mem` = reference temperature (typically 20°C)

## Temperature Memory and Reference System

### Global Temperature Variables
```cpp
float _temperature_mem;      // Reference temperature for compensation calculations
float _temperature_dx;       // Temperature differential (current - reference)
float _temperature_offset;   // User-configurable temperature offset correction
```

### Temperature Memory Management
- **Reference Temperature**: Fixed at 20.0°C in EEPROM initialization
- **Current Temperature**: Read from BME280/BME680 environmental sensor
- **Temperature Differential**: Calculated as `current_temp - reference_temp`
- **Temperature Offset**: User-adjustable calibration offset applied to readings

### EEPROM Storage (Page 9)
```cpp
sprintf(_EEpagebuffer, "TEMP %f %f %f", _temperature_mem, _temperature_dx, _temperature_offset);
```

Format: `"TEMP <reference_temp> <temp_differential> <temp_offset>"`

## Sensor-Specific Temperature Compensation

### Electrochemical Sensors (H2S, O3, SO2, NO2, NH3)

#### Compensation Coefficients (EEPROM Storage)
- **H2S**: `_h2s_tcomp` (EEPROM page 5)
- **O3**: `_o3_tcomp` (EEPROM page 8)
- **SO2**: `_so2_tcomp` (EEPROM page 7)
- **NO2**: `_no2_tcomp` (EEPROM page 6)
- **NH3**: `_nh3_tcomp` (EEPROM page 4)

#### Zero Calibration with Temperature Compensation
```cpp
bool zeroSPEC(byte model) {
    float offset, tcomp, dxtemp;
    dxtemp = _temperature - _temperature_mem;
    
    switch (model) {
        case 0: // H2S
            tcomp = _h2s_tcomp;
            offset = AD_ECvolt - (dxtemp * tcomp);
            if (offset < 0) { offset = 0; }
            _h2s_offset_volt = offset;
            break;
        // Similar for other EC sensors...
    }
}
```

### Infrared Sensors (CO2, C1, PID)

#### Compensation Coefficients (EEPROM Storage)
- **CO2**: `_co2_tcomp` (EEPROM page 2)
- **C1**: `_c1_tcomp` (EEPROM page 1)
- **PID**: `_pid_tcomp` (EEPROM page 3)

#### Zero Calibration Implementation
```cpp
bool zeroIR(byte model) {
    float offset, tcomp, dxtemp;
    dxtemp = _temperature - _temperature_mem;
    
    switch (model) {
        case 1: // CO2
            tcomp = _co2_tcomp;
            offset = ADvolt - (dxtemp * tcomp);
            if (offset < 0) { offset = 0; }
            _co2_offset_volt = offset;
            break;
        // Similar for C1 and PID sensors...
    }
}
```

### VOC Sensors

#### Compensation Coefficient
- **VOC**: `_voc_tcomp` (EEPROM page 10)

#### Special VOC Compensation
```cpp
// In peak detection for VOC:
S1 = _voc_tcomp; // Temperature compensation coefficient
peakDetect("voc", &_voc_corr, &_voc_raw_max, &_voc_signal, &_voc_base, 
           &_voc_top, &_voc_mem, &S1, &_voc_dx, &_voc_dx_trig, &_voc_found);
```

#### VOC Range Validation
```cpp
if ((_bme_err == false) && (_temperature > -40) && (_temperature < 80) && 
    (_humidity > 0) && (_humidity < 100)) {
    sgp40LowPower(_temperature, _humidity);
}
```

### Environmental Sensors (BME280/BME680)

#### Temperature Offset Application
```cpp
_temperature = (_temperature_sum / _temp_smp_count) + _temperature_offset;
```

#### User Calibration Function
```cpp
void calibTemp(float offset) {
    _temperature_offset = offset;
    saveTempConfig();
}
```

## Temperature Protection and Error Handling

### High Temperature Protection
```cpp
if (_temperature > 65) { // 65°C threshold
    _bme_err = true;
    Serial.println("Warning: BME temperature too high, check the sensor");
}
```

### CPU Temperature Protection
```cpp
if (_cpu_temp >= 70) { 
    goSleep("recordData() CPU temp > 70c, reboot in 1hr", 3600); 
}
```

### Historical Temperature Protection (Commented Out)
```cpp
// 4.26 commented out temperature protect
/*
if (_temperature > fTempEC_OFF) { 
    if (_spe_locked == false) { 
        pStat_Protect(5,3,true); // SO2 Protected OFF 
    }
}
*/
```

## EEPROM Calibration Data Format

### Individual Sensor Calibration
```cpp
// Format: "SENSOR_NAME enable range gain offset_volt tcomp dx_trig"
sprintf(_EEpagebuffer, "H2S %u %f %f %f %f %f", 
        _h2s_enable, fRange, _h2s_gain, _h2s_offset_volt, _h2s_tcomp, _h2s_dx_trig);
```

### Temperature Configuration
```cpp
// Format: "TEMP temperature_mem temperature_dx temperature_offset"
sprintf(_EEpagebuffer, "TEMP %f %f %f", 
        _temperature_mem, _temperature_dx, _temperature_offset);
```

### Calibration Data Parsing
```cpp
int scanCalib(char *_name, bool *_enable, float *_range, float *_gain, 
              float *_offset, float *_tComp, float *_dxTrigger) {
    // Parse EEPROM string and extract calibration parameters
    *_tComp = atof(strings[5]); // Temperature compensation coefficient
    return index;
}

int scanTcomp(char *_name, float *_tmem, float *_tdx, float *_toff) {
    // Parse temperature configuration string
    *_tmem = atof(strings[1]); // Temperature memory (reference)
    *_tdx = atof(strings[2]);  // Temperature differential
    *_toff = atof(strings[3]); // Temperature offset
    return index;
}
```

## Configuration Integration

### JSON Configuration Loading
```cpp
// Restore sensor global variables from configuration
_temperature_mem = _config_flash["hardware"]["sensors"]["cal_temp"]; // Default: 20.0
_temperature_offset = _config_flash["hardware"]["sensors"]["temp_offset"];
```

### Configuration Saving
```cpp
void saveTempConfig(void) {
    _config_flash["hardware"]["sensors"]["temp_offset"] = _temperature_offset;
    writeConfig();
    writeEECalib();
}
```

## Temperature Compensation Characteristics

### Linear Compensation Model
- **Assumption**: Linear relationship between temperature and sensor drift
- **Formula**: `drift = temperature_change × compensation_coefficient`
- **Direction**: Subtractive compensation (removes temperature-induced positive drift)

### Coefficient Interpretation
- **Positive Coefficient**: Sensor reading increases with temperature
- **Negative Coefficient**: Sensor reading decreases with temperature
- **Zero Coefficient**: No temperature compensation applied

### Compensation Accuracy
- **Reference Point**: 20°C (standard laboratory temperature)
- **Range**: Effective across typical environmental temperature range
- **Limitations**: Linear model may not capture non-linear temperature effects

## Error Prevention Mechanisms

### Negative Value Protection
```cpp
if (offset < 0) { offset = 0; }
if (*_volt_corr < 0) { *_volt_corr = 0; }
```
- Prevents negative corrected values that could cause calculation errors
- Ensures physical meaningfulness of sensor readings

### Temperature Range Validation
- **Environmental Sensors**: -40°C to +80°C operational range
- **System Protection**: >65°C triggers error state
- **CPU Protection**: >70°C triggers system sleep

### Sensor Error Integration
- Temperature compensation disabled when sensor errors detected
- Error flags prevent invalid compensation calculations
- System recovery procedures reset compensation parameters

## Calibration Procedures

### Zero-Point Calibration
1. **Stabilization**: Allow sensors to reach thermal equilibrium
2. **Reference Measurement**: Record temperature at calibration time
3. **Baseline Establishment**: Measure sensor output in clean air
4. **Compensation Calculation**: Apply temperature differential correction
5. **Offset Storage**: Save corrected baseline to EEPROM

### Temperature Coefficient Determination
1. **Multi-Point Measurement**: Test sensor at different temperatures
2. **Linear Regression**: Calculate temperature coefficient from data
3. **Validation**: Verify compensation accuracy across temperature range
4. **Storage**: Save coefficient to appropriate EEPROM page

### User Calibration
- **Temperature Offset**: User-adjustable for field calibration
- **Real-Time Application**: Immediately applied to temperature readings
- **Persistent Storage**: Saved to both JSON config and EEPROM