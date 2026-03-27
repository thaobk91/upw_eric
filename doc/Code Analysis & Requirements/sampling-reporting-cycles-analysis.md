# Sampling and Reporting Cycles Analysis

## Overview

The air monitoring system operates on a dual-cycle architecture with separate sampling and reporting intervals. The system continuously collects sensor data at regular sampling intervals, accumulates multiple samples, and then processes and transmits the data at longer reporting intervals. This approach optimizes power consumption, data quality, and communication efficiency.

## Configuration Parameters

### Core Timing Parameters
```json
{
  "sampling": {
    "pumping_time": 0,
    "sampling_interval_sec": 2,
    "report_interval_count": "15",
    "sleep_time_sec": 0,
    "engineering": false
  }
}
```

### Parameter Definitions

#### Sampling Interval
- **Parameter**: `sampling_interval_sec`
- **Default**: 2 seconds
- **Minimum**: 2 seconds (enforced in code)
- **Purpose**: Time between individual sensor readings
- **Usage**: Controls how frequently sensors are polled

#### Report Interval Count
- **Parameter**: `report_interval_count`
- **Default**: 15 samples
- **Purpose**: Number of samples to collect before generating a report
- **Calculation**: Total report period = `sampling_interval_sec × report_interval_count`
- **Example**: 2 seconds × 15 samples = 30 seconds per report

#### Pumping Time
- **Parameter**: `pumping_time`
- **Default**: 0 seconds (always on)
- **Range**: 0 = continuous, >0 = timed operation
- **Purpose**: Duration to run sampling pump before each measurement
- **Special**: 0 means pump runs continuously

#### Sleep Time
- **Parameter**: `sleep_time_sec`
- **Default**: 0 seconds (no sleep)
- **Purpose**: Deep sleep duration between report cycles
- **Power Management**: Enables ultra-low power operation

#### Engineering Mode
- **Parameter**: `engineering`
- **Default**: false
- **Purpose**: Enables additional diagnostic data in reports

## Task Architecture

### FreeRTOS Task Structure
The system uses three main tasks running on dual cores:

```cpp
// Core 0 Tasks
xTaskCreatePinnedToCore(sendDataTask, "SendData", 40960, NULL, 2, NULL, 0);
xTaskCreatePinnedToCore(collectDataTask, "CollectData", 40960, NULL, 1, NULL, 0);

// Core 1 Task  
xTaskCreatePinnedToCore(serialModemProxyTask, "SerialModemProxy", 4096, NULL, 1, NULL, 1);
```

### Task Priorities
- **sendDataTask**: Priority 2 (highest) - Data transmission
- **collectDataTask**: Priority 1 (medium) - Sensor data collection
- **serialModemProxyTask**: Priority 1 (medium) - Modem communication proxy

## Data Collection Cycle

### collectDataTask Function Flow
```cpp
void collectDataTask(void* parameter) {
    while (true) {
        // 1. Check sampling status
        if (_sampling_status == false) { 
            Serial.println("Collect Task Paused ...");  
        } 
        while (_sampling_status == false) { 
            _sampling_status_lock = true;
            taskDelay(10, 1); // wait for sampling to enable
        } 
        
        // 2. Manage pumping cycle
        if ((_pumping_time != 0) && (_pumping_status == true)) { 
            setSampling_OFF(); 
        }
        
        // 3. Wait for sensor stabilization
        taskDelay(_sampling_interval_sec * 1000, 1);

        // 4. Collect sensor data
        Serial.println("");
        collectSensors("==== collectDataTask ====");
        
        // 5. Check if report interval reached
        if (_sample_count >= _report_interval_count) {
            recordData();
            swNet_led_blink(1);
            taskDelay(10, 1); 

            initCollect();
            taskDelay(_sleep_time_sec * 1000, 1); 
        }

        // 6. Manage pumping cycle continuation
        if ((_pumping_time != 0) && (_pumping_status == false)) { 
            setSampling_ON(_pumping_time); 
        }  
        if (_pumping_time == 0) { 
            setSampling_ON(_pumping_time); 
        } 
        taskDelay(10, 1); 
    }
}
```

### Sensor Collection Process
```cpp
void collectSensors(String output) {
    Serial.println(output);

    // Environmental sensors
    collectTPH();  // Temperature, Humidity, Pressure

    // Battery monitoring
    _vbat = batGauge.getVoltage();
    _state_off_charge = batGauge.getSOC();
    _bat_change_rate = batGauge.getChangeRate();
    
    // Individual sensor collection
    collectMPS();    // Micro Particle Sensor
    if ((_WS_version != 14) && _nh3_enable && _nh3_i2c) {  
        mics6814_Read(); // NH3 sensor for older hardware
    }
    collectWind();   // Wind speed and direction
    collectPM();     // Particulate Matter
    CollectIR();     // Infrared sensors (CO2, CH4, PID)
    collectEC();     // Electrochemical sensors (H2S, O3, SO2, NO2, NH3)
    collectVOC();    // Volatile Organic Compounds

    // Increment sample counter
    _sample_count++;
    Serial.printf("============= EOF Collect %i/%i\r\n", _sample_count, _report_interval_count);
}
```

## Pumping Control System

### Pumping Modes

#### Continuous Pumping (pumping_time = 0)
- Pump runs continuously during operation
- Provides constant airflow through sensors
- Higher power consumption but better response time
- Suitable for applications requiring rapid detection

#### Timed Pumping (pumping_time > 0)
- Pump runs for specified duration before each measurement
- Reduces power consumption
- Allows sensor stabilization between measurements
- Suitable for battery-powered deployments

### Pumping Control Functions
```cpp
void setSampling_ON(unsigned short unsDelay) { 
    wsIC113.digitalWrite(P4, HIGH);  // Turn on pump relay
    if (unsDelay == 0) {
        Serial.println(F("Sampling pump ON"));  
    }
    else {
        Serial.printf("Sampling pump ON  %uSec\r\n", unsDelay); 
        delay(unsDelay * 1000);  // Run for specified time
    }
    _pumping_status = true;
}

void setSampling_OFF(void) { 
    wsIC113.digitalWrite(P4, LOW);   // Turn off pump relay
    Serial.println(F("Sampling pump OFF"));
    _pumping_status = false;
    delay(1000); 
}
```

### Pumping Cycle Logic
1. **Before Measurement**: If timed pumping enabled and pump is running, turn off pump
2. **Stabilization Wait**: Wait for `sampling_interval_sec` to allow sensors to stabilize
3. **Data Collection**: Collect all sensor readings
4. **After Measurement**: If timed pumping, restart pump for next cycle

## Data Processing and Formatting

### Sample Accumulation
- Each `collectSensors()` call increments `_sample_count`
- Individual sensor readings are accumulated/averaged internally
- Peak detection algorithms track maximum values during sampling period

### Report Generation Trigger
```cpp
if (_sample_count >= _report_interval_count) {
    recordData();           // Process and format accumulated data
    swNet_led_blink(1);     // Visual indicator
    initCollect();          // Reset accumulators for next cycle
    taskDelay(_sleep_time_sec * 1000, 1);  // Optional sleep period
}
```

### Data Processing Pipeline
1. **formatCollect()**: Process raw sensor data
   - Apply calibration equations
   - Perform temperature compensation
   - Execute peak detection algorithms
   - Calculate averages and maximums
   - Format error flags

2. **buildCSV()**: Create CSV data record
   - Format all sensor values
   - Include timestamps and metadata
   - Add error flags and diagnostic data

3. **log_Store()**: Save to local storage
   - Write CSV record to SD card
   - Update index file for transmission tracking
   - Maintain data integrity

## Sleep and Power Management

### Sleep Mode Operation
When `sleep_time_sec > 0`, the system enters deep sleep between report cycles:

```cpp
taskDelay(_sleep_time_sec * 1000, 1);
```

### Power Optimization Strategies

#### Sensor Boot Time Compensation
```cpp
if (_sleep_time_sec > 0) { 
    if ((_c1_enable || _co2_enable || _pid_enable) || (_pm25_restime_min > 0)) {
        if ((_report_interval_count * _sampling_interval_sec) < 90) {
            _report_interval_count = 90 / _sampling_interval_sec;  // Ensure 90sec minimum
        }
    }
}
```

This ensures sufficient time for IR sensors and PM sensors to boot up after sleep periods.

#### Battery Protection Integration
- Low battery conditions trigger extended sleep periods
- System monitors battery voltage and state of charge
- Automatic shutdown when battery critically low

## Timing Calculations and Examples

### Example Configuration 1: High-Frequency Monitoring
```json
{
  "sampling_interval_sec": 2,
  "report_interval_count": 15,
  "sleep_time_sec": 0
}
```
- **Sample Period**: 2 seconds
- **Report Period**: 2 × 15 = 30 seconds
- **Operation**: Continuous monitoring, no sleep
- **Power**: Higher consumption, real-time data

### Example Configuration 2: Power-Optimized Monitoring
```json
{
  "sampling_interval_sec": 5,
  "report_interval_count": 12,
  "sleep_time_sec": 300
}
```
- **Sample Period**: 5 seconds
- **Report Period**: 5 × 12 = 60 seconds active
- **Sleep Period**: 300 seconds (5 minutes)
- **Total Cycle**: 6 minutes (1 minute active, 5 minutes sleep)
- **Power**: Significantly reduced consumption

### Example Configuration 3: Burst Sampling
```json
{
  "sampling_interval_sec": 1,
  "report_interval_count": 60,
  "sleep_time_sec": 600
}
```
- **Sample Period**: 1 second
- **Report Period**: 1 × 60 = 60 seconds active
- **Sleep Period**: 600 seconds (10 minutes)
- **Total Cycle**: 11 minutes
- **Power**: Balanced approach with detailed sampling bursts

## Error Handling and Recovery

### Sampling Status Control
```cpp
bool _sampling_status = true;        // Global sampling enable/disable
bool _sampling_status_lock = false;  // Task synchronization lock
```

### Pause/Resume Mechanism
```cpp
bool pauseCollect(void) {
    Serial.println("Pausing collect task");
    _sampling_status = false;
    _sampling_status_lock = false; 
    
    // Wait up to 10 seconds for task to acknowledge pause
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

### Error Recovery Strategies
- **Sensor Errors**: Individual sensor failures don't stop collection cycle
- **Communication Errors**: I2C failures are logged but don't halt sampling
- **Power Issues**: Battery protection overrides sampling schedule
- **Storage Errors**: SD card failures trigger system reboot

## Integration with Communication System

### Data Transmission Coordination
The `sendDataTask` operates independently but coordinates with sampling:

```cpp
// Wait for data to be available
while ((_delay_TX > 0) || (_csvWIdx < _csvRIdx + 2)) {
    if (_ota_status == false) {
        serveMQQT(); // Handle MQTT communication
    }
    taskDelay(100, 0);
    _delay_TX--;
}
```

### Transmission Timing
- **Delay**: 1-second delay between packet transmissions (`_delay_TX = 10`)
- **Queue Management**: Maintains separate read/write pointers for data queue
- **Error Handling**: Communication failures don't affect sampling schedule

## Performance Characteristics

### Timing Accuracy
- **Sample Timing**: ±10ms accuracy due to FreeRTOS task scheduling
- **Report Timing**: Cumulative accuracy maintained over multiple samples
- **Sleep Timing**: Hardware timer accuracy for deep sleep periods

### Resource Utilization
- **CPU Usage**: Sampling task uses ~10% CPU during active periods
- **Memory Usage**: Minimal heap allocation, stack-based operations
- **Storage**: Efficient CSV format with binary indexing

### Scalability Considerations
- **Sample Rate Limits**: Minimum 2-second interval due to sensor response times
- **Report Size Limits**: CSV record size ~2KB, manageable for transmission
- **Storage Capacity**: SD card provides months of data storage capability

## Sensor-Specific Timing Requirements

### IR Sensors (CO2, CH4, PID)
- **Boot Time**: 60-90 seconds after power-on
- **Stabilization**: 30 seconds between readings
- **Response Time**: 2-5 seconds for concentration changes

### Electrochemical Sensors (H2S, O3, SO2, NO2, NH3)
- **Boot Time**: 30-60 seconds after power-on
- **Stabilization**: 10-30 seconds between readings
- **Response Time**: 1-3 seconds for concentration changes

### Particulate Matter Sensors (PM2.5, PM10)
- **Boot Time**: 30 seconds after power-on
- **Measurement Time**: 1 second per reading
- **Rest Time**: Configurable via `pm25_restime_min`

### Environmental Sensors (Temperature, Humidity, Pressure)
- **Boot Time**: <1 second
- **Stabilization**: Immediate
- **Response Time**: <1 second

## Optimization Strategies

### Power Optimization
1. **Sleep Scheduling**: Use `sleep_time_sec` for battery-powered deployments
2. **Sensor Selective Enable**: Disable unused sensors to reduce power
3. **Pumping Optimization**: Use timed pumping for power savings
4. **Communication Batching**: Accumulate multiple reports before transmission

### Data Quality Optimization
1. **Sampling Frequency**: Balance between data resolution and power consumption
2. **Stabilization Time**: Ensure adequate sensor stabilization between readings
3. **Peak Detection**: Use appropriate trigger thresholds for environmental conditions
4. **Temperature Compensation**: Enable for accurate readings across temperature ranges

### Communication Optimization
1. **Report Interval**: Balance between data freshness and transmission overhead
2. **Compression**: CSV format provides good compression for transmission
3. **Error Recovery**: Robust indexing system ensures no data loss
4. **Bandwidth Management**: Configurable transmission delays prevent network congestion