# Wind and MPS Sensors Analysis

## Overview

The air monitoring system includes wind measurement sensors and a Molecular Property Spectrometer (MPS) for methane detection. The wind system supports both traditional mechanical wind sensors and ultrasonic wind sensors, while the MPS provides precise methane concentration measurements through I2C communication with an ATtiny microcontroller.

## Wind Sensor Systems

### Hardware Architecture

**Sensor Types:**
1. **Ultrasonic Wind Sensor (Calypso)**: I2C address 0x15
2. **Mechanical Wind Sensor**: I2C address 0x38 (PCF8574 I/O expander)

**Wind Direction Measurement:**
- ESP32 ADC pin A2_I34 (GPIO 34)
- Analog voltage input: 0-1.5V (via resistor divider)
- Resolution: 22.5° increments

### Auto-Detection Priority

The system automatically detects available wind sensors with priority:

```cpp
void setupWind() {
    if (IsConnected(0x15)) {
        // Ultrasonic wind sensor found - disable mechanical sensor
        _wind_enable = false;
        _ultrawind_enable = true;
        Serial.printf("Found Sonic Wind, En:%u  TCycle: %lusec\r\n", _ultrawind_enable, _wind_interval_sec);
        return;
    }
    
    // Try mechanical wind sensor if ultrasonic not found
    if (_wind_enable && IsConnected(0x38)) {
        Serial.printf("Found Wind sensor, En:%u  TCycle: %lusec\r\n", _wind_enable, _wind_interval_sec);
        // Configure PCF8574 pins as inputs
        for (int i = 0; i < 8; i++) {
            fbIC7.pinMode(i, INPUT);
        }
    }
}
```

### Ultrasonic Wind Sensor (Calypso)

#### Communication Protocol

The ultrasonic sensor uses a 7-byte I2C protocol:

```cpp
void fetchUltraWind(void) {
    Wire.beginTransmission(0x15);
    Wire.write(0x10); // Register address
    Wire.endTransmission();
    
    Wire.requestFrom(0x15, 7); // Request 7 bytes
    if (Wire.available() == 7) {
        uint8_t unused1 = Wire.read(); // Unused byte
        uint8_t unused2 = Wire.read(); // Unused byte
        
        unsigned int windSpeed = Wire.read() << 8 | Wire.read(); // 16-bit wind speed
        unsigned int windDirection = Wire.read() << 8 | Wire.read(); // 16-bit direction
        uint8_t checksum = Wire.read(); // Checksum validation
        
        // Convert from sensor units
        windSpeed = windSpeed / 100;     // Scale to m/s
        windDirection = windDirection / 100; // Scale to degrees
        
        _wind_speed += windSpeed;
        _wind_dir += windDirection;
        _wind_sample_count++;
    }
}
```

#### Data Format
- **Wind Speed**: 16-bit value, divided by 100 for m/s
- **Wind Direction**: 16-bit value, divided by 100 for degrees
- **Range**: 0-360° for direction, variable speed range
- **Accuracy**: High precision due to ultrasonic measurement

### Mechanical Wind Sensor

#### Speed Measurement

Uses pulse counting via PCF8574 I/O expander:

```cpp
void fetchWind(void) {
    unsigned short samplingTime = 2000; // 2 second sampling
    
    // Read pulse count at start and end of sampling period
    byte windCount1 = fbIC7.digitalReadAll();
    taskDelay(2000, 1); // 2 second delay
    byte windCount2 = fbIC7.digitalReadAll();
    
    // Calculate pulse difference (handle overflow)
    unsigned short windCount = windCount2 - windCount1;
    if (windCount1 > windCount2) {
        windCount = 255 - windCount1 + windCount2;
    }
    
    // Convert to wind speed
    unsigned short windSpeed = windCount * (2.25 / (samplingTime / 1000)); // mph
    windSpeed = mphToMps(windSpeed); // Convert to m/s
    
    _wind_speed += windSpeed;
    _wind_sample_count++;
}
```

#### Speed Conversion Formula

```cpp
float mphToMps(float mph) {
    return mph * 0.44704; // Convert mph to m/s
}
```

**Specifications:**
- **Resolution**: 1.125 mph minimum (0.5 m/s)
- **Range**: 0-199 mph (0-90 m/s)
- **Sampling Time**: 2 seconds for accuracy
- **Pulse Rate**: 2.25 mph per pulse per second

#### Direction Measurement

Uses analog voltage measurement:

```cpp
// Wind direction measurement
int16_t adCount = analogRead(WINDIR_PIN); // GPIO 34, 0-2047 counts
float voltage = (adCount * 1.5) / 2047;   // Convert to voltage (0-1.5V)
unsigned short windDirection = (voltage * 359) / 1.5; // Convert to degrees

_wind_dir += windDirection;
```

**Direction Specifications:**
- **Range**: 0-360° (0° = North, 180° = South)
- **Resolution**: 22.5° increments
- **Input Range**: 0-1.5V (via resistor divider from 3.3V)
- **ADC Resolution**: 12-bit (0-2047 counts)

### Wind Data Management

#### Data Variables

```cpp
bool _wind_enable;              // Mechanical wind sensor enable
bool _ultrawind_enable;         // Ultrasonic wind sensor enable
bool _wind_err;                 // Error flag
unsigned short _wind_speed;     // Average wind speed (m/s)
unsigned short _wind_dir;       // Average wind direction (degrees)
unsigned long _wind_interval_sec; // Sampling interval
unsigned long _windCycleTrigger;  // Next measurement timestamp
```

#### Timing Control

```cpp
void collectWind(void) {
    if (_ultrawind_enable) {
        fetchUltraWind(); // Immediate reading
        return;
    }
    
    if (_wind_enable) {
        if (millis() >= _windCycleTrigger) {
            fetchWind();
            _windCycleTrigger = millis() + (_wind_interval_sec * 1000);
        }
    }
}
```

#### Data Processing

```cpp
void formatWind(void) {
    if (_wind_sample_count > 0) {
        _wind_speed = _wind_speed / _wind_sample_count;
        _wind_dir = _wind_dir / _wind_sample_count;
    }
}

void clearWind(void) {
    _wind_speed = 0;
    _wind_dir = 0;
    _wind_sample_count = 0;
    _wind_err = false;
}
```

## MPS (Molecular Property Spectrometer)

### Hardware Architecture

**I2C Configuration:**
- **Address**: 0x40
- **Controller**: ATtiny microcontroller
- **Communication**: 5-byte data packets
- **Power Control**: Integrated with IR sensor power management

### Communication Protocol

#### Data Packet Structure

The MPS uses a 5-byte I2C protocol with encoded data:

```cpp
bool mps_read(bool _trace = false) {
    byte I2C_RxBuffer[5];
    
    // Request 5 bytes from MPS
    I2C_Ack = Wire.requestFrom(0x040, 5);
    if (I2C_Ack != 5) {
        _mps_err = true;
        _mps_err_count++;
        return 1; // Communication error
    }
    
    // Read data packet
    for (int i = 0; i < 5; i++) {
        I2C_RxBuffer[i] = Wire.read();
    }
    
    // Check for "not ready" pattern
    if ((I2C_RxBuffer[0] == 0x30) && (I2C_RxBuffer[1] == 0x31) && 
        (I2C_RxBuffer[2] == 0x32) && (I2C_RxBuffer[3] == 0x33) && 
        (I2C_RxBuffer[4] == 0x35)) {
        _mps_err = true;
        _mps_err_count++;
        return 2; // Sensor not ready
    }
    
    // Decode data based on type code
    uint8_t txCode = I2C_RxBuffer[4];
    unsigned long DSum = decodeDataBytes(I2C_RxBuffer, txCode);
    
    return processDataType(DSum, txCode);
}
```

#### Data Decoding Algorithm

```cpp
// Extract 32-bit value from 4 data bytes using type code
txCode = I2C_RxBuffer[4];
DSum = 0;

// Decode bytes based on validity flags in txCode
if ((txCode & 16) == 0) DSum = I2C_RxBuffer[0];
if ((txCode & 32) == 0) DSum += (I2C_RxBuffer[1] << 8);
if ((txCode & 64) == 0) DSum += (I2C_RxBuffer[2] << 16);
if ((txCode & 128) == 0) DSum += (I2C_RxBuffer[3] << 24);

// Convert to signed value and scale
long DSigned = DSum;
float value = DSigned / 100.0;
```

### Data Types and Processing

#### Measurement Types

The MPS provides multiple measurement types identified by the lower 4 bits of txCode:

```cpp
switch (txCode & 15) {
    case 1: // Methane concentration
        concentration = value;
        if (concentration >= 0) {
            _mpsConcentration += concentration;
            _mps_sample_count++;
            if (concentration > _mpsConcentration_max) {
                _mpsConcentration_max = concentration;
            }
        }
        Serial.printf("ReadMPS: %.2fppm\r\n", concentration);
        break;
        
    case 2: // Temperature
        temperature = value;
        Serial.printf("ReadMPS: %.2f°C\r\n", temperature);
        break;
        
    case 3: // Pressure
        pressure = value * 1000.0; // Convert kPa to Pa
        Serial.printf("ReadMPS: %.2fPa\r\n", pressure);
        break;
        
    case 4: // Humidity
        humidity = value;
        if (humidity > 100) humidity = 100;
        Serial.printf("ReadMPS: %.2fRH\r\n", humidity);
        break;
        
    case 5: // Error code
        _mps_err = true;
        _mps_err_count++;
        Serial.printf("ReadMPS: Error %d\r\n", DSigned);
        return 1;
        
    default: // Invalid type code
        _mps_err = true;
        _mps_err_count++;
        return 1;
}
```

### Error Handling and Recovery

#### Automatic Recovery System

The MPS implements sophisticated error recovery:

```cpp
void rebootMPS(void) {
    static unsigned short _flush_count;
    static unsigned short _reboot_count;
    
    if ((_mps_i2c) && (_mps_enable)) {
        _mps_err = false;
        
        if (_flush_count >= 5) {
            // Multiple flush attempts failed - try power cycle
            if (_reboot_count == 3) {
                setMPS_OFF();
                Serial.println("Take MPS off line");
            }
            
            if (_reboot_count >= 3) {
                // Permanent failure
                _reboot_count = 4;
                _is_mps_on = false;
                _mps_err = true;
                return;
            }
            
            // Power cycle recovery
            setMPS_OFF();
            taskDelay(500, 1);
            Serial.printf("Try cycling the power to MPS _%u\r\n", _reboot_count);
            setMPS_ON();
            taskDelay(5000, 1);
            
            if (mps_begin() == 0) {
                _is_mps_on = true;
            }
            _reboot_count++;
            _flush_count = 0;
        } else {
            // Try flushing internal buffer
            _flush_count++;
            Serial.printf("Try flushing MPS internal buffer _%u\r\n", _flush_count);
            mps_read(true);
            
            if (_mps_err == false) {
                _is_mps_on = true;
                _flush_count = 0;
                _reboot_count = 0;
            }
        }
    }
}
```

#### Recovery Strategy

1. **Buffer Flush**: Attempt to clear internal sensor buffer (5 attempts)
2. **Power Cycle**: Hardware reset via power control (3 attempts)
3. **Offline Mode**: Disable sensor after persistent failures
4. **Error Counting**: Track consecutive failures for decision making

### Initialization Process

#### Sensor Startup Sequence

```cpp
bool mps_begin(void) {
    byte I2C_Ack;
    
    // Wait for ATtiny to overwrite default data packet
    I2C_Ack = 2;
    while (I2C_Ack == 2) {
        I2C_Ack = mps_read(true);
        delay(1000);
    }
    
    // Seed internal buffers with valid readings
    for (int i = 1; i < 5; i++) {
        I2C_Ack = mps_read(true);
        if (I2C_Ack == 0) {
            return 0; // Success
        }
        delay(100);
    }
    return 1; // Initialization failed
}
```

### Power Management

#### Sleep Mode (Future Feature)

```cpp
void sleepMPS(void) {
    if (_mps_i2c) {
        Serial.println("MPS Sleep (WeatherShield must be >= VF) ...");
        Wire.beginTransmission(0x040);
        Wire.write(0x05A); // Sleep command
        uint8_t ack = Wire.endTransmission();
        if (ack != 0) {
            Serial.printf("MPS Sleep: I2C error %u\r\n", ack);
        }
    }
}
```

**Note**: Sleep functionality requires ATtiny MPS V3d or later firmware.

### Data Processing

#### Concentration Averaging

```cpp
void formatMPS(void) {
    if (_mps_sample_count > 1) {
        _mpsConcentration = _mpsConcentration / _mps_sample_count;
    } else {
        _mpsConcentration = 0;
    }
}
```

#### Data Variables

```cpp
bool _mps_enable;              // MPS sensor enable flag
bool _is_mps_on;               // MPS operational status
bool _mps_err;                 // Current error status
unsigned short _mps_err_count; // Consecutive error count
float _mpsConcentration;       // Average methane concentration (ppm)
float _mpsConcentration_max;   // Maximum concentration in period
```

## Data Output Format

### Wind Measurements
- **Wind Speed**: Meters per second (m/s)
- **Wind Direction**: Degrees (0-360°, 0° = North)
- **Sampling**: Configurable interval timing

### MPS Measurements
- **Methane Concentration**: Parts per million (ppm)
- **Temperature**: Degrees Celsius (°C)
- **Pressure**: Pascals (Pa)
- **Humidity**: Relative humidity percentage (%RH)

### Serial Output Examples

**Wind Sensor:**
```
windSpeed: 3.25m/s  Cpt:15
winDir: 1.25V  angle:298
```

**Ultrasonic Wind:**
```
Wind Speed: 4 m/s
Wind Direction: 270 degrees
```

**MPS Sensor:**
```
ReadMPS: 2.45ppm
ReadMPS: 23.5°C
ReadMPS: 101325Pa
ReadMPS: 45.2RH
```

## Integration and Coordination

### System Integration

Both wind and MPS sensors integrate with the main data collection cycle:
- **Wind**: Configurable sampling intervals via `_wind_interval_sec`
- **MPS**: Continuous monitoring with automatic error recovery
- **Power Management**: Coordinated with overall system sleep cycles
- **Error Reporting**: Integrated error flags and recovery mechanisms

### Configuration Dependencies

- **Wind Enable**: JSON configuration controls mechanical wind sensor
- **MPS Enable**: JSON configuration controls methane detection
- **Auto-Detection**: Ultrasonic wind sensor automatically overrides mechanical sensor
- **Power Control**: MPS shares power management with IR sensors

The wind and MPS sensor systems provide comprehensive environmental monitoring with robust error handling, automatic sensor detection, and sophisticated data processing for accurate meteorological and gas concentration measurements.