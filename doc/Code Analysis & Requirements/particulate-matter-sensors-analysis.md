# Particulate Matter Sensors Analysis

## Overview

The air monitoring system includes a Sensirion SPS30 particulate matter sensor for measuring PM1.0, PM2.5, and PM10 concentrations. The sensor implements sophisticated power management with wake/sleep cycles and includes built-in AQI (Air Quality Index) calculation based on PM2.5 measurements.

## Hardware Architecture

### I2C Configuration

**I2C Address:**
- 0x69: SPS30 Particulate Matter Sensor

**Power Control:**
- Controlled via I/O expander for power cycling
- Supports automatic reboot on communication failures

### SPS30 Sensor Specifications

The SPS30 is a laser-based particulate matter sensor that measures:
- **PM1.0**: Particles ≤ 1.0 μm diameter
- **PM2.5**: Particles ≤ 2.5 μm diameter  
- **PM4.0**: Particles ≤ 4.0 μm diameter (available but not used)
- **PM10**: Particles ≤ 10 μm diameter

## Power Management and Sleep Cycles

### Wake/Sleep State Control

The system implements intelligent power management to extend sensor life:

```cpp
void wakeupPM(void) {
    if (_pm_enable) {
        sps30_start_measurement();
        _is_PM25_awake = true;
        Serial.println("PM wakes up");
    }
}

void sleepPM(void) {
    if (_pm_enable) {
        sps30_stop_measurement();
        _is_PM25_awake = false;
        Serial.println("PM goes to sleep");
    }
}
```

### Cycle Timing Management

**Boot Delay:**
- 1-minute initialization delay after power-on
- Allows sensor stabilization before measurements

**Measurement Cycle:**
- Configurable rest time between measurement cycles
- `_pm25_restime_min`: Rest time in minutes (configurable)
- `_pm_CycleTrigger`: Timestamp for next measurement cycle

### Power State Variables

```cpp
bool _pm_enable;        // Master enable/disable flag
bool _is_PM25_on;       // Sensor power state
bool _is_PM25_awake;    // Measurement active state
bool _pm_i2c;           // I2C communication status
unsigned short _pm_err; // Error status flag
```

## Data Collection Process

### Measurement Sequence

The `collectPM()` function implements the following measurement cycle:

1. **Timing Check**: Verify measurement cycle timing
2. **Data Ready Check**: Query sensor for available data
3. **Data Reading**: Retrieve measurement values
4. **Data Accumulation**: Add to running totals
5. **Error Handling**: Manage communication failures

### Data Collection Implementation

```cpp
void collectPM(void) {
    if (_is_PM25_on) {
        if (millis() >= _pm_CycleTrigger) {
            if (_is_PM25_awake) {
                // Check if data is ready
                pm_ack = sps30_read_data_ready(&pm_data_ready);
                
                if ((pm_ack >= 0) && (pm_data_ready)) {
                    // Read measurement data
                    pm_ack = sps30_read_measurement(&pm_data);
                    
                    if (pm_ack >= 0) {
                        // Process successful reading
                        _pm1 += pm_data.mc_1p0;
                        _pm2_5 += pm_data.mc_2p5;
                        _pm10 += pm_data.mc_10p0;
                        _pmAQI += pm25_AQI(pm_data.mc_2p5);
                        _pm_sample_count++;
                    }
                }
            } else {
                // Wake up sensor for measurement
                wakeupPM();
                _pm_CycleTrigger = millis() + (1*60*1000); // 1min ON
            }
        }
    }
}
```

## AQI Calculation Algorithm

### PM2.5 AQI Breakpoints

The system implements EPA AQI calculation using standard breakpoints:

```cpp
record_AQIbreakpoint pm25AQIbreakpoints[] = {
    {0.0f, 0},      // Bottom line
    {12.0f, 50},    // Good
    {35.4f, 100},   // Moderate
    {55.4f, 150},   // Unhealthy for sensitive groups
    {150.4f, 200},  // Unhealthy
    {250.4f, 300},  // Very unhealthy
    {320.4f, 400},  // Hazardous
    {500.4f, 500}   // Hazardous (max)
};
```

### AQI Calculation Formula

```cpp
float pm25_AQI(float value) {
    if (value >= 500) { value = 500; }
    
    // Find appropriate breakpoint range
    for (i = 1; i <= 7; i++) {
        ch = pm25AQIbreakpoints[i].c;
        if (value < ch) {
            cl = pm25AQIbreakpoints[i - 1].c;
            ih = pm25AQIbreakpoints[i].i;
            il = pm25AQIbreakpoints[i - 1].i;
            
            // Linear interpolation between breakpoints
            return (value - cl) / (ch - cl) * (ih - il) + il;
        }
    }
    return 500; // Maximum AQI
}
```

### AQI Categories

- **0-50**: Good (Green)
- **51-100**: Moderate (Yellow)
- **101-150**: Unhealthy for Sensitive Groups (Orange)
- **151-200**: Unhealthy (Red)
- **201-300**: Very Unhealthy (Purple)
- **301-500**: Hazardous (Maroon)

## Data Processing and Averaging

### Data Variables

```cpp
float _pm1;                    // PM1.0 concentration (μg/m³)
float _pm2_5;                  // PM2.5 concentration (μg/m³)
float _pm10;                   // PM10 concentration (μg/m³)
float _pmAQI;                  // Air Quality Index based on PM2.5
unsigned short _pm_sample_count; // Number of accumulated samples
```

### Data Formatting

The `formatPM()` function calculates averages from accumulated samples:

```cpp
void formatPM(void) {
    if (_pm_sample_count > 0) {
        _pm1 = _pm1 / _pm_sample_count;
        _pm2_5 = _pm2_5 / _pm_sample_count;
        _pm10 = _pm10 / _pm_sample_count;
        _pmAQI = _pmAQI / _pm_sample_count;
    } else {
        // No samples collected
        _pm1 = 0;
        _pm2_5 = 0;
        _pm10 = 0;
        _pmAQI = 0;
    }
}
```

### Data Clearing

```cpp
void clearPM() {
    _pm1 = 0;
    _pm2_5 = 0;
    _pm10 = 0;
    _pmAQI = 0;
    _pm_sample_count = 0;
    _pm_err = 0;
}
```

## Error Handling and Recovery

### Automatic Reboot System

The system implements automatic sensor recovery on communication failures:

```cpp
void rebootPM(void) {
    static unsigned short _reboot_count;
    
    if ((_pm_i2c) && (_pm_enable)) {
        _pm_err = false;
        
        if (_reboot_count == 3) {
            setPM_OFF();
            Serial.println("Take PM off line");
        }
        
        if (_reboot_count >= 3) {
            // Too many errors, disable sensor
            _reboot_count = 4;
            _is_PM25_on = false;
            _pm_err = true;
            return;
        }
        
        // Attempt power cycle recovery
        taskDelay(500, 1);
        Serial.printf("Try cycling the power to PM _%u\r\n", _reboot_count);
        setPM_ON();
        taskDelay(5000, 1);
        _reboot_count++;
        
        if (sps30_probe() == 0) {
            // Recovery successful
            _pm_CycleTrigger = millis() + (1*60*1000);
            _is_PM25_on = true;
            _reboot_count = 0;
            wakeupPM();
        }
    }
}
```

### Error Recovery Strategy

1. **First Failure**: Attempt immediate recovery
2. **Second Failure**: Power cycle and retry
3. **Third Failure**: Take sensor offline temporarily
4. **Persistent Failures**: Disable sensor permanently

### Communication Error Detection

```cpp
// Data ready check failure
if ((pm_ack < 0) || (!pm_data_ready)) {
    Serial.printf("PM err data_ready:%u  ack:%u\r\n", pm_data_ready, pm_ack);
    _pm_err = 0; // Non-critical error
}

// Measurement read failure
if (pm_ack < 0) {
    Serial.printf("PM err measurement, ack:%u\r\n", pm_ack);
    _pm_err = 1; // Critical error
}
```

## Sensor Initialization

### Setup Process

```cpp
void setupPM(void) {
    _pm_i2c = false;
    _pm_CycleTrigger = 0;
    clearPM();
    
    if (_pm_enable == false) {
        Serial.println("PM Sensor is disabled");
        return;
    }
    
    // Check I2C communication
    if ((_tcaError & 0x010) == 0x010) {
        Serial.println("PM Sensor not found");
        setPM_OFF();
        return;
    }
    _pm_i2c = true;
    
    // Probe sensor communication
    if (sps30_probe() == 0) {
        Serial.println("PM sensor Found");
        _pm_CycleTrigger = millis() + (1*60*1000); // 1min boot delay
        _is_PM25_on = true;
        wakeupPM();
    } else {
        Serial.println("PM Sensor fails begin");
    }
}
```

### Initialization Sequence

1. **Enable Check**: Verify sensor is enabled in configuration
2. **I2C Probe**: Test communication with sensor
3. **Power Management**: Initialize power control
4. **Boot Delay**: Set initial measurement delay
5. **Wake Sensor**: Start measurement mode

## Data Output Format

### Measurement Units
- **PM Concentrations**: Micrograms per cubic meter (μg/m³)
- **AQI**: Dimensionless index (0-500 scale)

### Serial Output Format

```
PM1.0 12, PM2.5 25, PM10.0 35, AQI2.5 75, PMcycle:5
```

### Data Structure

The SPS30 provides additional data not currently used:
- `pm_data.mc_4p0`: PM4.0 concentration
- Number concentrations for different particle sizes
- Typical particle size measurements

## Timing and Cycle Management

### Measurement Timing

**Boot Delay**: 1 minute after power-on
```cpp
_pm_CycleTrigger = millis() + (1*60*1000);
```

**Configurable Rest Time**: Between measurement cycles
```cpp
_pm_CycleTrigger = millis() + (_pm25_restime_min*60*1000);
```

### Millis() Overflow Handling

The system accounts for millis() overflow after 50 days:
```cpp
if (millis() >= _pm_CycleTrigger) {
    // Safe comparison handles overflow
}
```

## Power Optimization

### Sleep Mode Benefits
- Extends sensor laser diode life
- Reduces power consumption
- Prevents sensor overheating
- Maintains measurement accuracy

### Wake/Sleep Coordination
- Sensor sleeps when main system sleeps
- Automatic wake-up for scheduled measurements
- Coordinated with overall system power management

The particulate matter sensor system provides accurate PM measurements with intelligent power management, robust error recovery, and standardized AQI calculations for comprehensive air quality monitoring.