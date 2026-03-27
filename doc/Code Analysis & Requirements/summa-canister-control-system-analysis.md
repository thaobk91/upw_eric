# SUMMA Canister Control System Analysis

## Overview

The SUMMA (Specially prepared Unmixed MAterial) canister control system is an automated air sampling mechanism that triggers based on PID (Photoionization Detector) sensor readings. When volatile organic compound concentrations exceed a configurable threshold for a specified duration, the system automatically opens a SUMMA canister to collect an air sample, then closes it after a predetermined time.

## System Architecture

### Hardware Components
- **I2C Relay Controller**: Multi-channel relay board (Seeed Multi-Channel Relay)
- **I2C Addresses**: Primary 0x11, Fallback 0x12
- **Relay Channels**: Separate channels for open and close operations
- **Power Control**: Integrated with weather shield power management

### Software Components
- **SummaCan.cpp/h**: Hardware abstraction layer
- **SummaTest.cpp/h**: Comprehensive test suite
- **State Machine**: Integrated into main data collection loop
- **Configuration Management**: JSON-based parameter storage

## Configuration Parameters

### Core Settings
```json
{
  "hardware": {
    "summa": {
      "enable": false,
      "i2c_address": "0x11",
      "open_channel": 1,
      "close_channel": 2,
      "pid_threshold_ppm": 7.0,
      "dwell_seconds": 900,
      "open_pulse_ms": 100,
      "unlatch_after_seconds": 3000,
      "close_pulse_ms": 100,
      "triggered": 0,
      "triggered_at": 0
    }
  }
}
```

### Parameter Validation and Defaults

#### I2C Address
- **Format**: Hex string ("0x11", "0x12")
- **Range**: 0x08-0x77 (valid I2C address space)
- **Default**: 0x11
- **Validation**: Automatic parsing from hex string format

#### Channel Configuration
- **Open Channel**: Relay channel for opening canister (1-8)
- **Close Channel**: Relay channel for closing canister (1-8)
- **Validation**: Channels must be different and within 1-8 range
- **Default**: Open=1, Close=2

#### Trigger Parameters
- **PID Threshold**: Concentration threshold in ppm (must be positive)
- **Range**: 0.1-1000 ppm
- **Default**: 7.0 ppm
- **Purpose**: Minimum PID reading to start dwell timer

#### Timing Parameters
- **Dwell Seconds**: Time threshold must be exceeded before triggering
- **Range**: 1-3600 seconds (1 second to 1 hour)
- **Default**: 900 seconds (15 minutes)
- **Purpose**: Prevents false triggers from brief concentration spikes

- **Open Pulse Duration**: Relay activation time for opening
- **Range**: 10-5000 milliseconds
- **Default**: 100ms
- **Purpose**: Duration of electrical pulse to open canister valve

- **Close Pulse Duration**: Relay activation time for closing
- **Range**: 10-5000 milliseconds  
- **Default**: 100ms
- **Purpose**: Duration of electrical pulse to close canister valve

- **Unlatch After Seconds**: Auto-close timer duration
- **Range**: 60-7200 seconds (1 minute to 2 hours)
- **Default**: 3000 seconds (50 minutes)
- **Purpose**: Maximum sampling duration before automatic closure

#### State Persistence
- **Triggered**: Binary flag (0/1) indicating if canister has been triggered
- **Triggered At**: Unix timestamp when trigger occurred
- **Purpose**: Maintains trigger history across power cycles and reboots

## Hardware Interface Layer

### Initialization and Detection
```cpp
bool summa_init() {
    // Try primary address 0x11
    relay.begin(0x11);
    Wire.beginTransmission(0x11);
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
        _summa_address = 0x11;
        _summa_present = true;
        _summa_i2c = true;
        return true;
    }
    
    // Try fallback address 0x12
    relay.begin(0x12);
    Wire.beginTransmission(0x12);
    error = Wire.endTransmission();
    
    if (error == 0) {
        _summa_address = 0x12;
        _summa_present = true;
        _summa_i2c = true;
        return true;
    }
    
    // No hardware found
    _summa_present = false;
    _summa_i2c = false;
    return false;
}
```

### Relay Operations
```cpp
void summa_open() {
    if (!summa_present() || !_summa_enable) return;
    
    Serial.printf("SUMMA: Opening canister - channel %d, pulse %dms\n", 
                  _summa_open_channel, _summa_open_pulse_ms);
    
    relay.turn_on_channel(_summa_open_channel);
    delay(_summa_open_pulse_ms);
    relay.turn_off_channel(_summa_open_channel);
}

void summa_close() {
    if (!summa_present() || !_summa_enable) return;
    
    Serial.printf("SUMMA: Closing canister - channel %d, pulse %dms\n", 
                  _summa_close_channel, _summa_close_pulse_ms);
    
    relay.turn_on_channel(_summa_close_channel);
    delay(_summa_close_pulse_ms);
    relay.turn_off_channel(_summa_close_channel);
}
```

## State Machine Logic

### State Variables
```cpp
// Runtime state (not persisted)
bool _summa_armed = false;           // System armed and monitoring
bool _summa_triggered_state = false; // Canister has been triggered (runtime)
unsigned long _summa_dwell_start = 0; // When threshold was first exceeded
unsigned long _summa_close_due = 0;   // When to close the canister
bool _summa_threshold_exceeded = false; // Current threshold status

// Persistent state (saved to config.json)
uint8_t _summa_triggered = 0;        // Historical trigger flag
uint32_t _summa_triggered_at = 0;    // Timestamp of trigger event
```

### State Machine Flow

#### 1. System Initialization
```cpp
if (_summa_enable && summa_present()) {
    if (!_summa_armed) {
        _summa_armed = true;
        _summa_triggered_state = (_summa_triggered == 1); // Load persisted state
        Serial.println("SUMMA: System armed and monitoring");
    }
}
```

#### 2. Auto-Close Timer Check
```cpp
unsigned long current_time = millis();

if (_summa_triggered_state && _summa_close_due > 0 && current_time >= _summa_close_due) {
    Serial.println("SUMMA: Closing canister - timer expired");
    summa_close();
    _summa_close_due = 0; // Clear close timer
    // Note: We maintain triggered=1 as a persistent historical marker
}
```

#### 3. Threshold Monitoring (Only if not already triggered)
```cpp
if (!_summa_triggered_state) {
    bool threshold_met = (_pid >= _summa_pid_threshold_ppm);
    
    if (threshold_met && !_summa_threshold_exceeded) {
        // Threshold just exceeded - start dwell timer
        _summa_threshold_exceeded = true;
        _summa_dwell_start = current_time;
        Serial.printf("SUMMA: Threshold exceeded (%.2f >= %.2f ppm), starting dwell timer\r\n", 
                     _pid, _summa_pid_threshold_ppm);
    }
    else if (!threshold_met && _summa_threshold_exceeded) {
        // Threshold no longer met - reset dwell timer
        _summa_threshold_exceeded = false;
        _summa_dwell_start = 0;
        Serial.println("SUMMA: Threshold no longer exceeded, resetting dwell timer");
    }
    else if (threshold_met && _summa_threshold_exceeded) {
        // Check if dwell time is satisfied
        unsigned long dwell_elapsed = (current_time - _summa_dwell_start) / 1000;
        
        if (dwell_elapsed >= _summa_dwell_seconds) {
            // Trigger the canister
            Serial.printf("SUMMA: Triggering canister - dwell time satisfied (%lu >= %u seconds)\r\n", 
                         dwell_elapsed, _summa_dwell_seconds);
            
            summa_open();
            
            // Update state and persist immediately
            _summa_triggered = 1;
            _summa_triggered_at = _time_stamp;
            _summa_triggered_state = true;
            
            // Calculate close time
            _summa_close_due = current_time + (_summa_unlatch_after_seconds * 1000UL);
            
            // Reset threshold tracking
            _summa_threshold_exceeded = false;
            _summa_dwell_start = 0;
            
            // Persist state immediately
            saveSummaConfig();
            Serial.println("SUMMA: State persisted to config.json");
        }
    }
}
```

#### 4. Hardware Absence Handling
```cpp
else if (_summa_enable && !summa_present()) {
    // Hardware not present but enabled - disable functionality
    if (_summa_armed) {
        _summa_armed = false;
        Serial.println("SUMMA: Hardware not present, disabling functionality");
    }
}
```

## State Machine Diagram

```
[System Start] 
    ↓
[Check Enable & Hardware Present] 
    ↓ (Yes)
[Arm System & Load Persistent State]
    ↓
[Check Auto-Close Timer] → [Close Canister] (if timer expired)
    ↓
[Already Triggered?] → [Monitor Only] (if yes)
    ↓ (No)
[Check PID Threshold]
    ↓
[Threshold Exceeded?]
    ↓ (Yes, First Time)
[Start Dwell Timer]
    ↓
[Threshold Still Exceeded?]
    ↓ (No)
[Reset Dwell Timer] → [Continue Monitoring]
    ↓ (Yes)
[Dwell Time Satisfied?]
    ↓ (Yes)
[Trigger Canister: Open → Set State → Persist → Schedule Close]
    ↓
[Continue Monitoring for Auto-Close]
```

## Safety Mechanisms

### Hardware Protection
- **I2C Communication Validation**: Verifies relay board presence before operations
- **Address Scanning**: Automatic fallback from 0x11 to 0x12
- **Error Logging**: Single error message per boot cycle to prevent spam

### Configuration Validation
- **Parameter Bounds Checking**: All values validated against safe ranges
- **Channel Conflict Prevention**: Open and close channels must be different
- **Default Value Fallback**: Invalid parameters replaced with safe defaults

### State Persistence
- **Immediate Persistence**: Trigger state saved to config.json immediately
- **Power Cycle Recovery**: State maintained across reboots and power loss
- **Historical Tracking**: Triggered flag remains set as permanent record

### Timing Safety
- **Dwell Timer Reset**: Prevents false triggers from brief concentration spikes
- **Auto-Close Timer**: Ensures canister doesn't remain open indefinitely
- **Overflow Protection**: Uses unsigned long for timing calculations

## Data Integration

### CSV Data Format
The SUMMA trigger state is included as the final field in CSV data records:
```cpp
sprintf(scratch, "%u\r\n", _summa_triggered); // SUMMA triggered state  Idx:68
strcat(csvWBuffer, scratch);
```

### JSON Telemetry Format
```json
{
  "summa_triggered": {
    "value": 1
  }
}
```

**Important**: The JSON field is sourced directly from runtime state (`_summa_triggered`), not from CSV parsing, ensuring real-time accuracy.

### MQTT Transmission
The SUMMA trigger state is transmitted with every data packet, allowing remote monitoring of canister status and trigger history.

## Test Suite

### Comprehensive Testing Framework
The system includes extensive automated testing covering:

#### 1. I2C Communication Tests
- Bus initialization validation
- Address scanning (0x11 and 0x12)
- Hardware presence detection
- Communication error handling

#### 2. Relay Operation Tests
- Open channel pulse operations
- Close channel pulse operations
- Multiple operation sequences
- Hardware absence graceful handling

#### 3. Configuration Validation Tests
- Default value validation
- Parameter bounds checking
- Channel configuration validation
- Invalid parameter handling

#### 4. Configuration Persistence Tests
- State save operations
- Configuration reload verification
- State restoration after modification

#### 5. State Machine Tests
- Initial state validation
- Threshold detection logic
- Dwell timer reset functionality
- State transition verification

#### 6. Telemetry Integration Tests
- CSV field addition and formatting
- JSON structure validation
- Format consistency across states
- Runtime state independence verification

### Test Execution
```cpp
// Run complete test suite
summa_run_all_tests();

// Individual test categories
summa_test_run_i2c();
summa_test_run_relay();
summa_test_run_config();
summa_test_run_state();
summa_test_run_telemetry();
```

## Error Handling and Recovery

### Hardware Errors
- **I2C Communication Failure**: Graceful degradation, error logging
- **Relay Board Absence**: Automatic detection and functionality disable
- **Address Conflicts**: Automatic fallback to secondary address

### Configuration Errors
- **Invalid Parameters**: Automatic correction with safe defaults
- **Missing Configuration**: Default configuration creation
- **Persistence Failures**: Error logging and retry mechanisms

### State Machine Errors
- **Timer Overflow**: Protected by unsigned long arithmetic
- **Invalid State Transitions**: Bounds checking and validation
- **Power Cycle Recovery**: State restoration from persistent storage

## Integration with Main System

### Data Collection Integration
The SUMMA state machine executes within the main data collection loop (`recordData()` function), ensuring:
- Real-time PID monitoring
- Synchronized timing with sensor readings
- Integrated error handling with main system

### Power Management Integration
- **Sleep Mode Compatibility**: State preserved during system sleep
- **Battery Protection**: Respects low-power conditions
- **Graceful Shutdown**: Proper cleanup during system shutdown

### Communication Integration
- **MQTT Telemetry**: Trigger state included in all data transmissions
- **Remote Monitoring**: Real-time status visibility
- **Historical Tracking**: Persistent trigger records for analysis

## Deployment Considerations

### Hardware Setup
1. Install I2C relay board at address 0x11 (or 0x12)
2. Connect relay channels to canister valve actuators
3. Verify I2C communication during system initialization

### Configuration Setup
1. Enable SUMMA functionality in config.json
2. Set appropriate PID threshold for deployment environment
3. Configure dwell time based on sampling requirements
4. Set unlatch timer based on desired sample duration

### Testing and Validation
1. Run comprehensive test suite before deployment
2. Verify hardware communication and relay operations
3. Test state machine transitions with simulated PID values
4. Validate telemetry integration and data transmission

### Monitoring and Maintenance
1. Monitor SUMMA trigger status via MQTT telemetry
2. Review trigger history and timing patterns
3. Validate canister operations during maintenance visits
4. Update configuration parameters based on field experience