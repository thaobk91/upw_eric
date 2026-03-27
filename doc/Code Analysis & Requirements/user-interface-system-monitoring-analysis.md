# User Interface and System Monitoring Analysis

## Overview

The air monitoring system provides a minimal but comprehensive user interface consisting of a physical button, LED indicator, and extensive remote monitoring capabilities through MQTT commands. The system emphasizes autonomous operation with robust diagnostic and control mechanisms accessible both locally and remotely.

## Physical User Interface

### Hardware Components

#### User Button
- **Location**: PCF8574 I2C expander at address 0x39, bit 0
- **Function**: System reset and database clearing
- **Type**: Momentary push button
- **Debouncing**: Software-based with timing validation

#### Status LED
- **Location**: PCF8574 I2C expander at address 0x39, bit 1
- **Function**: Visual status indication and user feedback
- **Control**: Software-controlled on/off and blinking patterns
- **Polarity**: Active low (LED on when bit cleared)

### Button Interface Implementation

#### Button Reading Function
```cpp
bool swNet_button(void) {
    byte button;
    Wire.requestFrom(0x039, 1);  
    if (Wire.available()) {
        button = Wire.read();  
        if ((button & 0x01) == 0) {  // Bit 0 low = pressed
            return true; 
        }
        return false;
    }
    Serial.println("User button not read ...");
    return false;
}
```

#### Button Press Detection Logic
The system implements a long-press detection mechanism for safety:

```cpp
void loop() {
    int i;
    bool bstate = false;
    
    if (swNet_button()) { // Button pressed
        bstate = true;
        swNet_led(1); // Turn on LED for feedback
        taskDelay(10, 3); 
        Serial.println("User button pressed ...");
        
        // Check for 10-second hold
        for (i = 0; i < 1000; i++) { 
            taskDelay(10, 3);  // 10ms delay, 1000 iterations = 10 seconds
            if (swNet_button() == false) {  // Button released early
                bstate = false;
                break; 
            } 
        }
        swNet_led(0); // Turn off LED
        
        if (bstate) { // Button held for full 10 seconds
            if (xSemaphoreTake(sd_mutex, portMAX_DELAY)) {
                swNet_led_blink(5); // Visual confirmation
                log_Clear(); // Clear database
                goSleep("User action validated. reboot", 0); // Immediate reboot
            }
        } 
        Serial.println("User action dismissed !");
    }
}
```

### LED Interface Implementation

#### LED Control Functions
```cpp
void swNet_led(bool led) { // 1 = ON, 0 = OFF
    if (led) {
        clearIO_39(1); // Clear bit 1 (active low)
    }
    else {
        setIO_39(1);   // Set bit 1 (LED off)
    }
} 

void swNet_led_blink(int j) {
    for (int i = 0; i < j; i++) {
        swNet_led(1); 
        taskDelay(500, 0); 
        swNet_led(0); 
        taskDelay(500, 0);
    }
}
```

#### LED Status Indicators
- **Solid On**: Button press acknowledgment
- **Single Blink**: Data report transmission completed
- **5 Blinks**: Database clearing confirmation
- **Off**: Normal operation

## System Monitoring

### CPU Temperature Monitoring

#### ESP32 Internal Temperature Sensor
```cpp
float readTempESP() {
    SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR, 3, SENS_FORCE_XPD_SAR_S);
    SET_PERI_REG_BITS(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_CLK_DIV, 10, SENS_TSENS_CLK_DIV_S);
    SET_PERI_REG_BITS(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_XPD_WAIT, 0x02, SENS_TSENS_XPD_WAIT_S);
    SET_PERI_REG_BITS(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_XPD_FORCE, 0x03, SENS_TSENS_XPD_FORCE_S);
    SET_PERI_REG_BITS(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_CLK_INV, 1, SENS_TSENS_CLK_INV_S);
    
    // Enable temperature sensor
    CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
    CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP_FORCE);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
    
    // Wait for sensor to stabilize
    ets_delay_us(100);
    
    // Read temperature
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
    ets_delay_us(5);
    
    float result = (GET_PERI_REG_BITS2(SENS_SAR_SLAVE_ADDR3_REG, SENS_TSENS_OUT, SENS_TSENS_OUT_S) - 32) * 5/9;
    return result;
}
```

#### Temperature Monitoring Variables
```cpp
float _cpu_temp;                    // Current averaged CPU temperature (°C)
float _cpu_temp_sum;               // Accumulated temperature sum
unsigned short _cpu_temp_smp_count; // Sample count for averaging
```

#### Temperature Collection Process
```cpp
void collectTPH() {
    float S1;
    
    // Read ESP32 internal temperature
    S1 = readTempESP(); 
    _cpu_temp_sum += S1;
    _cpu_temp_smp_count++;
    _cpu_temp = _cpu_temp_sum / _cpu_temp_smp_count;
    Serial.println("CPU Temp: " + String(_cpu_temp) + "°C");
}
```

#### Temperature Protection
```cpp
if (_cpu_temp >= 70) { 
    goSleep("recordData() CPU temp > 70c, reboot in 1hr", 3600); 
}
```

### Battery Monitoring

#### Battery Gauge Integration
```cpp
SFE_MAX1704X batGauge(MAX1704X_MAX17048); // MAX17048 fuel gauge

void collectSensors(String output) {
    // Battery monitoring
    _vbat = batGauge.getVoltage();
    _state_off_charge = batGauge.getSOC();
    _bat_change_rate = batGauge.getChangeRate();
    
    Serial.printf("Battery: %fV, SOC: %f, Charge Rate: %f\r\n", 
                  _vbat, _state_off_charge, _bat_change_rate);
}
```

#### Battery Protection
```cpp
bool checkBatLow(void) {
    float V1 = batGauge.getVoltage();
    if (V1 <= _batt_low_voltage) {
        Serial.print("Battery Voltage: ");
        Serial.print(V1);
        Serial.print(" - Low setpoint @ ");
        Serial.println(_batt_low_voltage);
        return true;
    }
    return false;
}

// In main data collection loop
if (_vbat <= _batt_low_voltage) { 
    goSleep("recordData() Battery < 3.4V, reboot in 1hr", 3600); 
}
```

### Error Monitoring and Reporting

#### Error Flag System
```cpp
void formatError(void) {
    _errors = 0;
    
    // Individual sensor error flags
    if (_ir_err) _errors |= 1;                    // IR sensors
    if (_mps_err) _errors |= 2;                   // MPS sensor
    if (_voc_err) _errors |= 4;                   // VOC sensors
    if (_nh3_err) _errors |= 8;                   // NH3 sensor
    if (_pm_err > 0) _errors |= 16;               // PM sensors
    if (_spec_err || _o3_err || _so2_err || _no2_err || _h2s_err) _errors |= 32; // EC sensors
    if (_rtc_err) _errors |= 64;                  // RTC
    if (_bme_err) _errors |= 128;                 // Environmental sensors
    
    // I2C communication error flags
    if ((_ir_i2c == false) && (_c1_enable || _co2_enable || _pid_enable)) _errors |= 1;
    if ((_mps_i2c == false) && (_mps_enable)) _errors |= 2;
    if ((_voc_i2c == false) && (_voc_enable || _voc_mox_enable)) _errors |= 4;
    if ((_nh3_i2c == false) && (_nh3_enable) && (_WS_version != 14)) _errors |= 8;
    if ((_pm_i2c == false) && (_pm_enable)) _errors |= 16;
    if ((_SPEC_i2c == false) && (_h2s_enable || _so2_enable || _no2_enable || _o3_enable)) _errors |= 32;
    if ((_grove_i2c == false) && (_is_bme280_on == false) && (_is_bme680_on == false)) _errors |= 128;
    
    // Error loop counter (excludes RTC and MPS errors)
    if ((_errors & 0xBD) > 0) { // Binary: 1011 1101
        _err_loop += 1; 
    }
    else {
        _err_loop = 0;
    }
    
    Serial.printf("Err#/Tca/Count/Ctx:%u/%u/%u/%u  rtc:%u  mps/cnt:%u/%u  ir:%u  voc:%u  nh3:%u  pm:%u  spec:%u  bme:%u\r\n",
                  _errors, _tcaError, _err_loop, _ctxerr_count, _rtc_err, _mps_err, _mps_err_count, 
                  _ir_err, _voc_err, _nh3_err, _pm_err, _spec_err, _bme_err);
}
```

#### Error Recovery Logic
```cpp
// In recordData() function
if ((_err_loop >= 10) && ((_csvRIdx + 2) == _csvWIdx)) { // Errors & MQTT sync OK
    _err_loop = 0;
    _ctxerr_count++; 
    wContext(false); // Save context to log.sta
    
    if (_ctxerr_count > 3) { return; } // Stop rebooting after 3 attempts
    goSleep("recordData() Too many sensors error, reboot in 30sec", 30); 
}
```

## Remote Monitoring and Control

### MQTT Command Interface

The system provides extensive remote control capabilities through MQTT commands:

#### System Control Commands
```cpp
// System reset with configurable delay
{"msg":"reset", "delay":30}

// Clear log database
{"msg":"rstlog"}

// Get system configuration
{"msg":"config"}

// Get error log
{"msg":"errorlog"}

// Engineering mode toggle
{"msg":"eemode", "command":1}

// Signal quality check
{"msg":"csq"}
```

#### Sensor Calibration Commands
```cpp
// Temperature calibration
{"msg":"caltemp", "offset":3.2}

// SPEC sensor configuration
{"msg":"configureSPEC", "model":0, "root":0, "sensitivity":"60", "enable":0}

// SPEC sensor zero calibration
{"msg":"calibrateSPEC", "model":0}

// SPEC peak detection calibration
{"msg":"calibSPECdetect", "model":5, "qty":10, "threshold":100, "tcomp":0.0}

// IR sensor configuration
{"msg":"configureIR", "model":0, "range":0, "gain":0, "enable":0}

// IR sensor zero calibration
{"msg":"calibIR", "model":0}

// IR peak detection calibration
{"msg":"calibIRDetect", "model":0, "qty":"10000", "threshold":0.001, "tcomp":1.0}

// NH3 sensor calibration
{"msg":"calibNH3"}
```

#### Configuration Update Commands
```cpp
// Generic sensor parameter update
{"msg":"updateSensorJson", "key":"voc_lin_m", "value":0.0001}

// GPS coordinate request
{"msg":"gps"}
```

#### Firmware Update Commands
```cpp
// OTA firmware update
{"msg":"update", "version":"blink"}

// Configuration file update
{"msg":"json", "name":"AM-6003"}
```

### Command Processing Flow
```cpp
void callback(char* topic, byte* payload, unsigned int length) {
    char scratch[1024];
    bool bAck = false;
    
    // Parse incoming message
    for (int i = 0; i < length; i++) {
        scratch[i] = ((char)payload[i]);
    }
    scratch[length] = 0;
    
    // Deserialize JSON command
    deserializeJson(_user_cmd, scratch);
    const char* command = _user_cmd["msg"];
    
    // Process command and set acknowledgment flag
    if (strcmp(command, "reset") == 0) { 
        wContext(true); // Clear error log
        uint64_t delay = _user_cmd["delay"];
        publish_ack(command, true);
        taskDelay(500, 2);  // Allow ACK to transmit
        goSleep("Reset app", delay);
        bAck = true; 
    }
    
    // Send acknowledgment if required
    if (bAck) {
        publish_ack(command, true);
    }
}
```

## System Reset and Recovery

### Database Clearing Function
```cpp
void log_Clear(void) {
    if (_sd_enable == false) { return; }

    if (SD.exists("/log.csv")) { 
        SD.remove("/log.csv"); // Delete the log file
        Serial.println("Database cleared");
    }
}
```

### System Sleep and Reboot
```cpp
void goSleep(String output, uint64_t sleepDelay) { // seconds before reboot
    Serial.println(output); 
    pauseCollect(); // Stop collecting data
    setSampling_OFF(); 
    sleepPM(); 
    sleepMPS();
    sleepVOC();
        
    modem_gnss_off(); // GPS OFF
    lte_OFF(true); // Wait for MQTT transmission completion
    shieldOFF(); // Power down sensors

    pinMode(22, INPUT_PULLUP);  // SCL pulled high
    pinMode(23, INPUT_PULLUP);  // SDA pulled high
    Serial.println("Release I2C"); 
    
    if (sleepDelay == 0) { ESP.restart(); } 
    Serial.printf("Move core to deep sleep: %luSec\r\n", sleepDelay); 
    sleepDelay = sleepDelay * 1000000; // Convert to microseconds
    esp_sleep_enable_timer_wakeup(sleepDelay); 
    esp_deep_sleep_start();
    
    Serial.println("EOF goSleep, restart"); // Not executed
    ESP.restart(); // Fallback restart
}
```

### Cold Boot Function
```cpp
void ColdBoot(String output) { 
    Serial.println(output);
    goSleep("ColdBoot (CPU reset)", 60); 
}
```

## Diagnostic and Status Reporting

### System Status Variables
```cpp
// System identification
extern const char* _device_id;
extern const char* _version;
extern unsigned short _WS_version;

// Runtime status
extern unsigned long _loop_counter;
extern bool _sampling_status;
extern bool _sampling_status_lock;

// Error tracking
extern unsigned short _errors;
extern unsigned short _err_loop;
extern unsigned short _ctxerr_count;
extern unsigned short _tcaError;

// Power status
extern float _vbat;
extern float _state_off_charge;
extern float _bat_change_rate;
extern float _batt_low_voltage;

// Temperature monitoring
extern float _cpu_temp;
extern bool _rtc_err;
```

### Status Reporting via MQTT
```cpp
// Battery and system status in JSON telemetry
{
  "device_id": "AM-6003",
  "timestamp": 1234567890,
  "loopcounter": {"value": 123},
  "cpu_temp": {"value": 45.6},
  "battery_voltage": {"value": 3.7},
  "battery_gauge": {"value": 85.2},
  "battery_crate": {"value": -0.1},
  "errors": {"value": 0}
}
```

### Serial Console Diagnostics
The system provides extensive serial console output for local diagnostics:

```cpp
// Sample collection status
Serial.printf("============= EOF Collect %i/%i\r\n", _sample_count, _report_interval_count);

// Battery status
Serial.printf("Battery: %fV, SOC: %f, Charge Rate: %f\r\n", _vbat, _state_off_charge, _bat_change_rate);

// CPU temperature
Serial.println("CPU Temp: " + String(_cpu_temp) + "°C");

// Error status
Serial.printf("Err#/Tca/Count/Ctx:%u/%u/%u/%u\r\n", _errors, _tcaError, _err_loop, _ctxerr_count);

// Individual sensor errors
Serial.printf("SPEC:%u  H2S:%u  O3:%u  SO2:%u  NO2:%u\r\n", _spec_err, _h2s_err, _o3_err, _so2_err, _no2_err);
```

## Watchdog and Task Monitoring

### ESP32 Task Watchdog
```cpp
esp_task_wdt_init(90, true); // 90-second timeout, panic if not reset

// In tasks that need monitoring
esp_task_wdt_reset(); // Reset watchdog timer
```

### Task Status Monitoring
```cpp
// Task synchronization for sampling control
bool pauseCollect(void) {
    Serial.println("Pausing collect task");
    _sampling_status = false; // Pause sampling 
    _sampling_status_lock = false; 
    
    // Wait up to 10 seconds for task acknowledgment
    for (int i = 0; i < 100; i++) {
        if (_sampling_status_lock == true) { break; }
        taskDelay(100, 3); 
    }
    
    if (_sampling_status_lock == false) { 
        Serial.println("Err, failed to pause collect task");
        _sampling_status = true; // Resume sampling 
        return false; 
    }
    return true; 
}
```

## User Interface Design Philosophy

### Minimal Physical Interface
- **Single Button**: Reduces complexity and potential failure points
- **Long Press Protection**: Prevents accidental database clearing
- **Visual Feedback**: LED provides immediate user feedback
- **Fail-Safe Design**: System continues operation even with UI hardware failure

### Comprehensive Remote Interface
- **MQTT Commands**: Full system control and monitoring
- **Real-Time Status**: Continuous telemetry transmission
- **Diagnostic Access**: Complete error and status information
- **Configuration Management**: Remote parameter updates
- **Firmware Updates**: OTA capability for maintenance

### Autonomous Operation Priority
- **Self-Monitoring**: Automatic error detection and recovery
- **Self-Healing**: Automatic sensor reboots and system recovery
- **Persistent Logging**: Maintains operation history across power cycles
- **Graceful Degradation**: Continues operation with partial sensor failures

This design ensures reliable operation in remote deployments while providing comprehensive monitoring and control capabilities for maintenance and troubleshooting.