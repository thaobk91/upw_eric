# Air Monitor System Architecture Analysis

## System Overview

The air monitoring system is built on an ESP32 Feather microcontroller with Weather Shield hardware, running FreeRTOS for task management. The system implements a multi-sensor environmental monitoring platform with LTE connectivity for data transmission via MQTT protocol.

### Hardware Platform
- **Microcontroller**: ESP32 Feather (dual-core processor)
- **Hardware Version**: Weather Shield (WS) versions 14 and others
- **Operating System**: FreeRTOS with task-based architecture
- **Storage**: SD card and SPIFFS (SPI Flash File System)
- **Communication**: LTE modem with GPS/GNSS capability

## FreeRTOS Task Architecture

The system implements a multi-task architecture using FreeRTOS with three primary tasks running on different CPU cores:

### Task 1: collectDataTask (Core 0, Priority 1)
**Purpose**: Sensor data collection and processing
**Stack Size**: 40,960 words
**Core Assignment**: Core 0
**Priority**: 1 (lower priority)

**Functionality**:
- Manages sampling intervals and sensor stabilization timing
- Controls pumping system for gas sensor sampling
- Collects data from all sensor modules
- Implements data averaging and concentration calculations
- Manages SUMMA canister state machine
- Handles sensor error detection and automatic reboots
- Records processed data to CSV format and storage

**Key Operations**:
```cpp
while (true) {
    // Wait for sampling to be enabled
    while (_sampling_status == false) { 
        _sampling_status_lock = true;
        taskDelay(10, 1);
    }
    
    // Control pumping system
    if ((_pumping_time != 0) && (_pumping_status == true)) { 
        setSampling_OFF(); 
    }
    
    // Wait for sensor stabilization
    taskDelay(_sampling_interval_sec * 1000, 1);
    
    // Collect sensor data
    collectSensors("==== collectDataTask ====");
    
    // Process and record data when sample count reached
    if (_sample_count >= _report_interval_count) {
        recordData();
        initCollect();
        taskDelay(_sleep_time_sec * 1000, 1);
    }
    
    // Resume pumping if needed
    if ((_pumping_time != 0) && (_pumping_status == false)) { 
        setSampling_ON(_pumping_time); 
    }
}
```

### Task 2: sendDataTask (Core 0, Priority 2)
**Purpose**: Data transmission and MQTT communication
**Stack Size**: 40,960 words
**Core Assignment**: Core 0
**Priority**: 2 (higher priority)

**Functionality**:
- Manages LTE modem connection and MQTT client
- Reads stored CSV data from log files using index system
- Converts CSV data to JSON format for MQTT transmission
- Implements transmission queue with error recovery
- Handles connection failures and automatic reconnection
- Manages OTA (Over-The-Air) update process
- Serves MQTT commands and responses

**Key Operations**:
```cpp
while (true) {
    setupConnection(); // Initialize modem and GPS
    
    while (true) {
        // Wait for data to be available
        while ((_delay_TX > 0) || (_csvWIdx < _csvRIdx + 2)) {
            if (_ota_status == false) {
                serveMQQT(); // Handle MQTT messages
            }
            taskDelay(100, 0);
            _delay_TX--;
        }
        
        // Fetch and transmit data
        _csvPointer = logIdx_Fetch(_csvRIdx);
        if (_csvPointer != -1) {
            if (log_Fetch(_csvPointer) != -1) {
                buildJSON();
                if (publish_data()) {
                    logIdx_Mark(_csvRIdx, 0); // Mark as sent
                    _csvRIdx++;
                    _err_TX = 0;
                } else {
                    _err_TX++;
                }
            }
        }
        
        // Handle transmission errors
        if (_err_TX >= 3) {
            closeMQTT();
            break;
        }
    }
}
```

### Task 3: serialModemProxyTask (Core 1, Priority 1)
**Purpose**: Serial communication proxy for modem
**Stack Size**: 4,096 words
**Core Assignment**: Core 1
**Priority**: 1

**Functionality**:
- Provides serial communication interface to LTE modem
- Handles AT command processing and responses
- Manages modem debugging and diagnostics

## Inter-Task Communication

### Shared Resources and Synchronization

**SD Card Mutex (`sd_mutex`)**:
- Protects shared access to SD card between tasks
- Prevents data corruption during concurrent file operations
- Used by both data collection and transmission tasks

**Global State Variables**:
- `_csvWIdx`: Write index for CSV data (updated by collectDataTask)
- `_csvRIdx`: Read index for CSV data (updated by sendDataTask)
- `_sampling_status`: Controls data collection enable/disable
- `_sampling_status_lock`: Prevents race conditions during sampling control
- `_ota_status`: Coordinates OTA updates between tasks

**Task Synchronization Mechanisms**:
```cpp
// SD card access protection
if (xSemaphoreTake(sd_mutex, portMAX_DELAY)) {
    // Perform SD card operations
    xSemaphoreGive(sd_mutex);
}

// Data availability signaling
while ((_csvWIdx < _csvRIdx + 2)) {
    // Wait for new data to be available
    taskDelay(100, 0);
}
```

## Hardware Initialization and Setup

### Main Setup Sequence (`setup()` function)

1. **System Initialization**:
   ```cpp
   Serial.begin(115200);
   taskDelay(10000,7); // Wait for serial and restore screen color
   ```

2. **Storage System Setup**:
   ```cpp
   init_SDmutex();     // Initialize SD card mutex
   mountFlash();       // Mount SPIFFS
   mountSD();          // Mount SD card
   ```

3. **Configuration Loading**:
   ```cpp
   if (readConfig() == 0) { 
       ColdBoot("Unable to read config.json"); 
   }
   ```

4. **Hardware Platform Setup**:
   ```cpp
   setupFeather();     // Initialize ESP32 Feather hardware
   taskDelay(10000,7); // Wait for sensors to boot up
   ```

5. **Calibration Data Management**:
   ```cpp
   if (_config_id == 1) {
       if (checkEEStamp() == 0) { writeEECalib(); }
       readEECalib();
   }
   ```

6. **Sensor System Initialization**:
   ```cpp
   setupSensors();     // Initialize all sensor modules
   ```

7. **SUMMA Canister System**:
   ```cpp
   if (_summa_enable) {
       summa_init();
   }
   ```

8. **Data Logging System**:
   ```cpp
   log_Create();       // Create CSV log file if not found
   rContext(0);        // Fetch logging task status
   ```

9. **Task Creation and Startup**:
   ```cpp
   xTaskCreatePinnedToCore(sendDataTask, "SendData", 40960, NULL, 2, NULL, 0);
   xTaskCreatePinnedToCore(collectDataTask, "CollectData", 40960, NULL, 1, NULL, 0);
   xTaskCreatePinnedToCore(serialModemProxyTask, "SerialModemProxy", 4096, NULL, 1, NULL, 1);
   ```

10. **Watchdog Timer Setup**:
    ```cpp
    esp_task_wdt_init(90, true); // 90 second timeout with panic
    ```

### Hardware Interface Architecture

**I2C Communication System**:
- TCA multiplexer for sensor isolation and addressing
- Multiple I2C devices on shared bus with address mapping
- Error detection and recovery for I2C communication failures

**Power Management**:
- Controlled power sequencing for sensors and modem
- Battery monitoring with low-voltage protection
- Sleep mode implementation for power conservation
- Temperature-based sensor protection

**GPIO Pin Management**:
- LED control for status indication
- Button interface for user interaction (10-second press for reset)
- Relay control for SUMMA canister operation
- Power control lines for various subsystems

## Main Loop Functionality

The main loop runs continuously and handles:

1. **User Interface Management**:
   - Button press detection and timing
   - LED status indication
   - System reset functionality (10-second button press)

2. **System Monitoring**:
   - Watchdog timer reset
   - Task health monitoring
   - Error condition detection

3. **Emergency Procedures**:
   - Database clearing on user command
   - System reboot coordination
   - Error logging and recovery

```cpp
void loop() {
    taskDelay(100, 0);
    
    // Handle user button press for system reset
    if (swNet_button()) {
        // 10-second press detection logic
        // Clear database and reboot if confirmed
    }
}
```

## System State Management

### Configuration Management
- JSON configuration stored on both SD card and SPIFFS
- Runtime configuration updates via MQTT commands
- Persistent state storage for system recovery

### Error Handling and Recovery
- Automatic sensor reboot on communication failures
- System reboot on excessive errors (limited to 3 consecutive reboots)
- Battery and temperature protection with automatic shutdown
- MQTT error reporting and remote diagnostics

### Data Integrity
- CSV data validation and bounds checking
- Index file consistency mechanisms
- Transmission status tracking and recovery
- Duplicate transmission prevention

## Task Timing and Scheduling

**Data Collection Timing**:
- `_sampling_interval_sec`: Time between sensor readings
- `_report_interval_count`: Number of samples before reporting
- `_sleep_time_sec`: Sleep duration after data recording

**Communication Timing**:
- 1-second delay between MQTT transmissions
- Automatic retry on transmission failures
- Connection timeout and recovery mechanisms

**System Protection Timing**:
- 90-second watchdog timeout
- 1-hour reboot delay for battery/temperature protection
- 30-second reboot delay for sensor errors

This architecture provides a robust, multi-tasking system capable of continuous environmental monitoring with reliable data transmission and comprehensive error recovery mechanisms.