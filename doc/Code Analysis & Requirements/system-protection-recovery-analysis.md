# System Protection and Recovery Analysis

## Overview
This document analyzes the system protection and recovery mechanisms implemented in the air monitoring system, including battery protection, temperature monitoring, system reboot conditions, and watchdog timer functionality.

## Battery Protection System

### Battery Monitoring Variables
- **_vbat**: Current battery voltage (float)
- **_state_off_charge**: State of charge percentage (float) 
- **_bat_change_rate**: Battery charge/discharge rate (float)
- **_batt_low_voltage**: Configurable low voltage threshold (float)

### Battery Protection Implementation
```cpp
// Location: src/main.cpp:316
if (_vbat <= _batt_low_voltage) { 
    goSleep("recordData() Battery < 3.4V, reboot in 1hr", 3600); 
}
```

### Battery Monitoring Process
1. **Data Collection** (src/sensors.cpp:429-433):
   ```cpp
   _vbat = batGauge.getVoltage();
   _state_off_charge = batGauge.getSOC();
   _bat_change_rate = batGauge.getChangeRate();
   ```

2. **Startup Protection** (src/sensors.cpp:281):
   ```cpp
   if (S1 <= _batt_low_voltage) { 
       goSleep("Battery too low, sleep 1hr", 3600); 
   }
   ```

3. **Configuration Storage**: Battery threshold stored in JSON config as `_batt_low_voltage`

## Temperature Monitoring and Protection

### CPU Temperature Monitoring
- **_cpu_temp**: Current CPU temperature in Celsius (float)
- **_cpu_temp_sum**: Accumulated temperature readings (float)
- **_cpu_temp_smp_count**: Sample count for averaging (unsigned short)

### Temperature Protection Implementation
```cpp
// Location: src/main.cpp:317
if (_cpu_temp >= 70) { 
    goSleep("recordData() CPU temp > 70c, reboot in 1hr", 3600); 
}
```

### Temperature Monitoring Process
1. **Data Collection** (src/THPSense.cpp:69-72):
   ```cpp
   S1 = readTempESP(); 
   _cpu_temp_sum += S1;
   _cpu_temp_smp_count++;
   _cpu_temp = _cpu_temp_sum / _cpu_temp_smp_count;
   ```

2. **Sensor Protection** (src/main.cpp:165-201):
   - Electrochemical sensor protection based on ambient temperature
   - Temperature thresholds: 38°C ON, 40°C OFF (standard sensors)
   - CAN sensors: 48°C ON, 50°C OFF
   - Currently commented out (version 4.26)

## System Reboot Conditions and Procedures

### goSleep Function (src/WSGlobal.cpp:338-362)
```cpp
void goSleep(String output, uint64_t sleepDelay) {
    Serial.println(output); 
    pauseCollect(); // stop collecting data
    
    if (sleepDelay == 0) { 
        ESP.restart(); 
    }
    
    Serial.printf("Move core to deep sleep: %luSec\r\n", sleepDelay); 
    sleepDelay = sleepDelay * 1000000; // convert sec to micro sec
    esp_sleep_enable_timer_wakeup(sleepDelay); 
    esp_deep_sleep_start();
    ESP.restart(); // Fallback restart
}
```

### Reboot Trigger Conditions

1. **Battery Protection**:
   - Low voltage: 1 hour delay before reboot
   - Critical startup voltage: Immediate sleep

2. **Temperature Protection**:
   - CPU temperature ≥ 70°C: 1 hour delay before reboot

3. **Sensor Error Protection**:
   ```cpp
   // Location: src/main.cpp:305-313
   if ((_err_loop >= 10) && ((_csvRIdx + 2) == _csvWIdx)) {
       _err_loop = 0;
       _ctxerr_count++; 
       if (_ctxerr_count > 3) { return; } 
       goSleep("recordData() Too many sensors error, reboot in 30sec", 30); 
   }
   ```

4. **Data Logging Failures**:
   ```cpp
   // Location: src/main.cpp:295
   goSleep("recordData() Failed to record csv, reboot in 30sec", 30);
   ```

5. **Configuration Errors**:
   ```cpp
   // Location: src/myGlobal.cpp:1743
   goSleep("readConfig() Invalid config.json, reboot in 30min", 60*30);
   ```

6. **User-Initiated Reset**:
   - 10-second button press triggers database clear and reboot
   - MQTT "reset" command with configurable delay

## Watchdog Timer and Task Monitoring

### Watchdog Timer Configuration
```cpp
// Location: src/main.cpp:556-558
esp_log_level_set("task_wdt", ESP_LOG_VERBOSE);
esp_task_wdt_init(90, true); // 90 sec timeout, panic if not reset
```

### Watchdog Reset Points
1. **Main Data Transmission Task** (src/main.cpp:330, 349):
   ```cpp
   esp_task_wdt_reset(); // At task start and in main loop
   ```

2. **LTE Communication** (src/myLTE.cpp):
   - MQTT connection attempts: Line 1330, 1335
   - Network registration: Line 1526
   - GPRS connection: Line 1535
   - LTE modem initialization: Line 1579, 1584, 1591, 1598

### Task Monitoring Strategy
- 90-second watchdog timeout
- Strategic reset points during long operations
- Panic mode enabled for system recovery
- Verbose logging for debugging

## Error Counting and Recovery Logic

### Error Counter Variables
- **_errors**: Binary error flags for different subsystems
- **_err_loop**: Consecutive error loop counter
- **_ctxerr_count**: Context error count (persistent across reboots)

### Error Classification (src/sensors.cpp:466-500)
```cpp
_errors = 0;
if (_ir_err) _errors |= 1;           // IR sensors
if (_mps_err) _errors |= 2;          // MPS sensors  
if (_voc_err) _errors |= 4;          // VOC sensors
if (_nh3_err) _errors |= 8;          // NH3 sensors
if (_pm_err > 0) _errors |= 16;      // PM sensors
if (_spec_err || _o3_err || _so2_err || _no2_err || _h2s_err) _errors |= 32; // SPEC sensors
if (_rtc_err) _errors |= 64;         // RTC
if (_bme_err) _errors |= 128;        // Environmental sensors
```

### Recovery Logic
1. **Incremental Error Counting**:
   ```cpp
   if ((_errors & 0xBD) > 0) { // All except RTC and MPS errors
       _err_loop += 1; 
   } else {
       _err_loop = 0;
   }
   ```

2. **Reboot Protection**: Maximum 3 consecutive reboots
3. **MQTT Error Reporting**: Errors published when count > 3
4. **Context Persistence**: Error counts saved to SD card

## Sensor-Specific Recovery

### MPS Sensor Recovery (src/MPSense.cpp:183-240)
```cpp
void rebootMPS(void) {
    static unsigned short _reboot_count; 
    
    if (_reboot_count >= 3) { 
        setMPS_OFF();
        _is_mps_on = false;
        _mps_err = true;
        return;
    }
    
    setMPS_OFF();
    taskDelay(500,1);
    Serial.printf("Try cycling the power to MPS _%u\r\n", _reboot_count);
    setMPS_ON();
    taskDelay(5000,1);
    _reboot_count++;
}
```

### PM Sensor Recovery (src/main.cpp:206)
```cpp
if ((_pm_err) || (_pm_enable && (_is_PM25_on == false))) { 
    rebootPM(); 
}
```

## System Recovery Mechanisms

### Deep Sleep Recovery
- Configurable sleep duration (seconds to microseconds conversion)
- Timer-based wakeup using ESP32 RTC
- Graceful shutdown of data collection
- I2C bus release before sleep

### Immediate Restart Conditions
- Configuration file corruption
- Critical system failures
- User-initiated reset commands
- OTA update completion

### Data Integrity Protection
- SD card mutex protection
- Index bounds checking with automatic reset
- Context saving before critical operations
- Recovery from invalid file pointers

## Configuration and Thresholds

### Configurable Parameters
- **_batt_low_voltage**: Battery protection threshold
- **CPU temperature**: Fixed 70°C threshold
- **Error loop threshold**: 10 consecutive errors
- **Context error limit**: 3 reboot attempts
- **Watchdog timeout**: 90 seconds

### Hardware-Specific Settings
- Different temperature thresholds for sensor versions
- Version-dependent error handling
- I2C address validation and recovery

## Monitoring and Diagnostics

### Status Reporting
- Battery voltage, SOC, and charge rate logging
- CPU temperature continuous monitoring
- Error state binary encoding
- Watchdog reset confirmation
- Recovery attempt counting

### Debug Information
- Serial output for all protection events
- MQTT error reporting with detailed status
- Context file persistence for post-reboot analysis
- Verbose watchdog logging

## Summary

The system implements comprehensive protection mechanisms:

1. **Battery Protection**: Voltage monitoring with configurable thresholds and delayed shutdown
2. **Temperature Protection**: CPU temperature monitoring with automatic thermal shutdown
3. **Watchdog Protection**: 90-second timeout with strategic reset points
4. **Error Recovery**: Graduated response from sensor restart to system reboot
5. **Data Integrity**: Mutex protection and bounds checking
6. **Persistent Monitoring**: Error counting across reboots with MQTT reporting

The protection system balances operational continuity with hardware safety, implementing multiple layers of defense against system failures while maintaining data collection capabilities.