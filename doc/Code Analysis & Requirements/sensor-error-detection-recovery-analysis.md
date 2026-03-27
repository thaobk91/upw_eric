# Sensor Error Detection and Recovery Analysis

## Overview

This document analyzes the error detection logic and automatic recovery procedures implemented for each sensor type in the air monitoring system. The system employs multiple layers of error detection and recovery mechanisms to ensure robust operation.

## Error Detection Variables

### Global Error Flags
Each sensor type maintains dedicated error flags:

**Electrochemical Sensors (ECSense.cpp/h):**
- `_h2s_err` - H2S sensor error flag
- `_o3_err` - O3 sensor error flag  
- `_so2_err` - SO2 sensor error flag
- `_no2_err` - NO2 sensor error flag
- `_nh3_err` - NH3 sensor error flag (WS version 14 only)
- `_spec_err` - General SPEC sensor communication error
- `_SPEC_i2c` - SPEC I2C communication status

**Infrared Sensors (IRSense.cpp/h):**
- `_ir_err` - General IR sensor error flag
- `_ir_i2c` - IR I2C communication status

**Environmental Sensors (THPSense.cpp/h):**
- `_bme_err` - BME280/BME680 sensor error flag
- `_grove_i2c` - Grove connector I2C status

**Particulate Matter Sensors (PMSense.cpp/h):**
- `_pm_err` - PM sensor error flag (unsigned short)
- `_pm_i2c` - PM I2C communication status
- `_is_PM25_on` - PM sensor operational status

**VOC Sensors (VOCSense.cpp/h):**
- `_voc_err` - VOC sensor error flag
- `_voc_i2c` - VOC I2C communication status

**System-Level Error Tracking:**
- `_errors` - Global error counter
- `_err_loop` - Loop error counter
- `_ctxerr_count` - Context error count for reboot management

## Error Detection Mechanisms

### 1. I2C Communication Errors

**ADC Reading Errors (SPEC Sensors):**
```cpp
// In SPEC_read() function
if (AD_ECErr != 0) {
    Serial.printf("wsAD_SPEC(%u) Error %u !\r\n", ch, AD_ECErr); 
    _spec_err = true;
}
```

**ADC Reading Errors (IR Sensors):**
```cpp
// In IR_read() function  
if (ADErr != 0) {
    Serial.printf("wsAD_IR(%u) Error %u !\r\n", ch, ADErr); 
    _ir_err = true;
}
```

### 2. Sensor Initialization Errors

**LMP91000 Potentiostat Detection:**
```cpp
// In pStat_Config() function
if (pStat.isReady()==false) { 
    Serial.printf("\r\npStat(%u) LMP91_IC%u not found  ", csbit, ChipID);
    bErr = _h2s_enable; // Only error if sensor is enabled
    return bErr;
}
```

**Device Presence Detection:**
```cpp
// TCA multiplexer error detection
if (( _tcaError & 0x02) == 0x02) { 
    Serial.println("ADC SPEC wsIC108 not found");
    return;
}
```

### 3. Data Validation Errors

**NaN Value Detection (BME sensors):**
```cpp
// In collectTPH() function
S1 = bme280.readTemperature();
if (!isnan(S1)) {   
    // Process valid data
} else {
    _bme_err = true; 
}
```

**Temperature Range Validation:**
```cpp
// Temperature protection logic
if (_temperature > 65) { 
    _bme_err = true;
    Serial.println("Warning: BME280 temperature too high, check the sensor");
}
```

### 4. Communication Timeout Errors

**PM Sensor Data Ready Check:**
```cpp
// In collectPM() function
pm_ack = sps30_read_data_ready(&pm_data_ready);
if ((pm_ack >= 0) && (pm_data_ready)) {
    // Process data
} else {
    Serial.printf("PM err data_ready:%u  ack:%u\r\n", pm_data_ready, pm_ack);
    _pm_err = 0;  
}
```

**VOC Sensor Communication:**
```cpp
// In sgp40LowPower() function
error = sgp40.measureRawSignal(compensationRh, compensationT, srawVoc);
if (error != 0) {
    sgp40PrintErr ("Err2 SGP40 reading: ", error);
    _voc_err = true;
    return;
}
```

## Automatic Recovery Procedures

### 1. Sensor Reboot Functions

**PM Sensor Auto-Reboot:**
```cpp
void rebootPM(void) {
    static unsigned short _reboot_count;
    
    if ((_pm_i2c) && (_pm_enable)) {   
        _pm_err = false;
        if ( _reboot_count == 3) { 
            setPM_OFF();
            Serial.println("Take PM off line");
        }
        if ( _reboot_count >= 3) { // Too many errors, drop it
            _reboot_count = 4;
            _is_PM25_on = false; 
            _pm_err = true;
            return;
        }
        
        taskDelay(500,1);
        Serial.printf("Try cycling the power to PM _%u\r\n", _reboot_count);
        setPM_ON();
        taskDelay(5000,1);
        _reboot_count++;
        
        if (sps30_probe() == 0) { 
            _pm_CycleTrigger = millis() + ( 1*60*1000); // Wait 1 min prior reading PM
            _is_PM25_on = true; 
            _reboot_count = 0;
            wakeupPM();
        }
    }
}
```

**MPS Sensor Auto-Reboot:**
```cpp
// Called from main.cpp recordData() function
if (( _mps_err) || ( _mps_enable && ( _is_mps_on == false))) { 
    rebootMPS(); 
}
```

### 2. Error Counting and Thresholds

**Loop Error Management:**
```cpp
// In main.cpp recordData() function
if ((_err_loop >= 10) && ((_csvRIdx + 2) == _csvWIdx)) { // Err & mqtt_tx ok
    _err_loop = 0;
    _ctxerr_count++; 
    wContext(false); // Save context to log.sta
    
    if (_ctxerr_count > 3) { return; } 
    goSleep("recordData() Too many sensors error, reboot in 30sec", 30); 
}
```

### 3. System Protection Mechanisms

**Temperature Protection (Commented Out in v4.26):**
```cpp
// Temperature-based sensor protection (disabled in current version)
if (_temperature > fTempEC_OFF) { 
    if (_spe_locked == false) { 
        pStat_Protect (5,3,true); // SO2 Protected OFF 
        pStat_Protect (6,0,true); // H2S 
        pStat_Protect (9,1,true); // NO2
        pStat_Protect (10,2,true); // O3
        if (_WS_version == 14) { pStat_Protect (16,4,true); } // NH3
        _spe_locked = true;
    }
}
```

**Battery Protection:**
```cpp
// In main.cpp recordData() function
if (_vbat <= _batt_low_voltage) { 
    goSleep("recordData() Battery < 3.4V, reboot in 1hr", 3600); 
}
```

**CPU Temperature Protection:**
```cpp
// In main.cpp recordData() function
if (_cpu_temp >= 70) { 
    goSleep("recordData() CPU temp > 70c, reboot in 1hr", 3600); 
}
```

### 4. Sensor-Specific Recovery Logic

**SPEC Sensor Configuration Recovery:**
```cpp
// In setupEC() function - reconfigure all sensors on startup
_h2s_err = pStat_Config(8,0,3,1,1,0,7);  // H2S configuration
_no2_err = pStat_Config(9,1,0,1,0,0,499); // NO2 configuration
_o3_err = pStat_Config(12,2,5,1,0,0,35);  // O3 configuration
_so2_err = pStat_Config(3,3,6,1,1,0,120); // SO2 configuration
```

**IR Sensor Boot Delay:**
```cpp
// In setupIR() function
irBootDelay = millis() + 90*1000; // Allow 90sec for IR to boot

// In CollectIR() function
if ( millis() >= irBootDelay) {
    // Normal operation
} else {
    Serial.print(F("IR booting ..."));
}
```

## Error Threshold Management

### 1. Reboot Conditions

**Multiple Error Threshold:**
- `_err_loop >= 10` - Triggers system reboot consideration
- `_ctxerr_count > 3` - Prevents excessive reboots
- MQTT transmission sync required before reboot

**Individual Sensor Thresholds:**
- PM sensor: 3 reboot attempts before taking offline
- MPS sensor: Similar retry logic with power cycling
- Communication errors: Immediate flagging with retry on next cycle

### 2. Error Recovery Strategies

**Graceful Degradation:**
- Sensors taken offline after multiple failures
- System continues operation with remaining sensors
- Error status reported via MQTT

**Power Cycling:**
- Physical power control for PM and MPS sensors
- Controlled delays for sensor stabilization
- Automatic retry with exponential backoff

**Communication Recovery:**
- I2C bus reset capabilities
- TCA multiplexer reset functionality
- Sensor reinitialization procedures

## Error Reporting and Logging

### 1. Serial Debug Output

All error conditions generate detailed serial output:
```cpp
Serial.printf("wsAD_SPEC(%u) Error %u !\r\n", ch, AD_ECErr);
Serial.println("Warning: BME280 temperature too high, check the sensor");
Serial.printf("PM err measurement, ack:%u\r\n", pm_ack);
```

### 2. MQTT Error Reporting

Error states are included in MQTT data transmission and can be queried via MQTT commands:
- `errorlog` command reports errors and resets error count
- `reset` command resets error counters
- Error flags included in regular data packets

### 3. Context Preservation

Error context is preserved across reboots:
```cpp
wContext(false); // Save context including error counts
rContext(0);     // Restore context on startup
```

## Key Findings

1. **Multi-layered Error Detection**: The system implements error detection at communication, validation, and operational levels.

2. **Automatic Recovery**: Most sensors have automatic reboot/recovery procedures with configurable retry limits.

3. **Graceful Degradation**: Failed sensors are taken offline while the system continues operating with remaining sensors.

4. **Error Persistence**: Error states and counts are preserved across system reboots for trend analysis.

5. **Temperature Protection**: Electrochemical sensors have temperature-based protection mechanisms (currently disabled).

6. **Resource Management**: Error thresholds prevent infinite retry loops and excessive power consumption.

7. **Diagnostic Capabilities**: Comprehensive error reporting enables remote diagnosis and troubleshooting.