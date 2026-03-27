# Air Monitor System - Comprehensive Technical Analysis

## Table of Contents

1. [System Overview and Architecture](#system-overview-and-architecture)
2. [Sensor Systems Documentation](#sensor-systems-documentation)
3. [Data Processing and Algorithms](#data-processing-and-algorithms)
4. [Communication and Configuration](#communication-and-configuration)
5. [Implementation Guide and Reference Materials](#implementation-guide-and-reference-materials)

---

## System Overview and Architecture

### Hardware Platform Overview

The air monitoring system is built on an **ESP32 Feather microcontroller** with **Weather Shield hardware**, implementing a sophisticated multi-sensor environmental monitoring platform with LTE connectivity for real-time data transmission.

#### Core Hardware Components
- **Microcontroller**: ESP32 Feather (dual-core Xtensa LX6 processor)
- **Hardware Platform**: Weather Shield (WS) - multiple versions supported (v14 and earlier)
- **Operating System**: FreeRTOS with multi-task architecture
- **Storage Systems**: 
  - SD card for data logging and configuration
  - SPIFFS (SPI Flash File System) for firmware and backup storage
- **Communication**: LTE modem with integrated GPS/GNSS capability
- **Power Management**: Battery monitoring with low-voltage protection

#### System Architecture Diagram

```mermaid
graph TB
    subgraph "ESP32 Feather Core"
        CPU1[Core 0<br/>Data Collection<br/>Data Transmission]
        CPU2[Core 1<br/>Serial Proxy]
        WDT[Watchdog Timer<br/>90s timeout]
    end
    
    subgraph "I2C Communication Bus"
        TCA[TCA9548 Multiplexer<br/>0x70]
        PCF1[PCF8574 IC8<br/>0x39 - UI Control]
        PCF2[PCF8574 IC113<br/>0x3A - Power Control]
        PCF3[PCF8574 IC102<br/>0x3B - SPEC Control]
        EEPROM[EEPROM<br/>0x50/0x52]
        BATT[Battery Monitor<br/>0x4B]
    end
    
    subgraph "Sensor Channels via TCA"
        CH0[Ch0: Grove BME280/680<br/>0x76]
        CH1[Ch1: SPEC Sensors<br/>LMP91000 0x6B]
        CH2[Ch2: VOC Sensors<br/>SGP40 0x59]
        CH3[Ch3: WS BME280/680<br/>0x76]
        CH4[Ch4: PM Sensor<br/>SPS30 0x69]
        CH5[Ch5: MPS Sensor<br/>0x40]
        CH6[Ch6: IR Sensors<br/>ADC 0x6C]
        CH7[Ch7: NH3 Sensor<br/>ADC 0x6A]
    end
    
    subgraph "External Systems"
        LTE[LTE Modem<br/>GPS/GNSS]
        SD[SD Card Storage]
        SUMMA[SUMMA Canister<br/>Relay Control]
        PUMP[Sampling Pump]
    end
    
    CPU1 --> TCA
    CPU1 --> PCF1
    CPU1 --> PCF2
    CPU1 --> PCF3
    CPU2 --> LTE
    
    TCA --> CH0
    TCA --> CH1
    TCA --> CH2
    TCA --> CH3
    TCA --> CH4
    TCA --> CH5
    TCA --> CH6
    TCA --> CH7
    
    PCF2 --> PUMP
    PCF3 --> SUMMA
    CPU1 --> SD
```

### FreeRTOS Task Architecture

The system implements a sophisticated multi-task architecture using FreeRTOS with three primary tasks distributed across both CPU cores for optimal performance and real-time operation.

#### Task 1: collectDataTask (Core 0, Priority 1)
**Primary Function**: Sensor data collection and processing

**Technical Specifications**:
- **Stack Size**: 40,960 words (163,840 bytes)
- **CPU Core**: Core 0 (shared with sendDataTask)
- **Priority**: 1 (lower priority than transmission task)
- **Execution Model**: Continuous loop with configurable timing intervals

**Core Responsibilities**:
1. **Sampling Control**: Manages sampling intervals and sensor stabilization timing
2. **Pump Management**: Controls sampling pump for gas sensor operation
3. **Data Collection**: Orchestrates data collection from all sensor modules
4. **Data Processing**: Implements averaging, concentration calculations, and peak detection
5. **SUMMA Control**: Manages SUMMA canister state machine and trigger logic
6. **Error Handling**: Detects sensor failures and implements automatic recovery
7. **Data Storage**: Records processed data in CSV format to SD card storage

**Task Flow Diagram**:
```mermaid
flowchart TD
    A[Wait for Sampling Enable] --> B{Pumping Active?}
    B -->|Yes| C[Stop Sampling]
    B -->|No| D[Wait Stabilization Period]
    C --> D
    D --> E[Collect All Sensors]
    E --> F{Sample Count Reached?}
    F -->|No| G[Increment Sample Count]
    F -->|Yes| H[Process & Record Data]
    G --> I{Pumping Needed?}
    H --> J[Initialize Next Collection]
    J --> K[Sleep Period]
    K --> I
    I -->|Yes| L[Resume Pumping]
    I -->|No| A
    L --> A
```

#### Task 2: sendDataTask (Core 0, Priority 2)
**Primary Function**: Data transmission and MQTT communication

**Technical Specifications**:
- **Stack Size**: 40,960 words (163,840 bytes)
- **CPU Core**: Core 0 (shared with collectDataTask)
- **Priority**: 2 (higher priority for time-critical communications)
- **Execution Model**: Event-driven with automatic retry mechanisms

**Core Responsibilities**:
1. **Connection Management**: Establishes and maintains LTE modem and MQTT connections
2. **Data Retrieval**: Reads stored CSV data using binary index system
3. **Data Conversion**: Converts CSV records to JSON format for MQTT transmission
4. **Transmission Queue**: Manages transmission queue with error recovery
5. **Error Recovery**: Handles connection failures and implements automatic reconnection
6. **OTA Updates**: Manages Over-The-Air firmware update process
7. **Command Processing**: Handles incoming MQTT commands and responses

#### Task 3: serialModemProxyTask (Core 1, Priority 1)
**Primary Function**: Serial communication proxy for LTE modem

**Technical Specifications**:
- **Stack Size**: 4,096 words (16,384 bytes)
- **CPU Core**: Core 1 (dedicated core for isolation)
- **Priority**: 1 (standard priority)
- **Execution Model**: Continuous proxy service

**Core Responsibilities**:
1. **Serial Interface**: Provides transparent serial communication to LTE modem
2. **AT Command Processing**: Handles AT command routing and responses
3. **Debug Interface**: Enables modem debugging and diagnostics
4. **Protocol Translation**: Manages protocol translation between ESP32 and modem

### Inter-Task Communication and Synchronization

#### Shared Resource Management

**SD Card Mutex Protection**:
```cpp
SemaphoreHandle_t sd_mutex;  // Protects SD card access between tasks

// Usage pattern:
if (xSemaphoreTake(sd_mutex, portMAX_DELAY)) {
    // Perform SD card operations safely
    xSemaphoreGive(sd_mutex);
}
```

**Critical Shared Variables**:
- `_csvWIdx`: Write index for CSV data (updated by collectDataTask)
- `_csvRIdx`: Read index for CSV data (updated by sendDataTask)
- `_sampling_status`: Global sampling enable/disable flag
- `_sampling_status_lock`: Prevents race conditions during sampling control
- `_ota_status`: Coordinates OTA updates between tasks

#### Data Flow Architecture

```mermaid
sequenceDiagram
    participant CT as collectDataTask
    participant SD as SD Card Storage
    participant ST as sendDataTask
    participant MQTT as MQTT Broker
    
    CT->>CT: Collect Sensor Data
    CT->>CT: Process & Average
    CT->>SD: Write CSV Record
    CT->>SD: Update Write Index
    
    Note over ST: Wait for new data
    ST->>SD: Check Read Index
    SD-->>ST: CSV Record Available
    ST->>SD: Read CSV Record
    ST->>ST: Convert to JSON
    ST->>MQTT: Publish Data
    MQTT-->>ST: Acknowledge
    ST->>SD: Mark Record as Sent
    ST->>SD: Update Read Index
```

### Hardware Interface Architecture

#### I2C Communication System

**TCA9548 Multiplexer Architecture**:
The system uses a TCA9548 I2C multiplexer (address 0x70) to manage multiple I2C devices and resolve address conflicts across 8 channels:

| Channel | Device Type | Address | Function |
|---------|-------------|---------|----------|
| 0 | Grove BME280/680 | 0x76 | External environmental sensor |
| 1 | LMP91000 | 0x6B | Electrochemical sensor ADC |
| 2 | SGP40/SGP30 | 0x59/0x58 | VOC sensors |
| 3 | WS BME280/680 | 0x76 | Onboard environmental sensor |
| 4 | SPS30 | 0x69 | Particulate matter sensor |
| 5 | ATTiny MPS | 0x40 | Micro particle sensor |
| 6 | IR ADC | 0x6C | Infrared gas sensors |
| 7 | NH3 ADC | 0x6A | Ammonia sensor (version dependent) |

**GPIO Expander Network**:
Multiple PCF8574 GPIO expanders provide distributed I/O control:

- **0x39 (FeatherBase IC8)**: User interface and system control
- **0x3A (Weather Shield IC113)**: Sensor power management
- **0x3B (Weather Shield IC102)**: SPEC sensor selection
- **0x38 (FeatherBase IC7)**: Wind speed measurement

#### Power Management Architecture

**Power Sequencing System**:
1. **Core Power**: ESP32 and I2C infrastructure initialization
2. **GPIO Expanders**: PCF8574 device configuration
3. **3VP Supply**: 3.3V sensor power rail activation
4. **Individual Sensors**: Controlled power-up via GPIO expanders
5. **TCA Multiplexer**: Reset and channel configuration
6. **Sensor Initialization**: Sequential sensor setup through TCA channels

**Power Control Functions**:
```cpp
// Core power management
void set3VP_ON()          // Enable 3.3V sensor power rail
void shieldOFF()          // Disable all sensor power

// Sensor-specific power control
void setIR_ON()           // Power CO2, CH4, PID sensors
void setSPEC_ONLine(byte) // Enable specific electrochemical sensor
void setMPS_ON()          // Power micro particle sensor
void setPM_ON()           // Power particulate matter sensor
void setSampling_ON()     // Enable sampling pump
```

### System Initialization and Startup Sequence

#### Main Setup Process (`setup()` function)

**Phase 1: Core System Initialization**
```cpp
Serial.begin(115200);
taskDelay(10000,7);  // Wait for serial and restore screen color
```

**Phase 2: Storage System Setup**
```cpp
init_SDmutex();      // Initialize SD card mutex for task synchronization
mountFlash();        // Mount SPIFFS for firmware storage
mountSD();           // Mount SD card for data logging
```

**Phase 3: Configuration Management**
```cpp
if (readConfig() == 0) { 
    ColdBoot("Unable to read config.json"); 
}
```

**Phase 4: Hardware Platform Configuration**
```cpp
setupFeather();      // Initialize ESP32 Feather hardware interfaces
taskDelay(10000,7);  // Allow sensors to complete boot sequence
```

**Phase 5: Calibration Data Management**
```cpp
if (_config_id == 1) {
    if (checkEEStamp() == 0) { writeEECalib(); }  // Write default calibration
    readEECalib();                                // Load calibration data
}
```

**Phase 6: Sensor System Initialization**
```cpp
setupSensors();      // Initialize all sensor modules sequentially
```

**Phase 7: SUMMA Canister System**
```cpp
if (_summa_enable) {
    summa_init();    // Initialize SUMMA canister control system
}
```

**Phase 8: Data Logging System**
```cpp
log_Create();        // Create CSV log file if not found
rContext(0);         // Fetch logging task status and recovery context
```

**Phase 9: Task Creation and Startup**
```cpp
// Create tasks with specific core assignments and priorities
xTaskCreatePinnedToCore(sendDataTask, "SendData", 40960, NULL, 2, NULL, 0);
xTaskCreatePinnedToCore(collectDataTask, "CollectData", 40960, NULL, 1, NULL, 0);
xTaskCreatePinnedToCore(serialModemProxyTask, "SerialModemProxy", 4096, NULL, 1, NULL, 1);
```

**Phase 10: System Protection**
```cpp
esp_task_wdt_init(90, true);  // 90-second watchdog timer with panic on timeout
```

### Main Loop and System Monitoring

The main loop provides continuous system monitoring and user interface management:

**Core Functions**:
1. **User Interface**: Button press detection and LED status indication
2. **System Reset**: 10-second button press for database clearing and reboot
3. **Watchdog Management**: Regular watchdog timer reset
4. **Task Health Monitoring**: Detection of task failures and system errors
5. **Emergency Procedures**: Coordinated system shutdown and recovery

```cpp
void loop() {
    taskDelay(100, 0);  // 100ms loop timing
    
    // User button handling for system reset
    if (swNet_button()) {
        // Implement 10-second press detection
        // Clear database and coordinate system reboot
    }
    
    // Watchdog timer reset and system health monitoring
    esp_task_wdt_reset();
}
```

### System State Management and Recovery

#### Configuration Management
- **Dual Storage**: Configuration stored on both SD card and SPIFFS for redundancy
- **Runtime Updates**: Configuration parameters can be updated via MQTT commands
- **Persistent State**: System state preserved across reboots for recovery

#### Error Handling and Recovery Mechanisms
- **Sensor Recovery**: Automatic sensor reboot on I2C communication failures
- **System Protection**: Automatic reboot on excessive errors (limited to 3 consecutive reboots)
- **Environmental Protection**: Battery and temperature monitoring with automatic shutdown
- **Remote Diagnostics**: MQTT error reporting for remote system monitoring

#### Data Integrity and Validation
- **CSV Validation**: Data bounds checking and format validation
- **Index Consistency**: Binary index file integrity mechanisms
- **Transmission Tracking**: Status tracking prevents duplicate transmissions
- **Recovery Mechanisms**: Automatic recovery from storage and transmission failures

This comprehensive system architecture provides a robust foundation for continuous environmental monitoring with reliable data collection, processing, and transmission capabilities.
---


## Sensor Systems Documentation

The air monitoring system incorporates a comprehensive array of sensors for environmental monitoring, gas detection, and air quality assessment. This section provides detailed documentation of all sensor types, their data collection processes, calibration procedures, and maintenance requirements.

### Sensor Architecture Overview

The sensor system is built around a hierarchical I2C communication architecture using a TCA9548 multiplexer to manage multiple sensor types across 8 dedicated channels. Each sensor type implements specialized data processing algorithms, calibration procedures, and error recovery mechanisms.

#### Sensor Categories

1. **Electrochemical Sensors**: H2S, O3, SO2, NO2, NH3 gas detection
2. **Infrared Sensors**: CO2, CH4, PID gas measurements
3. **Environmental Sensors**: Temperature, humidity, pressure, air quality
4. **Particulate Matter Sensors**: PM1.0, PM2.5, PM10 measurements
5. **VOC Sensors**: Volatile organic compound detection
6. **Wind Sensors**: Wind speed and direction measurement
7. **MPS Sensors**: Molecular property spectrometer for methane

### Sensor Data Flow Architecture

```mermaid
flowchart TD
    subgraph "TCA Multiplexer Channels"
        CH0[Ch0: Grove BME280/680<br/>Environmental]
        CH1[Ch1: LMP91000<br/>Electrochemical]
        CH2[Ch2: SGP40/SGP30<br/>VOC Sensors]
        CH3[Ch3: WS BME280/680<br/>Environmental]
        CH4[Ch4: SPS30<br/>Particulate Matter]
        CH5[Ch5: ATTiny MPS<br/>Methane Detection]
        CH6[Ch6: IR ADC<br/>CO2, CH4, PID]
        CH7[Ch7: NH3 ADC<br/>Ammonia]
    end
    
    subgraph "Data Processing Pipeline"
        RAW[Raw Sensor Data]
        CALIB[Calibration Application]
        COMP[Temperature Compensation]
        PEAK[Peak Detection]
        AVG[Averaging & Validation]
        FORMAT[Data Formatting]
    end
    
    subgraph "Output Data"
        CONC[Concentrations ppm/ppb]
        ENV[Environmental Data]
        AQI[Air Quality Indices]
        DIAG[Diagnostic Values]
    end
    
    CH0 --> RAW
    CH1 --> RAW
    CH2 --> RAW
    CH3 --> RAW
    CH4 --> RAW
    CH5 --> RAW
    CH6 --> RAW
    CH7 --> RAW
    
    RAW --> CALIB
    CALIB --> COMP
    COMP --> PEAK
    PEAK --> AVG
    AVG --> FORMAT
    
    FORMAT --> CONC
    FORMAT --> ENV
    FORMAT --> AQI
    FORMAT --> DIAG
```

### Electrochemical Sensor System

#### Hardware Architecture and Configuration

The electrochemical sensor system uses **LMP91000 potentiostat chips** for signal conditioning and **MCP342x ADCs** for high-precision voltage measurement. The system supports five gas types with sophisticated peak detection and temperature compensation.

**Supported Gas Types:**
- **H2S (Hydrogen Sulfide)**: Channel 1, CSbit 0
- **NO2 (Nitrogen Dioxide)**: Channel 2, CSbit 1  
- **O3 (Ozone)**: Channel 3, CSbit 2
- **SO2 (Sulfur Dioxide)**: Channel 4, CSbit 3
- **NH3 (Ammonia)**: Channel 5, CSbit 4 (SGX CAN only)

**Potentiostat Configuration by Sensor:**

| Sensor | TIA Gain | IntZ | Bias Sign | Bias % | Sensitivity |
|--------|----------|------|-----------|--------|-------------|
| H2S | 7KΩ/120KΩ | 50% | Positive | 0% | 1200/212 nA/ppm |
| NO2 | 35KΩ/499KΩ | 50% | Negative | 0-1% | 30/45 nA/ppm |
| O3 | 35KΩ/499KΩ | 50% | Negative | 1% | 1000/60 nA/ppm |
| SO2 | 120KΩ/499KΩ | 50% | Positive | 0-10% | 400/25 nA/ppm |
| NH3 | 35KΩ | 50% | Positive | 0% | 40 nA/ppm |

#### Peak Detection Algorithm Implementation

Each electrochemical sensor implements a sophisticated peak detection algorithm for transient gas detection:

**Peak Detection Variables (per sensor):**
```cpp
float _signal;    // Current peak signal value
float _base;      // Baseline reference value  
float _top;       // Peak top value
float _mem;       // Memory/history value
float _tcomp;     // Temperature compensation factor
float _dx;        // Differential value
float _dx_trig;   // Trigger threshold for peak detection
float _corr;      // Temperature corrected value
bool _found;      // Peak detection flag
```

**Peak Detection Process:**
1. **Baseline Establishment**: Continuous baseline tracking in clean air
2. **Signal Monitoring**: Real-time signal comparison against baseline
3. **Trigger Detection**: Signal exceeds trigger threshold above baseline
4. **Peak Tracking**: Maximum signal value during detection event
5. **Memory Update**: Historical reference for future comparisons

#### Calibration Procedures

**Zero Calibration Process:**
1. **Clean Air Exposure**: Ensure sensor is in clean air environment
2. **Temperature Compensation**: Apply current temperature differential
3. **Offset Calculation**: `offset = current_voltage - (temp_diff × temp_comp)`
4. **Validation**: Ensure offset is positive and reasonable
5. **Storage**: Save calibration to EEPROM for persistence

**Span Calibration Process:**
1. **Standard Gas Application**: Expose to known concentration
2. **M-Factor Calculation**: `M = sensitivity × TIA_resistance × 10^-6`
3. **Gain Determination**: `gain = 1 / M_factor`
4. **Validation**: Verify response linearity and accuracy
5. **Configuration Storage**: Save parameters to configuration system

**Peak Detection Calibration:**
1. **Standard Gas Exposure**: Apply known concentration for peak detection
2. **Peak Signal Capture**: Wait for `_found` flag and record `_signal`
3. **Gain Calculation**: `gain = standard_ppm / peak_signal`
4. **Mode Setting**: Set `offset = 3.3V` to enable peak detection mode
5. **Trigger Optimization**: Adjust `_dx_trig` for optimal sensitivity

### Infrared Sensor System

#### Hardware Configuration and Processing

The infrared sensor system measures **CO2, CH4 (C1), and PID** sensors using analog voltage outputs processed through **MCP342x ADCs**. The system implements both linear regression and exponential calibration methods.

**Sensor Channel Mapping:**
- **Channel 1**: C1/CH4 sensor (methane detection)
- **Channel 3**: PID sensor (photo-ionization detector)
- **Channel 4**: CO2 sensor (carbon dioxide)

**Power Management:**
- **90-second boot delay** after power-on for sensor stabilization
- **Sequential power control** via PCF8574 I/O expander
- **Temperature monitoring** for sensor protection

#### Calibration Methods

**Linear Calibration (Standard Mode):**
```cpp
// Two-point linear regression between offset and full-scale
avgX = (offset_volt + 2.0) / 2.0;
avgY = (baseline + range) / 2.0;
slope = ssY / ssX;
intercept = avgY - (slope × avgX);
concentration = (slope × voltage) + intercept;
```

**Peak Detection Mode:**
```cpp
// Direct gain multiplication when offset > 3.0V
if (offset_volt > 3.0) {
    concentration = gain × signal_voltage;
}
```

**Temperature Compensation:**
```cpp
// Applied during calibration and processing
offset = measured_voltage - (temp_differential × temp_comp_factor);
```

#### Typical Calibration Parameters

| Sensor | Range | Baseline | Sensitivity | Calibration Gas |
|--------|-------|----------|-------------|-----------------|
| CO2 | 0-50,000 ppm | 407 ppm | ~6.3 µV/ppm | 30% CO2 |
| CH4 | 0-50,000 ppm | 0 ppm | ~26 µV/ppm | 50,000 ppm CH4 |
| PID | 0-40 ppm | 0 ppm | ~25 mV/ppm | 40 ppm isobutylene |

### Environmental Sensor System

#### BME280/BME680 Auto-Detection

The system automatically detects and configures either **BME280** (basic) or **BME680** (advanced) environmental sensors:

**BME280 Capabilities:**
- Temperature measurement with offset compensation
- Relative humidity measurement
- Atmospheric pressure measurement
- Basic environmental monitoring

**BME680 Advanced Features:**
- All BME280 capabilities plus:
- **BSEC Library Integration** for advanced processing
- **Air Quality Index (AQI)** calculation via gas resistance
- **CO2 Equivalent** estimation
- **VOC Equivalent** measurements
- **Indoor Air Quality** assessment

#### Data Processing and Compensation

**Temperature Offset Calibration:**
```cpp
// Applied to compensate for mounting effects
final_temperature = (raw_average + temperature_offset);
```

**Sample Accumulation:**
```cpp
// All measurements use accumulation averaging
sensor_sum += raw_reading;
sample_count++;
final_value = sensor_sum / sample_count;
```

**ESP32 CPU Temperature Monitoring:**
- Internal temperature sensor for diagnostic purposes
- System protection against overheating (>70°C threshold)
- Continuous monitoring for thermal management

#### Error Handling and Validation

**NaN Detection:**
```cpp
if (!isnan(sensor_reading)) {
    // Process valid reading
    accumulate_data(sensor_reading);
} else {
    set_error_flag();
}
```

**Range Validation:**
- Temperature: -40°C to +80°C operational range
- Humidity: 0-100% RH with bounds checking
- Pressure: Standard atmospheric range validation

### Particulate Matter Sensor System

#### SPS30 Laser-Based Measurement

The **Sensirion SPS30** provides precise particulate matter measurements using laser scattering technology:

**Measurement Capabilities:**
- **PM1.0**: Particles ≤ 1.0 μm diameter
- **PM2.5**: Particles ≤ 2.5 μm diameter  
- **PM10**: Particles ≤ 10 μm diameter
- **Built-in AQI**: EPA-standard Air Quality Index calculation

#### Intelligent Power Management

**Wake/Sleep Cycle Implementation:**
```cpp
// Measurement cycle timing
if (measurement_time_reached) {
    if (sensor_awake) {
        // Collect data and calculate averages
        read_pm_data();
        calculate_aqi();
        sleep_sensor(); // Extend sensor life
    } else {
        wake_sensor();
        schedule_next_measurement();
    }
}
```

**Power Optimization Benefits:**
- **Extended Sensor Life**: Reduces laser diode wear
- **Power Conservation**: Significant power savings during sleep
- **Thermal Management**: Prevents sensor overheating
- **Measurement Accuracy**: Maintains calibration stability

#### EPA AQI Calculation

**Standard AQI Breakpoints:**
| PM2.5 Range (μg/m³) | AQI Range | Category |
|---------------------|-----------|----------|
| 0.0 - 12.0 | 0 - 50 | Good |
| 12.1 - 35.4 | 51 - 100 | Moderate |
| 35.5 - 55.4 | 101 - 150 | Unhealthy for Sensitive |
| 55.5 - 150.4 | 151 - 200 | Unhealthy |
| 150.5 - 250.4 | 201 - 300 | Very Unhealthy |
| 250.5 - 500.4 | 301 - 500 | Hazardous |

**AQI Calculation Formula:**
```cpp
// Linear interpolation between breakpoints
AQI = ((concentration - C_low) / (C_high - C_low)) × (I_high - I_low) + I_low;
```

#### Automatic Error Recovery

**Recovery Strategy:**
1. **First Failure**: Immediate retry attempt
2. **Second Failure**: Power cycle and reinitialize
3. **Third Failure**: Temporary offline mode
4. **Persistent Failures**: Permanent sensor disable

### VOC Sensor System

#### SGP30/SGP40 Auto-Detection

The system supports both **SGP30** (legacy) and **SGP40** (current) VOC sensors with automatic detection:

**SGP30 Features:**
- **TVOC**: Total Volatile Organic Compounds (ppb)
- **eCO2**: Equivalent CO2 (ppm)
- **Built-in Processing**: Factory-calibrated algorithms
- **Baseline Management**: Automatic baseline compensation

**SGP40 Advanced Processing:**
- **Raw Signal Output**: 16-bit sensor counts for custom processing
- **Temperature/Humidity Compensation**: Real-time environmental correction
- **Peak Detection**: Transient event detection
- **Multiple Calibration Methods**: Linear, exponential, and peak detection

#### Advanced Calibration Methods

**Exponential Calibration (SGP40):**
```cpp
// Based on sensor response curve fitting
concentration = A × exp(b × raw_signal) + C;

// Default parameters from datasheet analysis:
// A = 0.5177, b = 0.0007834, C = -0.03093
```

**Linear Calibration:**
```cpp
// Simple linear relationship
concentration = slope × raw_signal + intercept;
```

**Peak Detection Mode:**
```cpp
// Uses detected peak signal instead of averaged value
if (offset < 0) {  // Peak detection enabled
    concentration = gain × peak_signal;
}
```

#### Temperature and Humidity Compensation

**Compensation Formula:**
```cpp
// Convert environmental data to sensor format
humidity_comp = humidity × (65535.0 / 100.0);
temp_comp = (temperature + 45.0) × (65535.0 / 175.0);

// Apply compensation during measurement
compensated_reading = measure_with_compensation(temp_comp, humidity_comp);
```

### Wind Measurement System

#### Dual Sensor Support

The system automatically detects and prioritizes available wind sensors:

**Ultrasonic Wind Sensor (Calypso) - Priority 1:**
- **I2C Address**: 0x15
- **High Precision**: Ultrasonic measurement technology
- **7-byte Protocol**: Wind speed and direction in single reading
- **No Moving Parts**: Maintenance-free operation

**Mechanical Wind Sensor - Priority 2:**
- **Speed Measurement**: Pulse counting via PCF8574
- **Direction Measurement**: Analog voltage (0-1.5V)
- **Resolution**: 22.5° direction increments
- **Conversion**: 2.25 mph per pulse per second

#### Data Processing and Conversion

**Ultrasonic Sensor Processing:**
```cpp
// 16-bit values scaled by 100
wind_speed_ms = raw_speed / 100;      // Convert to m/s
wind_direction = raw_direction / 100;  // Convert to degrees
```

**Mechanical Sensor Processing:**
```cpp
// Pulse counting for speed
pulse_count = end_count - start_count;
wind_speed_mph = pulse_count × (2.25 / sampling_time_sec);
wind_speed_ms = wind_speed_mph × 0.44704; // Convert to m/s

// Analog voltage for direction
voltage = (adc_count × 1.5) / 2047;
direction_degrees = (voltage × 359) / 1.5;
```

### MPS (Molecular Property Spectrometer)

#### ATtiny-Based Methane Detection

The **MPS system** uses an **ATtiny microcontroller** for precise methane concentration measurement:

**Communication Protocol:**
- **5-byte I2C packets** with encoded data
- **Multiple data types**: Concentration, temperature, pressure, humidity
- **Error detection**: Built-in validation and recovery
- **Automatic recovery**: Buffer flushing and power cycling

#### Data Decoding and Processing

**Packet Structure:**
```cpp
// 5-byte packet: [Data0][Data1][Data2][Data3][TypeCode]
// TypeCode bits 7-4: Data validity flags
// TypeCode bits 3-0: Data type identifier

// Decode 32-bit value from valid bytes
decoded_value = 0;
if (!(type_code & 0x10)) decoded_value |= data[0];
if (!(type_code & 0x20)) decoded_value |= (data[1] << 8);
if (!(type_code & 0x40)) decoded_value |= (data[2] << 16);
if (!(type_code & 0x80)) decoded_value |= (data[3] << 24);

// Convert to engineering units
final_value = signed_value / 100.0;
```

**Data Types:**
- **Type 1**: Methane concentration (ppm)
- **Type 2**: Internal temperature (°C)
- **Type 3**: Internal pressure (kPa)
- **Type 4**: Internal humidity (%RH)
- **Type 5**: Error codes

#### Sophisticated Error Recovery

**Multi-Level Recovery Strategy:**
1. **Buffer Flush**: Clear internal ATtiny buffer (5 attempts)
2. **Power Cycle**: Hardware reset via power control (3 attempts)
3. **Offline Mode**: Disable sensor after persistent failures
4. **Error Tracking**: Consecutive failure counting for decision making

### Sensor Calibration Procedures

#### General Calibration Workflow

**Pre-Calibration Requirements:**
1. **Environmental Stabilization**: Allow 30-minute warm-up period
2. **Clean Air Verification**: Ensure zero-gas environment
3. **Temperature Recording**: Document ambient conditions
4. **System Status Check**: Verify all sensors operational

**Zero Calibration Process:**
1. **Pause Data Collection**: Stop sampling during calibration
2. **Environmental Reading**: Record current temperature/humidity
3. **Baseline Measurement**: Take multiple readings in clean air
4. **Temperature Compensation**: Apply thermal correction factors
5. **Offset Calculation**: Determine zero-point correction
6. **Validation**: Verify offset within acceptable range
7. **Storage**: Save calibration to EEPROM/configuration

**Span Calibration Process:**
1. **Standard Gas Application**: Apply certified reference gas
2. **Stabilization Period**: Allow sensor response stabilization
3. **Response Measurement**: Record sensor output at known concentration
4. **Gain Calculation**: Determine sensitivity factor
5. **Linearity Check**: Verify linear response if applicable
6. **Cross-Sensitivity Test**: Check for interference effects
7. **Parameter Storage**: Save calibration coefficients

#### Sensor-Specific Calibration Requirements

**Electrochemical Sensors:**
- **Zero Gas**: Ultra-pure air or nitrogen
- **Span Gas**: Certified concentration in air balance
- **Temperature Range**: 15-35°C for optimal accuracy
- **Humidity Control**: 30-70% RH recommended
- **Stabilization Time**: 15-30 minutes per gas

**Infrared Sensors:**
- **Zero Gas**: CO2-free air for CO2 sensors, hydrocarbon-free air for others
- **Span Gas**: High-concentration certified standards
- **Temperature Stability**: ±2°C during calibration
- **Pressure Compensation**: Account for altitude effects
- **Warm-up Time**: 90 seconds minimum after power-on

**Environmental Sensors:**
- **Temperature**: Calibrated reference thermometer
- **Humidity**: Saturated salt solutions for reference points
- **Pressure**: Certified barometric reference
- **Cross-Calibration**: Compare with meteorological standards

### Maintenance Requirements and Procedures

#### Preventive Maintenance Schedule

**Daily Checks:**
- System status LED verification
- Data transmission confirmation
- Battery voltage monitoring
- Basic sensor response validation

**Weekly Maintenance:**
- **Particulate Sensor**: Check for dust accumulation
- **Sampling System**: Verify pump operation and flow rates
- **Environmental Sensors**: Compare with local weather data
- **Communication**: Verify MQTT connectivity and data upload

**Monthly Maintenance:**
- **Zero Calibration**: All gas sensors in clean air
- **Flow System**: Clean sampling lines and filters
- **Physical Inspection**: Check for corrosion, damage, or contamination
- **Data Validation**: Review sensor trends and identify drift

**Quarterly Maintenance:**
- **Span Calibration**: Full calibration with certified reference gases
- **Sensor Replacement**: Replace consumable sensors as needed
- **System Cleaning**: Clean enclosure and sensor ports
- **Software Updates**: Apply firmware updates if available

**Annual Maintenance:**
- **Complete Recalibration**: All sensors with full multi-point calibration
- **Hardware Inspection**: Detailed inspection of all components
- **Performance Validation**: Compare with reference instruments
- **Documentation Update**: Update calibration certificates and records

#### Sensor Replacement Procedures

**Electrochemical Sensor Replacement:**
1. **Power Down**: Safely shut down system
2. **Sensor Removal**: Carefully disconnect old sensor
3. **Installation**: Install new sensor with proper orientation
4. **Configuration**: Update sensor parameters in configuration
5. **Calibration**: Perform complete zero and span calibration
6. **Validation**: Verify performance against specifications

**Environmental Sensor Replacement:**
1. **Backup Configuration**: Save current calibration parameters
2. **Hardware Replacement**: Install new BME280/BME680 sensor
3. **Auto-Detection**: Verify system detects new sensor type
4. **Calibration Transfer**: Apply previous calibration if compatible
5. **Performance Check**: Validate against reference measurements

#### Troubleshooting Common Issues

**Communication Errors:**
- **I2C Bus Issues**: Check wiring, pull-up resistors, and bus conflicts
- **TCA Multiplexer**: Verify multiplexer operation and channel selection
- **Power Supply**: Ensure stable 3.3V supply to all sensors
- **EMI Interference**: Check for electromagnetic interference sources

**Calibration Drift:**
- **Temperature Effects**: Verify temperature compensation is working
- **Aging Effects**: Check sensor age and replace if beyond service life
- **Contamination**: Clean sensor surfaces and sampling paths
- **Cross-Sensitivity**: Verify no interfering gases present

**Data Quality Issues:**
- **Noise**: Check for electrical noise sources and grounding
- **Baseline Drift**: Perform zero calibration in clean air
- **Response Time**: Verify sensor warm-up and stabilization times
- **Range Issues**: Ensure measurements within sensor specifications

This comprehensive sensor systems documentation provides the foundation for understanding, operating, and maintaining the complete air monitoring sensor array with all necessary procedures for accurate and reliable environmental monitoring.-
--

## Data Processing and Algorithms

The air monitoring system implements sophisticated data processing algorithms to transform raw sensor readings into accurate, validated measurements. This section provides comprehensive documentation of all mathematical operations, calibration equations, validation mechanisms, and data formatting procedures.

### Data Processing Architecture Overview

The data processing pipeline follows a structured flow from raw sensor readings to formatted output data:

```mermaid
flowchart TD
    subgraph "Raw Data Collection"
        ADC[ADC Readings]
        I2C[I2C Sensor Data]
        ANALOG[Analog Inputs]
    end
    
    subgraph "Primary Processing"
        TEMP_COMP[Temperature Compensation]
        CALIB[Calibration Application]
        PEAK[Peak Detection]
        AVG[Sample Averaging]
    end
    
    subgraph "Advanced Processing"
        LINEAR[Linear Corrections]
        EXPO[Exponential Conversions]
        VALIDATION[Data Validation]
        ERROR[Error Detection]
    end
    
    subgraph "Output Formatting"
        CSV[CSV Formatting]
        JSON[JSON Conversion]
        MQTT[MQTT Publishing]
        STORAGE[Data Storage]
    end
    
    ADC --> TEMP_COMP
    I2C --> TEMP_COMP
    ANALOG --> TEMP_COMP
    
    TEMP_COMP --> CALIB
    CALIB --> PEAK
    PEAK --> AVG
    
    AVG --> LINEAR
    LINEAR --> EXPO
    EXPO --> VALIDATION
    VALIDATION --> ERROR
    
    ERROR --> CSV
    CSV --> JSON
    JSON --> MQTT
    CSV --> STORAGE
```

### Peak Detection Algorithm

The system implements a sophisticated peak detection algorithm applied across multiple sensor types to identify transient gas concentration events above baseline levels.

#### Algorithm Core Variables

**Signal Processing Variables:**
- `_signal`: Current peak signal strength (corrected voltage - baseline)
- `_base`: Baseline voltage level recorded when peak detection starts
- `_top`: Maximum signal strength reached during current peak event
- `_mem`: Memory of previous corrected voltage reading for derivative calculation
- `_dx`: First derivative (rate of change) = current_corrected_voltage - previous_memory
- `_trig`: Trigger threshold for peak detection start/end
- `_found`: Boolean flag indicating if a peak is currently being tracked

#### Peak Detection Process Flow

**1. Temperature Compensation Application:**
```cpp
*_volt_corr = *_volt_max - ((_temperature - _temperature_mem) * *_tcomp);
if (*_volt_corr < 0) { *_volt_corr = 0; }
```

**2. Derivative Calculation:**
```cpp
*_dx = *_volt_corr - *_mem;  // Rate of change calculation
```

**3. Peak Start Detection:**
```cpp
if ((*_dx >= *_trig) && (*_found == false)) {
    *_found = true;      // Mark peak as detected
    *_base = *_mem;      // Record baseline from previous reading
    *_top = 0;           // Reset peak maximum tracker
}
```

**4. Signal Strength Calculation:**
```cpp
*_signal = *_volt_corr - *_base;  // Peak magnitude above baseline
```

**5. Peak Maximum Tracking:**
```cpp
if ((*_found == true) && (*_signal > *_top)) {
    *_top = *_signal;    // Update peak maximum
}
```

**6. Peak End Detection:**
```cpp
// Peak ends when signal declines and drops to 10% of maximum
if ((*_dx <= *_trig) && (*_found == true) && (*_signal <= (*_top * 0.1))) {
    *_found = false;     // Reset peak detection
}
```

#### Sensor-Specific Peak Detection Applications

**Electrochemical Sensors:**
```cpp
peakDetect("h2s", &_h2s_corr, &_h2s_volt_max, &_h2s_signal, &_h2s_base, 
           &_h2s_top, &_h2s_mem, &_h2s_tcomp, &_h2s_dx, &_h2s_dx_trig, &_h2s_found);
```

**Infrared Sensors:**
```cpp
peakDetect("irco2", &_co2_corr, &_co2_volt_max, &_co2_signal, &_co2_base, 
           &_co2_top, &_co2_mem, &_co2_tcomp, &_co2_dx, &_co2_dx_trig, &_co2_found);
```

**VOC Sensors:**
```cpp
peakDetect("voc", &_voc_corr, &_voc_raw_max, &_voc_signal, &_voc_base, 
           &_voc_top, &_voc_mem, &_voc_tcomp, &_voc_dx, &_voc_dx_trig, &_voc_found);
```

### Temperature Compensation System

The system implements comprehensive temperature compensation across all sensor types to maintain accuracy across environmental temperature variations.

#### Universal Temperature Compensation Formula

**Core Compensation Equation:**
```cpp
corrected_value = raw_value - ((current_temperature - reference_temperature) × temperature_coefficient)
```

**Implementation in Calibration:**
```cpp
offset = measured_voltage - (temperature_differential × temp_compensation_factor);
if (offset < 0) { offset = 0; }  // Prevent negative offsets
```

#### Temperature Reference System

**Global Temperature Variables:**
- `_temperature_mem`: Reference temperature (typically 20°C)
- `_temperature_dx`: Temperature differential (current - reference)
- `_temperature_offset`: User-configurable temperature calibration offset

**Temperature Memory Management:**
```cpp
float dxtemp = _temperature - _temperature_mem;  // Calculate differential
```

#### Sensor-Specific Temperature Compensation

**Electrochemical Sensors:**
Each electrochemical sensor has its own temperature compensation coefficient stored in EEPROM:
- `_h2s_tcomp`: H2S temperature compensation (V/°C)
- `_o3_tcomp`: O3 temperature compensation (V/°C)
- `_so2_tcomp`: SO2 temperature compensation (V/°C)
- `_no2_tcomp`: NO2 temperature compensation (V/°C)
- `_nh3_tcomp`: NH3 temperature compensation (V/°C)

**Infrared Sensors:**
- `_co2_tcomp`: CO2 temperature compensation (V/°C)
- `_c1_tcomp`: CH4 temperature compensation (V/°C)
- `_pid_tcomp`: PID temperature compensation (V/°C)

**VOC Sensors:**
- `_voc_tcomp`: VOC temperature compensation coefficient
- Real-time temperature and humidity compensation for SGP40 sensors

#### Temperature Protection Mechanisms

**High Temperature Protection:**
```cpp
if (_temperature > 65) {  // 65°C threshold
    _bme_err = true;
    Serial.println("Warning: BME temperature too high, check the sensor");
}
```

**CPU Temperature Protection:**
```cpp
if (_cpu_temp >= 70) { 
    goSleep("CPU temp > 70°C, reboot in 1hr", 3600); 
}
```

### Calibration and Conversion Functions

The system implements multiple calibration methods to convert raw sensor voltages into meaningful concentration values.

#### Universal Sensor Conversion Formula

**Core Conversion Equation:**
```cpp
concentration = gain × (voltage - offset);
if (concentration < 0) { concentration = 0; }
```

**Engineering Mode Override:**
```cpp
if (_is_engineering) { 
    concentration = raw_voltage;  // Return raw values for diagnostics
}
```

#### Calibration Parameter Storage (EEPROM)

**EEPROM Page Allocation:**
- Page 1: C1 (CH4) sensor calibration
- Page 2: CO2 sensor calibration
- Page 3: PID sensor calibration
- Page 4: NH3 sensor calibration
- Page 5: H2S sensor calibration
- Page 6: NO2 sensor calibration
- Page 7: SO2 sensor calibration
- Page 8: O3 sensor calibration
- Page 9: Temperature compensation parameters
- Page 10: VOC sensor calibration

**Calibration Data Format:**
```cpp
sprintf(_EEpagebuffer, "SENSOR_NAME %u %f %f %f %f %f", 
        enable_flag, range, gain, offset_volt, temp_comp, trigger_threshold);
```

#### Linear Correction Functions (y = mx + b)

**Implementation Pattern:**
```cpp
if (sensor_linear_slope != 0) {
    corrected_value = sensor_linear_slope × original_value + sensor_linear_intercept;
}
```

**Electrochemical Sensor Linear Corrections:**
```cpp
if (_h2s_lin_m != 0) { _h2s = _h2s_lin_m × _h2s + _h2s_lin_b; }
if (_o3_lin_m != 0) { _o3 = _o3_lin_m × _o3 + _o3_lin_b; }
if (_so2_lin_m != 0) { _so2 = _so2_lin_m × _so2 + _so2_lin_b; }
if (_no2_lin_m != 0) { _no2 = _no2_lin_m × _no2 + _no2_lin_b; }
```

#### Exponential Conversion Functions

**VOC Exponential Model:**
```cpp
// Function: y = A × e^(b × x) + C
concentration = A × exp(b × raw_signal) + C;

// Overflow protection
if (b × raw_signal > 50) { b × raw_signal = 50; }
if (b × raw_signal < -50) { b × raw_signal = -50; }
```

**Default VOC Exponential Parameters:**
- A (Amplitude): 0.5177
- b (Rate): 0.0007834
- C (Offset): -0.03093

#### Infrared Sensor Statistical Calibration

**CO2 Linear Regression Calibration:**
```cpp
// Two-point calibration between offset voltage and full-scale
avgX = (offset_volt + 2.0) / 2.0;
avgY = (407.0 + range) / 2.0;  // 407ppm baseline CO2
slope = ssY / ssX;
intercept = avgY - (slope × avgX);
concentration = (slope × voltage) + intercept;
```

**CH4 Linear Regression Calibration:**
```cpp
// Two-point calibration with zero baseline
avgX = (offset_volt + 2.0) / 2.0;
avgY = (0 + range) / 2.0;
slope = ssY / ssX;
intercept = avgY - (slope × avgX);
concentration = (slope × voltage) + intercept;
```

### Data Validation and Error Detection

The system implements comprehensive data validation mechanisms to ensure measurement integrity and detect sensor malfunctions.

#### Error Flag System

**Bit-Mapped Error Encoding:**
```cpp
void formatError(void) {
    _errors = 0;
    
    // Bit 0 (0x01): Infrared sensors
    if (_ir_err || ((_ir_i2c == false) && (_c1_enable || _co2_enable || _pid_enable))) 
        _errors |= 1;
    
    // Bit 1 (0x02): MPS sensor
    if (_mps_err || ((_mps_i2c == false) && (_mps_enable))) 
        _errors |= 2;
    
    // Bit 2 (0x04): VOC sensors
    if (_voc_err || ((_voc_i2c == false) && (_voc_enable || _voc_mox_enable))) 
        _errors |= 4;
    
    // Bit 3 (0x08): NH3 sensors
    if (_nh3_err || communication_error_conditions) 
        _errors |= 8;
    
    // Bit 4 (0x10): Particulate matter sensors
    if (_pm_err > 0 || ((_pm_i2c == false) && (_pm_enable))) 
        _errors |= 16;
    
    // Bit 5 (0x20): Electrochemical sensors
    if (_spec_err || _o3_err || _so2_err || _no2_err || _h2s_err) 
        _errors |= 32;
    
    // Bit 6 (0x40): Real-time clock
    if (_rtc_err) 
        _errors |= 64;
    
    // Bit 7 (0x80): Environmental sensors
    if (_bme_err || grove_communication_error) 
        _errors |= 128;
}
```

#### Data Range Validation

**Negative Value Protection:**
```cpp
if (concentration < 0) { concentration = 0; }
if (corrected_voltage < 0) { corrected_voltage = 0; }
if (offset < 0) { offset = 0; }
```

**NaN Detection and Handling:**
```cpp
if (!isnan(sensor_reading)) {
    // Process valid reading
    accumulate_sample(sensor_reading);
} else {
    // Set error flag for invalid reading
    set_sensor_error_flag();
}
```

**Range Boundary Checking:**
```cpp
// Temperature range validation
if ((_temperature > -40) && (_temperature < 80)) {
    // Process valid temperature reading
} else {
    // Temperature out of valid range
    _bme_err = true;
}
```

#### Sample Averaging and Statistical Processing

**Environmental Sensor Averaging:**
```cpp
// Accumulation-based averaging
sensor_sum += raw_reading;
sample_count++;
final_value = sensor_sum / sample_count;
```

**Particulate Matter Averaging:**
```cpp
void formatPM(void) {
    if (_pm_sample_count > 0) {
        _pm1 = _pm1 / _pm_sample_count;
        _pm2_5 = _pm2_5 / _pm_sample_count;
        _pm10 = _pm10 / _pm_sample_count;
        _pmAQI = _pmAQI / _pm_sample_count;
    }
    _pm_sample_count = 0;  // Reset for next cycle
}
```

### Data Formatting and Structure

The system implements a comprehensive 69-field CSV data format with structured JSON conversion for MQTT transmission.

#### CSV Data Structure (69 Fields)

**Device and System Fields (0-7):**
```cpp
"ID, Time, LoopCount, EFormat, CPUTemp, VBat, SOC, CRate"
```

**Environmental Fields (8-14):**
```cpp
"Temp_err, Temp, Humidity, ATM, bmeAQI, WindSpeed, WindDir"
```

**Gas Sensor Fields (15-48):**
```cpp
"voc_err, VOC, VOC_max, EVOC, ECO2, pm_err, pm1.0, pm2.5, pm10, pmAQI, "
"ir_err, ir_C1, ir_C1_max, ir_CO2, ir_CO2_max, PID, PID_max, "
"mps_err, mps_c1, mps_c1_max, EC_err, H2S, H2S_max, O3, O3_max, SO2, SO2_max, "
"NO2, NO2_max, nh3_err, mos_CO, mos_NO2, NH3, NH3_max, Errors"
```

**Extended Diagnostic Fields (49-68):**
```cpp
"Temp_dx, Humidity_dx, "
"C1_signal, C1_base, C1_top, C1_dx, C1_found, "
"H2S_signal, H2S_base, H2S_top, H2S_dx, H2S_found, "
"VOC_signal, VOC_base, VOC_top, VOC_dx, VOC_found, "
"VOC_nocomp_max, summa_triggered"
```

#### Data Type Formatting Specifications

**Floating Point Precision:**
```cpp
sprintf(scratch, "%.1f", _cpu_temp);        // 1 decimal place for temperature
sprintf(scratch, "%.2f", _vbat);            // 2 decimal places for voltage
sprintf(scratch, "%0.4f", _h2s);            // 4 decimal places for gas concentrations
```

**Integer Formatting:**
```cpp
sprintf(scratch, "%lu", _time_stamp);       // Unsigned long for timestamps
sprintf(scratch, "%u", _errors);            // Unsigned int for error flags
```

#### JSON Conversion with Bounds Checking

**Safe Field Access:**
```cpp
const size_t EXPECTED_FIELDS = 69;
size_t field_count = parse_csv_fields(csv_string);

if (field_count >= 5) _mqtt_buffer["cpu_temp"]["value"] = atof(strings[4]);
if (field_count >= 6) _mqtt_buffer["battery_voltage"]["value"] = atof(strings[5]);
if (field_count >= 49) _mqtt_buffer["errors"]["value"] = atoi(strings[49]);
```

### Mathematical Operations and Equation Reference

#### Gas Concentration Calculations

**Electrochemical Sensor Equation:**
```
Concentration (ppm) = Gain (ppm/V) × (Corrected_Voltage - Offset_Voltage)

Where:
- Corrected_Voltage = Raw_Voltage - (Temperature_Differential × Temp_Coefficient)
- Gain = 1 / (Sensitivity × TIA_Resistance × 10^-6)
- Sensitivity in nA/ppm, TIA_Resistance in Ohms
```

**Infrared Sensor Linear Regression:**
```
Concentration = Slope × Voltage + Intercept

Where:
- Slope = (Y2 - Y1) / (X2 - X1)
- Intercept = Y_average - (Slope × X_average)
- Two-point calibration between offset and full-scale voltages
```

**VOC Exponential Conversion:**
```
Concentration = A × e^(b × Raw_Signal) + C

Default Parameters:
- A = 0.5177 (amplitude coefficient)
- b = 0.0007834 (exponential rate)
- C = -0.03093 (offset constant)
```

#### Air Quality Index (AQI) Calculation

**PM2.5 AQI Formula:**
```
AQI = ((C - C_low) / (C_high - C_low)) × (I_high - I_low) + I_low

Where:
- C = PM2.5 concentration (μg/m³)
- C_low, C_high = Concentration breakpoints
- I_low, I_high = AQI index breakpoints
```

**EPA AQI Breakpoints:**
| PM2.5 (μg/m³) | AQI | Category |
|---------------|-----|----------|
| 0.0 - 12.0 | 0 - 50 | Good |
| 12.1 - 35.4 | 51 - 100 | Moderate |
| 35.5 - 55.4 | 101 - 150 | Unhealthy for Sensitive |
| 55.5 - 150.4 | 151 - 200 | Unhealthy |
| 150.5 - 250.4 | 201 - 300 | Very Unhealthy |
| 250.5 - 500.4 | 301 - 500 | Hazardous |

#### Wind Speed Conversion

**Mechanical Wind Sensor:**
```
Wind_Speed (m/s) = Pulse_Count × (2.25 mph/pulse/second) × 0.44704 (m/s per mph)
                 = Pulse_Count × 1.00584 m/s per pulse per second
```

**Wind Direction Conversion:**
```
Direction (degrees) = (ADC_Voltage / 1.5V) × 359°

Where ADC_Voltage ranges from 0-1.5V for 0-359° direction
```

### Error Detection and Recovery Mechanisms

#### Sensor Communication Error Detection

**I2C Communication Validation:**
```cpp
if (i2c_transaction_result != expected_bytes) {
    sensor_error_flag = true;
    error_count++;
    trigger_recovery_procedure();
}
```

**Automatic Recovery Strategies:**

**Level 1 - Immediate Retry:**
```cpp
if (communication_error) {
    retry_sensor_reading();
    if (success) reset_error_count();
}
```

**Level 2 - Sensor Reinitialization:**
```cpp
if (error_count >= 3) {
    reinitialize_sensor();
    reset_communication_interface();
}
```

**Level 3 - Power Cycle Recovery:**
```cpp
if (error_count >= 5) {
    power_cycle_sensor();
    delay(stabilization_time);
    reinitialize_sensor();
}
```

**Level 4 - Sensor Disable:**
```cpp
if (error_count >= 10) {
    disable_sensor_permanently();
    log_critical_error();
}
```

#### Data Integrity Validation

**Checksum Validation (MPS Sensor):**
```cpp
// Validate MPS data packet integrity
if ((packet[0] == 0x30) && (packet[1] == 0x31) && 
    (packet[2] == 0x32) && (packet[3] == 0x33) && 
    (packet[4] == 0x35)) {
    // "Not ready" pattern detected
    return SENSOR_NOT_READY;
}
```

**Range Validation:**
```cpp
// Validate sensor readings within physical limits
if ((concentration >= 0) && (concentration <= sensor_max_range)) {
    accept_reading(concentration);
} else {
    reject_reading();
    increment_error_count();
}
```

### Performance Optimization and Computational Efficiency

#### Memory Management

**Fixed Buffer Allocation:**
```cpp
char scratch[2048];        // Temporary formatting buffer
char csvWBuffer[4096];     // Main CSV data buffer
```

**Efficient String Operations:**
```cpp
// Use sprintf for formatted conversion, strcat for concatenation
sprintf(scratch, "%.4f,%.4f,", sensor1_value, sensor2_value);
strcat(csvWBuffer, scratch);
```

#### Computational Optimization

**Minimize Floating Point Operations:**
```cpp
// Pre-calculate constants where possible
const float VOLTAGE_SCALE = 2.048 / 131071;  // ADC conversion factor
const float MPH_TO_MPS = 0.44704;            // Wind speed conversion
```

**Efficient Peak Detection:**
```cpp
// Single-pass algorithm with minimal memory usage
// O(1) space complexity, O(n) time complexity per sensor
```

#### Real-Time Performance Considerations

**Task Timing Management:**
```cpp
taskDelay(10, 1);  // Yield to OS during intensive operations
```

**Interrupt-Safe Operations:**
```cpp
// Atomic operations for shared variables
// Mutex protection for SD card access
```

This comprehensive data processing and algorithms documentation provides the mathematical foundation and implementation details necessary for accurate sensor data conversion, validation, and formatting in the air monitoring system.---


## Communication and Configuration

The air monitoring system implements a comprehensive communication and configuration framework that enables reliable data transmission, remote monitoring, and flexible system management. This section provides detailed documentation of networking protocols, data transmission methods, configuration management, and deployment procedures.

### Communication Architecture Overview

The system employs a multi-layered communication architecture with redundant connectivity options and robust error recovery mechanisms:

```mermaid
flowchart TD
    subgraph "Local System"
        SENSORS[Sensor Data Collection]
        CSV[CSV Data Storage]
        JSON[JSON Conversion]
        CONFIG[Configuration Management]
    end
    
    subgraph "Communication Layer"
        LTE[LTE Modem BG95]
        WIFI[WiFi Interface]
        GPS[GPS/GNSS]
        SSL[SSL/TLS Security]
    end
    
    subgraph "Network Services"
        MQTT[AWS IoT Core MQTT]
        NTP[Network Time Protocol]
        OTA[OTA Updates]
        CLOUD[Cloud Services]
    end
    
    subgraph "Remote Management"
        DASHBOARD[Monitoring Dashboard]
        COMMANDS[Remote Commands]
        ALERTS[Alert System]
        ANALYTICS[Data Analytics]
    end
    
    SENSORS --> CSV
    CSV --> JSON
    JSON --> LTE
    CONFIG --> LTE
    
    LTE --> SSL
    WIFI --> SSL
    GPS --> LTE
    
    SSL --> MQTT
    LTE --> NTP
    MQTT --> OTA
    
    MQTT --> DASHBOARD
    MQTT --> COMMANDS
    MQTT --> ALERTS
    MQTT --> ANALYTICS
```

### LTE Modem Management System

The system uses a **Quectel BG95-M3** LTE modem for primary cellular connectivity, providing robust data transmission and GPS/GNSS functionality.

#### Hardware Configuration and Control

**Pin Assignments:**
- **MODEM_PWR_EN (Pin 13)**: Power enable control (qTop board only)
- **MODEM_PWKEY (Pin 32)**: Power key toggle for modem on/off control
- **Serial Interface**: Hardware Serial 2 at 115200 baud rate

**Power Management:**
```cpp
void togglePWKEY(bool bWait = false) {
    Serial.println("Toggle BG95 PWKEY 1sec ...");
    digitalWrite(MODEM_PWKEY, HIGH);  // Assert power key
    delay(1000);                      // 1-second pulse duration
    digitalWrite(MODEM_PWKEY, LOW);   // Release power key
    
    if (bWait) { 
        Serial.println("Waiting 10sec for BG95 to initialize ...");
        delay(10000);  // Allow modem initialization
    }
}
```

#### Modem Initialization and Startup Sequence

**Phase 1: Hardware Initialization**
1. **Brownout Protection**: Disable ESP32 brownout detector during power-on
2. **Power Sequencing**: Enable modem power and toggle PWKEY
3. **Serial Setup**: Initialize UART communication at 115200 baud
4. **Modem Detection**: Verify modem response and firmware version

**Phase 2: Network Registration**
```cpp
// Network registration process
modem.sendAT("+CREG=1");  // Enable network registration notifications
if (modem.waitForNetwork(60000)) {  // 60-second timeout
    Serial.println("Network registered successfully");
    
    // Connect to GPRS with configured APN
    if (modem.gprsConnect(_apn)) {
        _is_lte_connected = true;
        _rssi = modem.getSignalQuality();
        Serial.printf("GPRS connected, RSSI: %d\n", _rssi);
    }
}
```

**Phase 3: Information Collection**
```cpp
// Collect modem identification information
_chip = modem.getModemInfo();     // Modem model and firmware version
_imei = modem.getIMEI();          // International Mobile Equipment Identity
_imsi = modem.getIMSI();          // International Mobile Subscriber Identity
_ccid = modem.getSimCCID();       // SIM card identifier
_rssi = modem.getSignalQuality(); // Signal strength indicator
```

#### Network Configuration and APN Management

**Supported Network Providers:**
- **Teal Network**: APN "teal" with eUICC ID retrieval
- **Telus Network**: APN "isp.telus.com"
- **Custom APNs**: Configurable via JSON configuration

**EID Retrieval for Teal Network:**
```cpp
String getEID(void) {
    // AT command sequence for eUICC ID extraction
    modem.sendAT("+CSIM=10,\"0070000000\"");
    modem.sendAT("+CSIM=42,\"01A4040010A0000005591010FFFFFFFF8900000200\"");
    modem.sendAT("+CSIM=10,\"81CA005A00\"");
    
    // Parse response to extract 32-character EID
    return parsed_eid;
}
```

#### Connection Management and Recovery

**Retry Strategy:**
```cpp
void setupConnection() {
    int lte_retry_count = 0;
    int mqtt_retry_count = 0;
    
    while (lte_retry_count < 10) {  // Maximum 10 LTE attempts
        if (lte_ON()) {  // Attempt LTE connection
            mqtt_retry_count = 0;
            
            while (mqtt_retry_count < 3) {  // Maximum 3 MQTT attempts
                if (setupMQTT(mqtt_retry_count)) {
                    return;  // Success - exit retry loops
                }
                mqtt_retry_count++;
                delay(60000);  // 1-minute delay between MQTT retries
            }
        }
        
        lte_retry_count++;
        lte_OFF();  // Power cycle modem
        delay(300000);  // 5-minute delay between LTE retries
    }
    
    // After 10 failures, turn off modem and restart cycle
    lte_OFF();
    lte_retry_count = 0;
}
```

### MQTT Protocol Implementation

The system implements **MQTT over SSL/TLS** for secure, bidirectional communication with **AWS IoT Core**.

#### SSL/TLS Certificate Management

**Certificate Architecture:**
```cpp
// Three-certificate authentication system
secure_layer.setCACert(AWS_CERT_CA);        // Amazon Root CA 1
secure_layer.setCertificate(AWS_CERT_CRT);   // Device-specific certificate
secure_layer.setPrivateKey(AWS_CERT_PRIVATE); // RSA private key
```

**Certificate Types:**
1. **Root CA Certificate**: Validates AWS IoT Core server authenticity
2. **Device Certificate**: Unique device identification for AWS IoT
3. **Private Key**: RSA key for SSL handshake and message encryption

#### MQTT Topic Structure and Messaging

**Topic Naming Convention:**
All topics follow the hierarchical pattern: `AM/{device_id}/{subtopic}`

```cpp
String buildTopic(const char *topic) {
    return String("AM/" + String(_device_id) + topic);
}
```

**Published Topics:**

| Topic | Purpose | Content Type | Frequency |
|-------|---------|--------------|-----------|
| `/sensors` | Primary sensor data | JSON | Per report cycle |
| `/status` | Device status and acknowledgments | JSON | On events |
| `/debug` | Debug messages and diagnostics | String | As needed |
| `/gps` | GPS location data | JSON | On GPS acquisition |
| `/config/device` | Device information | JSON | On connection |
| `/config/hardware` | Hardware configuration | JSON | On connection |
| `/config/server` | Server settings | JSON | On connection |
| `/config/network` | Network configuration | JSON | On connection |
| `/config/sampling` | Sampling parameters | JSON | On connection |
| `/errors` | Error conditions and diagnostics | String | On errors |

**Subscribed Topics:**
- `/cmd`: Command reception for remote control and configuration updates

#### MQTT Connection Management

**Connection Establishment:**
```cpp
bool setupMQTT(int fail_count) {
    if (_is_lte_connected || _network_wifi_enabled) {
        if (client->connect(_device_id)) {
            // Configure connection parameters
            client->setKeepAlive(60);      // 60-second heartbeat
            client->setBufferSize(2048);   // Large buffer for JSON payloads
            
            // Publish initial status and configuration
            client->publish(buildTopic("/status").c_str(), _version);
            publish_config();  // Send complete configuration
            publish_gps();     // Send GPS location data
            
            // Subscribe to command topic
            client->subscribe(buildTopic("/cmd").c_str());
            return true;
        }
    }
    return false;
}
```

**Connection Monitoring:**
```cpp
void serveMQTT(void) {
    if (!client->connected()) {
        if (client->connect(_device_id)) {
            client->subscribe(buildTopic("/cmd").c_str());
            Serial.println("MQTT Reconnected");
        }
    }
    client->loop();  // Process incoming messages
}
```

#### JSON Message Structure and Data Publishing

**Primary Sensor Data Message:**
```json
{
    "device_id": "AM-6003",
    "timestamp": 1698765432,
    "loopcounter": {"value": 1234},
    "cpu_temp": {"value": 45.6},
    "battery_voltage": {"value": 3.85},
    "battery_gauge": {"value": 87.3},
    "temperature": {"value": 22.5},
    "humidity": {"value": 65.2},
    "pressure": {"value": 1013.25},
    "bmeAQI": {"value": 42},
    "windSpeed": {"value": 2.3},
    "winDir": {"value": 180},
    "tvoc": {"value": 125},
    "pm2.5": {"value": 12.8},
    "pmAQI": {"value": 45},
    "ir_co2": {"value": 420},
    "h2s": {"value": 0.05},
    "ozone": {"value": 0.08},
    "so2": {"value": 0.02},
    "no2": {"value": 0.03},
    "errors": {"value": 0}
}
```

**Message Building Process:**
1. **CSV Parsing**: Convert 69-field CSV record to structured data
2. **Bounds Checking**: Validate field count and prevent buffer overruns
3. **Error Masking**: Use bit-masked error flags to determine data validity
4. **Conditional Inclusion**: Only include sensor data if sensors are operational
5. **JSON Serialization**: Convert structured data to optimized JSON format

#### Remote Command Processing

**Command Message Format:**
```json
{
    "msg": "command_name",
    "parameter1": "value1",
    "parameter2": "value2"
}
```

**Supported Command Categories:**

**System Commands:**
- `config`: Request configuration republish
- `reset`: System reset with optional delay
- `rstlog`: Clear data log files
- `csq`: Request signal quality report
- `gps`: Request GPS data publication

**Sensor Calibration Commands:**
- `configureSPEC`: Configure electrochemical sensors
- `calibrateSPEC`: Zero calibration for SPEC sensors
- `calibIR`: Zero calibration for infrared sensors
- `caltemp`: Temperature offset calibration

**SUMMA Canister Commands:**
- `summaReset`: Reset SUMMA trigger state
- `summaTest`: Run comprehensive tests
- `summaConfig`: Display current configuration

**Configuration Commands:**
- `updateSensorJson`: Update individual sensor parameters
- `eemode`: Toggle engineering mode

### Data Logging and Indexing System

The system implements a sophisticated dual-file logging system with binary indexing for efficient data storage and retrieval.

#### File Structure and Organization

**Primary Data Files:**
- **`/log.csv`**: Main sensor data in 69-field CSV format
- **`/log.idx`**: Binary index mapping CSV record positions
- **`/mark.idx`**: Transmission status markers (RAM-buffered)
- **`/config.json`**: System configuration (SD card + SPIFFS backup)

#### CSV Data Format (69 Fields)

**Field Categories:**
1. **Device Information (0-4)**: ID, timestamp, loop counter, engineering mode, CPU temperature
2. **Power Management (5-7)**: Battery voltage, state of charge, charge rate
3. **Environmental Data (8-14)**: Temperature, humidity, pressure, BME AQI, wind data
4. **Gas Sensors (15-47)**: VOC, PM, IR, MPS, electrochemical sensor readings
5. **Error Information (48-49)**: Comprehensive error flags and diagnostics
6. **Extended Diagnostics (50-68)**: Peak detection data, signal processing values

#### Binary Index System

**Index Record Structure:**
```cpp
struct IndexRecord {
    uint32_t csvPointer;    // File position in CSV file (4 bytes)
    uint8_t statusMarker;   // Transmission status (1 byte)
};
```

**Status Marker Values:**
- **0x55**: New record (not transmitted)
- **0xAA**: Successfully transmitted
- **0xA5**: Transmission error

**Index Operations:**
```cpp
void logIdx_Store(void) {
    uint32_t _idxPointer = _csvWIdx * sizeof(IndexRecord);
    byte _idxMarker = 0x55;  // New record marker
    
    if (xSemaphoreTake(sd_mutex, portMAX_DELAY)) {
        File idxFile = SD.open("/log.idx", FILE_WRITE);
        idxFile.seek(_idxPointer);
        idxFile.write((uint8_t*)&_csvPointer, sizeof(_csvPointer));
        idxFile.write((uint8_t*)&_idxMarker, sizeof(_idxMarker));
        idxFile.flush();
        idxFile.close();
        xSemaphoreGive(sd_mutex);
    }
}
```

#### Transmission Queue Management

**Queue Pointer System:**
- **`_csvWIdx`**: Write index (next record to write)
- **`_csvRIdx`**: Read index (next record to transmit)
- **`_csvPointer`**: Current CSV file position

**Data Transmission Flow:**
1. **Index Lookup**: Retrieve CSV file position for next untransmitted record
2. **Data Retrieval**: Read CSV record from calculated file position
3. **JSON Conversion**: Transform CSV data to JSON format
4. **MQTT Transmission**: Publish data to AWS IoT Core
5. **Status Update**: Mark record as successfully transmitted

**Recovery Mechanisms:**
```cpp
uint32_t logIdx_Fetch_422(int _idxPointer) {
    // Bidirectional search for untransmitted records
    
    // Forward search for new records
    for (int i = _csvRIdx_current; i < _csvWIdx; i++) {
        if (getMarkerStatus(i) == 0x55) {
            return getCsvPointer(i);  // Found untransmitted record
        }
    }
    
    // Backward search for missed records
    for (int i = _csvRIdx_current - 1; i >= 0; i--) {
        if (getMarkerStatus(i) == 0x55) {
            return getCsvPointer(i);  // Found missed record
        }
    }
    
    return -1;  // No untransmitted records found
}
```

### GPS/GNSS Time Synchronization

The system maintains accurate timestamps through multiple time sources with automatic synchronization and validation.

#### Time Source Hierarchy

**Priority Order:**
1. **GPS/GNSS Time**: Satellite-based, most accurate
2. **Network Time**: Cellular network-provided time
3. **External RTC**: Hardware real-time clock (DS3231)
4. **Internal RTC**: ESP32 built-in RTC with compile-time base

#### GPS/GNSS Implementation

**GNSS Configuration:**
```cpp
bool modem_gnss_on(void) {
    if (_is_gnss_on == false) {
        // Configure for USA (GPS + GLONASS)
        modem.sendAT(GF("+QGPSCFG=\"gnssconfig\",1"));
        
        // Set GPS priority over GLONASS
        modem.sendAT(GF("+QGPSCFG=\"priority\",0,0"));
        
        // Enable high-accuracy continuous acquisition
        modem.sendAT(GF("+QGPS=1,3,0"));
        
        _is_gnss_on = true;
        return true;
    }
    return false;
}
```

**GPS Data Collection:**
```cpp
bool get_gnss_data(void) {
    String gnss_str = modem.getGPSraw();  // Get raw NMEA data
    
    if (gnss_str.length() > 32) {
        // Parse NMEA fields: time, lat, lon, alt, quality, etc.
        _gnss_time = getfield(gnss_str, ',', 0);      // UTC time
        _gnss_latitude = getfield(gnss_str, ',', 1).toFloat();
        _gnss_longitude = getfield(gnss_str, ',', 2).toFloat();
        _gnss_altitude = getfield(gnss_str, ',', 4).toInt();
        _gnss_quality = getfield(gnss_str, ',', 5).toInt();
        _gnss_nsat = getfield(gnss_str, ',', 10).toInt();
        
        _is_gnss_ready = true;
        return true;
    }
    
    _is_gnss_ready = false;
    return false;
}
```

#### Time Synchronization Strategy

**Automatic Synchronization Triggers:**
1. **RTC Error Condition**: Immediate sync when `_rtc_err` is true
2. **Periodic Sync**: GPS time every 100 data collection cycles
3. **System Startup**: Initial synchronization during boot sequence
4. **Manual Request**: MQTT command-triggered synchronization

**Time Validation System:**
```cpp
bool isValidDateTime(DateTime dt) {
    DateTime compileTime = DateTime(F(__DATE__), F(__TIME__));
    uint16_t currentYear = compileTime.year();
    
    // Validation criteria
    if (dt.year() < currentYear || dt.year() > currentYear + 5) return false;
    if (dt.month() < 1 || dt.month() > 12) return false;
    if (dt.day() < 1 || dt.day() > 31) return false;
    if (dt.hour() > 23 || dt.minute() > 59 || dt.second() > 59) return false;
    
    return true;
}
```

### JSON Configuration Management

The system uses a comprehensive JSON configuration structure with dual storage and automatic fallback mechanisms.

#### Configuration Architecture

**Storage Locations:**
1. **Primary**: SD card (`/config.json`) - user-accessible
2. **Fallback**: SPIFFS flash memory (`/config.json`) - embedded backup

**Loading Process:**
```cpp
bool readConfig(void) {
    _config_id = 0;  // 0 = SD card, 1 = SPIFFS
    
    File SDconfig = SD.open("/config.json", "r");
    if (!SDconfig) {
        Serial.println("Failed to open config.json on SD card");
        _config_id = 1;
        
        File FSconfig = SPIFFS.open("/config.json", "r");
        if (!FSconfig) {
            Serial.println("Failed to open config.json on SPIFFS");
            return false;
        }
        deserializeJson(_config_flash, FSconfig);
        FSconfig.close();
    } else {
        deserializeJson(_config_flash, SDconfig);
        SDconfig.close();
    }
    
    return fetchConfig();  // Parse and validate configuration
}
```

#### Configuration Structure

**Device Information:**
```json
{
    "device": {
        "id": "AM-6003",
        "firmware": "4.40",
        "verbose_level": 1,
        "location": {
            "latitude": -97.59014,
            "longitude": 30.45492,
            "altitude": 171,
            "quality": 3,
            "label": "Terra",
            "group": "Main"
        },
        "owner": "TerraSLS"
    }
}
```

**Hardware Configuration:**
```json
{
    "hardware": {
        "weather_shield_version": 14,
        "panel_watts": 25,
        "battery": {
            "low_power_voltage": 3.4,
            "battery_ah": 30,
            "low_power_nap": "2:00"
        },
        "sensors": {
            "cal_temp": 20.0,
            "temp_offset": -3.2,
            "pm_enable": false,
            "voc_enable": false,
            "wind_enable": true,
            "h2s_enable": false,
            "o3_enable": false,
            "so2_enable": false,
            "no2_enable": false,
            "nh3_enable": false,
            "mps_enable": false
        }
    }
}
```

**Network Configuration:**
```json
{
    "network": {
        "wifi": {
            "enabled": false,
            "ssid": "TerraSLS",
            "password": "password"
        },
        "lte": {
            "enabled": true,
            "apn": "teal",
            "gps_enabled": true,
            "IMEI": "860111058858410",
            "RSSI": 33
        },
        "offline_mode": false
    }
}
```

**Server Configuration:**
```json
{
    "server": {
        "mqtt": {
            "host": "a1njj292w2vjt1-ats.iot.us-west-2.amazonaws.com",
            "port": 8883,
            "ssl": {
                "ca": "-----BEGIN CERTIFICATE-----...",
                "cert": "-----BEGIN CERTIFICATE-----...",
                "key": "-----BEGIN RSA PRIVATE KEY-----..."
            }
        },
        "update": {
            "endpoint": "airmonitor-utils.terrasls.com/softwareupdate",
            "automatic_updates": false
        }
    }
}
```

**Sampling Configuration:**
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

#### Configuration Validation and Error Handling

**Parameter Validation:**
1. **Required Fields**: Device ID must be present and non-empty
2. **Range Checking**: Numeric values validated against acceptable ranges
3. **Type Validation**: Ensures correct data types for all parameters
4. **Default Values**: Missing parameters receive safe default values
5. **Bounds Checking**: Prevents configurations that could cause system instability

**Error Recovery:**
- Invalid configuration triggers 30-minute sleep and retry
- Missing configuration files cause system reboot
- Corrupted JSON data results in fallback to SPIFFS or factory defaults

### Communication Protocol Reference

#### MQTT Message Formats

**Sensor Data Message Structure:**
```json
{
    "device_id": "string",
    "timestamp": "unix_timestamp",
    "sensor_name": {
        "value": "numeric_value",
        "unit": "measurement_unit",
        "quality": "data_quality_flag"
    }
}
```

**Command Message Structure:**
```json
{
    "msg": "command_name",
    "parameters": {
        "param1": "value1",
        "param2": "value2"
    },
    "timestamp": "unix_timestamp"
}
```

**Status Message Structure:**
```json
{
    "command": "executed_command",
    "ack": "boolean_success",
    "timestamp": "unix_timestamp",
    "details": "optional_details"
}
```

#### Error Reporting Protocol

**Error Flag Encoding:**
- **Bit 0 (0x01)**: Infrared sensors (CO2, CH4, PID)
- **Bit 1 (0x02)**: MPS (Micro Particle Sensor)
- **Bit 2 (0x04)**: VOC sensors (SGP30/SGP40)
- **Bit 3 (0x08)**: NH3 sensors
- **Bit 4 (0x10)**: Particulate matter sensors (PM1.0, PM2.5, PM10)
- **Bit 5 (0x20)**: Electrochemical sensors (H2S, O3, SO2, NO2)
- **Bit 6 (0x40)**: Real-time clock
- **Bit 7 (0x80)**: Environmental sensors (BME280/BME680)

### Deployment and Configuration Procedures

#### Initial System Deployment

**Phase 1: Hardware Setup**
1. **Physical Installation**: Mount device in appropriate enclosure
2. **Power Connection**: Connect solar panel and battery system
3. **Antenna Installation**: Install LTE and GPS antennas with proper orientation
4. **Sensor Mounting**: Install sensors according to manufacturer specifications

**Phase 2: Configuration Setup**
1. **SD Card Preparation**: Load configuration file with site-specific parameters
2. **Network Configuration**: Configure APN and MQTT broker settings
3. **Sensor Calibration**: Perform zero and span calibration for gas sensors
4. **GPS Acquisition**: Allow system to acquire GPS coordinates

**Phase 3: System Validation**
1. **Communication Test**: Verify MQTT connectivity and data transmission
2. **Sensor Validation**: Confirm all enabled sensors are operational
3. **Error Checking**: Review system error flags and resolve issues
4. **Data Quality**: Validate sensor readings against expected ranges

#### Remote Configuration Management

**Configuration Update Process:**
1. **MQTT Command**: Send `updateSensorJson` command with parameter updates
2. **Validation**: System validates new parameters against acceptable ranges
3. **Application**: Parameters applied immediately to running system
4. **Persistence**: Configuration saved to both SD card and SPIFFS
5. **Acknowledgment**: System sends confirmation of successful update

**Calibration Procedures:**
1. **Zero Calibration**: Expose sensors to clean air and send calibration command
2. **Span Calibration**: Apply certified reference gas and configure sensor parameters
3. **Temperature Compensation**: Adjust temperature coefficients based on field conditions
4. **Validation**: Verify calibration accuracy through test measurements

#### System Monitoring and Maintenance

**Real-Time Monitoring:**
- **Data Transmission**: Continuous sensor data publishing every report cycle
- **System Status**: Regular status updates including battery, signal strength, errors
- **GPS Location**: Periodic location updates for mobile deployments
- **Error Reporting**: Immediate notification of sensor failures or communication issues

**Maintenance Procedures:**
- **Remote Diagnostics**: Use MQTT commands to check system health
- **Configuration Updates**: Modify sampling intervals, sensor enables, calibration parameters
- **Firmware Updates**: OTA firmware updates for bug fixes and feature enhancements
- **Data Recovery**: Retrieve stored data during communication outages

This comprehensive communication and configuration documentation provides the foundation for reliable system deployment, operation, and maintenance of the air monitoring platform.---


## Implementation Guide and Reference Materials

This section provides practical guidance for implementing an equivalent air monitoring system, including quick reference guides, migration considerations, compatibility requirements, and code implementation templates.

### Quick Reference Guides

#### System Architecture Quick Reference

**Core Components:**
- **Microcontroller**: ESP32 Feather (dual-core Xtensa LX6)
- **Operating System**: FreeRTOS with 3 primary tasks
- **Communication**: Quectel BG95-M3 LTE modem with GPS
- **Storage**: SD card (primary) + SPIFFS (backup)
- **I2C Architecture**: TCA9548 multiplexer with 8 sensor channels

**Task Architecture:**
```cpp
// Task 1: Data Collection (Core 0, Priority 1, 40KB stack)
xTaskCreatePinnedToCore(collectDataTask, "CollectData", 40960, NULL, 1, NULL, 0);

// Task 2: Data Transmission (Core 0, Priority 2, 40KB stack)  
xTaskCreatePinnedToCore(sendDataTask, "SendData", 40960, NULL, 2, NULL, 0);

// Task 3: Serial Proxy (Core 1, Priority 1, 4KB stack)
xTaskCreatePinnedToCore(serialModemProxyTask, "SerialModemProxy", 4096, NULL, 1, NULL, 1);
```

#### I2C Device Address Map

| Address | Device | Function | TCA Channel |
|---------|--------|----------|-------------|
| 0x70 | TCA9548 | I2C Multiplexer | - |
| 0x39 | PCF8574 IC8 | User interface control | - |
| 0x3A | PCF8574 IC113 | Power management | - |
| 0x3B | PCF8574 IC102 | SPEC sensor control | - |
| 0x38 | PCF8574 IC7 | Wind speed sensor | - |
| 0x50 | EEPROM IC113 | Weather Shield calibration | - |
| 0x52 | EEPROM | FeatherBase calibration | - |
| 0x4B | ADS1115 | Battery monitoring | - |
| 0x76 | BME280/680 | Environmental sensors | 0, 3 |
| 0x6B | LMP91000 | Electrochemical ADC | 1 |
| 0x59 | SGP40 | VOC sensor | 2 |
| 0x58 | SGP30 | VOC sensor (alternative) | 2 |
| 0x69 | SPS30 | Particulate matter | 4 |
| 0x40 | ATTiny MPS | Methane detection | 5 |
| 0x6C | MCP342x | Infrared sensor ADC | 6 |
| 0x6A | MCP342x | NH3 sensor ADC | 7 |
| 0x15 | Calypso | Ultrasonic wind sensor | - |

#### Sensor Calibration Quick Reference

**Electrochemical Sensors (H2S, O3, SO2, NO2, NH3):**
```cpp
// Zero calibration in clean air
bool zeroSPEC(byte model) {
    float dxtemp = _temperature - _temperature_mem;
    float offset = measured_voltage - (dxtemp * temp_compensation);
    if (offset < 0) offset = 0;
    // Store offset for specific sensor
    return true;
}

// Concentration calculation
float concentration = gain * (corrected_voltage - offset);
if (concentration < 0) concentration = 0;
```

**Infrared Sensors (CO2, CH4, PID):**
```cpp
// Linear regression calibration
avgX = (offset_volt + 2.0) / 2.0;
avgY = (baseline + range) / 2.0;
slope = ssY / ssX;
intercept = avgY - (slope * avgX);
concentration = (slope * voltage) + intercept;
```

**VOC Sensors (SGP40):**
```cpp
// Exponential conversion: y = A * e^(b * x) + C
concentration = A * exp(b * raw_signal) + C;

// Default parameters:
// A = 0.5177, b = 0.0007834, C = -0.03093
```

#### Configuration File Structure

**Essential Configuration Sections:**
```json
{
    "device": {
        "id": "AM-XXXX",
        "firmware": "4.40"
    },
    "hardware": {
        "weather_shield_version": 14,
        "sensors": {
            "cal_temp": 20.0,
            "temp_offset": 0.0,
            "sensor_enable_flags": "..."
        }
    },
    "network": {
        "lte": {
            "enabled": true,
            "apn": "teal",
            "gps_enabled": true
        }
    },
    "server": {
        "mqtt": {
            "host": "your-iot-endpoint.amazonaws.com",
            "port": 8883
        }
    },
    "sampling": {
        "sampling_interval_sec": 2,
        "report_interval_count": 15,
        "sleep_time_sec": 0
    }
}
```

#### MQTT Topic Reference

**Published Topics:**
- `AM/{device_id}/sensors` - Primary sensor data (JSON)
- `AM/{device_id}/status` - Device status and acknowledgments
- `AM/{device_id}/config/device` - Device information
- `AM/{device_id}/config/hardware` - Hardware configuration
- `AM/{device_id}/gps` - GPS location data
- `AM/{device_id}/errors` - Error conditions

**Subscribed Topics:**
- `AM/{device_id}/cmd` - Remote commands

**Command Examples:**
```json
{"msg": "config"}                    // Request configuration republish
{"msg": "reset", "delay": 30}        // System reset with delay
{"msg": "calibrateSPEC", "model": 0} // Zero calibration for H2S
{"msg": "updateSensorJson", "key": "h2s_gain", "value": 39.3}
```

### Migration Considerations

#### Hardware Migration Path

**From Legacy Systems:**
1. **Sensor Compatibility**: Most sensors use standard I2C/analog interfaces
2. **Power Requirements**: Ensure adequate power supply (3.3V, 5V rails)
3. **Mechanical Integration**: Weather Shield form factor considerations
4. **Antenna Placement**: LTE and GPS antennas require proper positioning

**Hardware Version Dependencies:**
```cpp
// Weather Shield version handling
if (_WS_version == 14) {
    // CAN-based sensors, different NH3 handling
    _nh3_can_enable = true;
} else {
    // Standard I2C sensor configuration
    _nh3_can_enable = false;
}
```

#### Software Migration Strategy

**Phase 1: Core System Implementation**
1. **FreeRTOS Task Structure**: Implement 3-task architecture
2. **I2C Communication**: TCA multiplexer and device management
3. **Basic Sensor Reading**: Start with environmental sensors (BME280)
4. **Data Storage**: CSV logging with binary indexing
5. **Configuration System**: JSON configuration loading and validation

**Phase 2: Communication Implementation**
1. **LTE Modem Integration**: Quectel BG95 AT command interface
2. **MQTT Protocol**: SSL/TLS with AWS IoT Core
3. **GPS/GNSS**: Time synchronization and location services
4. **OTA Updates**: Firmware update capability

**Phase 3: Advanced Sensor Integration**
1. **Electrochemical Sensors**: LMP91000 potentiostat interface
2. **Infrared Sensors**: MCP342x ADC integration
3. **Particulate Matter**: SPS30 sensor with power management
4. **VOC Sensors**: SGP40/SGP30 with temperature compensation
5. **Peak Detection**: Advanced signal processing algorithms

**Phase 4: System Optimization**
1. **Power Management**: Sleep modes and battery optimization
2. **Error Recovery**: Robust error handling and sensor recovery
3. **Calibration System**: EEPROM storage and calibration procedures
4. **Performance Tuning**: Memory optimization and timing adjustments

#### Code Migration Templates

**Basic Sensor Reading Template:**
```cpp
class SensorInterface {
private:
    bool _enabled;
    bool _error;
    float _value;
    float _sum;
    unsigned short _sample_count;
    
public:
    virtual bool initialize() = 0;
    virtual bool readSensor() = 0;
    virtual void processData() = 0;
    virtual void clearData() = 0;
    
    bool isEnabled() { return _enabled; }
    bool hasError() { return _error; }
    float getValue() { return _value; }
};

class BME280Sensor : public SensorInterface {
public:
    bool initialize() override {
        // Initialize BME280 via I2C
        return bme280.begin(0x76);
    }
    
    bool readSensor() override {
        if (!_enabled) return false;
        
        float temp = bme280.readTemperature();
        if (!isnan(temp)) {
            _sum += temp;
            _sample_count++;
            return true;
        }
        
        _error = true;
        return false;
    }
    
    void processData() override {
        if (_sample_count > 0) {
            _value = _sum / _sample_count;
        }
    }
    
    void clearData() override {
        _sum = 0;
        _sample_count = 0;
        _error = false;
    }
};
```

**I2C Communication Template:**
```cpp
class I2CManager {
private:
    SemaphoreHandle_t i2c_mutex;
    
public:
    bool selectTCAChannel(uint8_t channel) {
        if (xSemaphoreTake(i2c_mutex, portMAX_DELAY)) {
            Wire.beginTransmission(0x70);  // TCA9548 address
            Wire.write(1 << channel);      // Select channel
            uint8_t result = Wire.endTransmission();
            xSemaphoreGive(i2c_mutex);
            return (result == 0);
        }
        return false;
    }
    
    bool readI2CDevice(uint8_t address, uint8_t* buffer, size_t length) {
        if (xSemaphoreTake(i2c_mutex, portMAX_DELAY)) {
            Wire.requestFrom(address, length);
            size_t bytesRead = 0;
            while (Wire.available() && bytesRead < length) {
                buffer[bytesRead++] = Wire.read();
            }
            xSemaphoreGive(i2c_mutex);
            return (bytesRead == length);
        }
        return false;
    }
};
```

**Configuration Management Template:**
```cpp
class ConfigManager {
private:
    JsonDocument config_doc;
    bool config_loaded;
    
public:
    bool loadConfiguration() {
        // Try SD card first
        File configFile = SD.open("/config.json", "r");
        if (!configFile) {
            // Fallback to SPIFFS
            configFile = SPIFFS.open("/config.json", "r");
            if (!configFile) {
                return false;
            }
        }
        
        DeserializationError error = deserializeJson(config_doc, configFile);
        configFile.close();
        
        if (error) {
            Serial.println("Failed to parse config.json");
            return false;
        }
        
        config_loaded = true;
        return validateConfiguration();
    }
    
    bool validateConfiguration() {
        // Validate required fields
        if (!config_doc["device"]["id"].is<String>()) {
            Serial.println("Missing device ID");
            return false;
        }
        
        // Apply defaults for missing values
        if (!config_doc["sampling"]["interval"].is<int>()) {
            config_doc["sampling"]["interval"] = 2;
        }
        
        return true;
    }
    
    template<typename T>
    T getConfigValue(const char* path, T defaultValue) {
        if (!config_loaded) return defaultValue;
        
        JsonVariant value = config_doc[path];
        if (value.is<T>()) {
            return value.as<T>();
        }
        return defaultValue;
    }
};
```

### Compatibility Requirements

#### Hardware Compatibility Matrix

**Microcontroller Requirements:**
- **ESP32 Feather**: Dual-core processor with FreeRTOS support
- **Memory**: Minimum 4MB flash, 520KB RAM
- **I2C**: Hardware I2C with configurable pins
- **Serial**: Multiple UART interfaces for modem communication
- **GPIO**: Sufficient pins for power control and status indication

**Sensor Interface Requirements:**
- **I2C Devices**: 3.3V logic level, standard I2C timing
- **ADC Resolution**: 18-bit for electrochemical sensors (MCP342x)
- **Power Control**: GPIO-controlled power switching capability
- **Analog Inputs**: 12-bit ADC for wind direction measurement

**Communication Requirements:**
- **LTE Modem**: Quectel BG95 or compatible AT command interface
- **GPS/GNSS**: Integrated or external GPS receiver
- **Antenna Connections**: U.FL or SMA connectors for LTE/GPS antennas

#### Software Dependencies

**Core Libraries:**
```cpp
// Essential Arduino/ESP32 libraries
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Communication libraries
#include <PubSubClient.h>
#include <SSLClient.h>
#include <TinyGsmClient.h>

// Sensor libraries
#include <Adafruit_BME280.h>
#include <Adafruit_BME680.h>
#include <RTClib.h>
```

**Version Compatibility:**
- **Arduino Core**: ESP32 Arduino Core 2.0.x or later
- **ArduinoJson**: Version 6.x for configuration management
- **PubSubClient**: Version 2.8.x for MQTT communication
- **TinyGSM**: Version 0.11.x for modem communication

#### Network and Protocol Requirements

**LTE Network Compatibility:**
- **Frequency Bands**: Support for local LTE bands
- **APN Configuration**: Carrier-specific APN settings
- **Data Plans**: Unlimited or high-volume data plans recommended

**MQTT Broker Requirements:**
- **Protocol Version**: MQTT 3.1.1 or 5.0
- **SSL/TLS**: TLS 1.2 minimum for security
- **Message Size**: Support for 2KB+ message payloads
- **QoS Levels**: QoS 0 and 1 support required

**Time Synchronization:**
- **GPS/GNSS**: GPS, GLONASS, or multi-constellation support
- **Network Time**: Cellular network time protocol support
- **RTC Backup**: Hardware RTC with battery backup recommended

### Code Implementation Examples

#### Complete Sensor Reading Implementation

```cpp
class AirMonitorSensorManager {
private:
    struct SensorData {
        float temperature;
        float humidity;
        float pressure;
        float h2s;
        float o3;
        float so2;
        float no2;
        float co2;
        float pm25;
        float voc;
        uint16_t errors;
    };
    
    SensorData current_data;
    I2CManager i2c_mgr;
    ConfigManager config_mgr;
    
public:
    bool initializeSensors() {
        bool success = true;
        
        // Initialize I2C and TCA multiplexer
        Wire.begin();
        if (!i2c_mgr.selectTCAChannel(0)) {
            Serial.println("TCA multiplexer initialization failed");
            success = false;
        }
        
        // Initialize environmental sensor
        if (config_mgr.getConfigValue("hardware/sensors/bme_enable", false)) {
            i2c_mgr.selectTCAChannel(0);
            if (!bme280.begin(0x76)) {
                Serial.println("BME280 initialization failed");
                current_data.errors |= 0x80;
                success = false;
            }
        }
        
        // Initialize electrochemical sensors
        if (config_mgr.getConfigValue("hardware/sensors/h2s_enable", false)) {
            i2c_mgr.selectTCAChannel(1);
            if (!initializeECSensor(0)) {  // H2S = model 0
                Serial.println("H2S sensor initialization failed");
                current_data.errors |= 0x20;
                success = false;
            }
        }
        
        return success;
    }
    
    bool collectAllSensors() {
        bool success = true;
        
        // Collect environmental data
        if (!(current_data.errors & 0x80)) {
            i2c_mgr.selectTCAChannel(0);
            current_data.temperature = bme280.readTemperature();
            current_data.humidity = bme280.readHumidity();
            current_data.pressure = bme280.readPressure() / 100.0;
            
            if (isnan(current_data.temperature)) {
                current_data.errors |= 0x80;
                success = false;
            }
        }
        
        // Collect electrochemical sensor data
        if (!(current_data.errors & 0x20)) {
            i2c_mgr.selectTCAChannel(1);
            current_data.h2s = readECSensor(0);  // H2S
            current_data.o3 = readECSensor(2);   // O3
            current_data.so2 = readECSensor(3);  // SO2
            current_data.no2 = readECSensor(1);  // NO2
        }
        
        return success;
    }
    
private:
    float readECSensor(uint8_t model) {
        // Read ADC value
        uint32_t adc_count = readMCP342x(0x6B, model);
        float voltage = (adc_count * 2.048) / 131071.0;
        
        // Apply temperature compensation
        float temp_diff = current_data.temperature - 20.0;  // Reference temp
        float temp_comp = getTemperatureCompensation(model);
        float corrected_voltage = voltage - (temp_diff * temp_comp);
        
        // Apply calibration
        float gain = getCalibrationGain(model);
        float offset = getCalibrationOffset(model);
        float concentration = gain * (corrected_voltage - offset);
        
        return (concentration < 0) ? 0 : concentration;
    }
    
    uint32_t readMCP342x(uint8_t address, uint8_t channel) {
        uint8_t config = 0x80 | (channel << 5) | 0x0C;  // 18-bit, one-shot
        
        Wire.beginTransmission(address);
        Wire.write(config);
        Wire.endTransmission();
        
        delay(100);  // Conversion time
        
        Wire.requestFrom(address, 4);
        uint32_t result = 0;
        for (int i = 0; i < 3; i++) {
            result = (result << 8) | Wire.read();
        }
        Wire.read();  // Discard config byte
        
        return result;
    }
};
```

#### MQTT Communication Implementation

```cpp
class MQTTManager {
private:
    PubSubClient* mqtt_client;
    String device_id;
    JsonDocument sensor_message;
    
public:
    bool initialize(const char* host, int port, const char* device_id) {
        this->device_id = String(device_id);
        
        // Initialize SSL client with certificates
        ssl_client.setCACert(AWS_CERT_CA);
        ssl_client.setCertificate(AWS_CERT_CRT);
        ssl_client.setPrivateKey(AWS_CERT_PRIVATE);
        
        mqtt_client = new PubSubClient(host, port, mqttCallback, ssl_client);
        mqtt_client->setBufferSize(2048);
        
        return connect();
    }
    
    bool connect() {
        if (mqtt_client->connect(device_id.c_str())) {
            mqtt_client->setKeepAlive(60);
            
            // Subscribe to command topic
            String cmd_topic = "AM/" + device_id + "/cmd";
            mqtt_client->subscribe(cmd_topic.c_str());
            
            // Publish connection status
            String status_topic = "AM/" + device_id + "/status";
            mqtt_client->publish(status_topic.c_str(), "Connected");
            
            return true;
        }
        return false;
    }
    
    bool publishSensorData(const SensorData& data) {
        // Build JSON message
        sensor_message.clear();
        sensor_message["device_id"] = device_id;
        sensor_message["timestamp"] = getCurrentTimestamp();
        
        // Add sensor readings
        sensor_message["temperature"]["value"] = data.temperature;
        sensor_message["humidity"]["value"] = data.humidity;
        sensor_message["pressure"]["value"] = data.pressure;
        sensor_message["h2s"]["value"] = data.h2s;
        sensor_message["o3"]["value"] = data.o3;
        sensor_message["so2"]["value"] = data.so2;
        sensor_message["no2"]["value"] = data.no2;
        sensor_message["errors"]["value"] = data.errors;
        
        // Serialize and publish
        String json_string;
        serializeJson(sensor_message, json_string);
        
        String topic = "AM/" + device_id + "/sensors";
        return mqtt_client->publish(topic.c_str(), json_string.c_str());
    }
    
    void loop() {
        if (!mqtt_client->connected()) {
            connect();
        }
        mqtt_client->loop();
    }
    
private:
    static void mqttCallback(char* topic, byte* payload, unsigned int length) {
        // Parse incoming commands
        JsonDocument command_doc;
        deserializeJson(command_doc, payload, length);
        
        const char* command = command_doc["msg"];
        
        // Process commands
        if (strcmp(command, "reset") == 0) {
            int delay_sec = command_doc["delay"] | 0;
            scheduleSystemReset(delay_sec);
        } else if (strcmp(command, "config") == 0) {
            publishConfiguration();
        }
        
        // Send acknowledgment
        publishAcknowledgment(command, true);
    }
};
```

#### Data Logging Implementation

```cpp
class DataLogger {
private:
    SemaphoreHandle_t sd_mutex;
    uint32_t write_index;
    uint32_t read_index;
    
public:
    bool initialize() {
        sd_mutex = xSemaphoreCreateMutex();
        
        if (!SD.begin()) {
            Serial.println("SD card initialization failed");
            return false;
        }
        
        // Create log files if they don't exist
        createLogFiles();
        
        // Load current indices
        loadIndices();
        
        return true;
    }
    
    bool logSensorData(const SensorData& data) {
        if (xSemaphoreTake(sd_mutex, portMAX_DELAY)) {
            // Build CSV record
            String csv_record = buildCSVRecord(data);
            
            // Write to CSV file
            File csv_file = SD.open("/log.csv", FILE_APPEND);
            uint32_t file_position = csv_file.position();
            csv_file.println(csv_record);
            csv_file.close();
            
            // Write index entry
            writeIndexEntry(write_index, file_position, 0x55);  // 0x55 = new
            
            write_index++;
            xSemaphoreGive(sd_mutex);
            return true;
        }
        return false;
    }
    
    String getNextRecord() {
        if (xSemaphoreTake(sd_mutex, portMAX_DELAY)) {
            // Find next untransmitted record
            uint32_t file_position = findNextUntransmitted();
            
            if (file_position != 0xFFFFFFFF) {
                File csv_file = SD.open("/log.csv", FILE_READ);
                csv_file.seek(file_position);
                String record = csv_file.readStringUntil('\n');
                csv_file.close();
                
                xSemaphoreGive(sd_mutex);
                return record;
            }
            
            xSemaphoreGive(sd_mutex);
        }
        return "";
    }
    
    void markRecordTransmitted(uint32_t index) {
        if (xSemaphoreTake(sd_mutex, portMAX_DELAY)) {
            writeIndexMarker(index, 0xAA);  // 0xAA = transmitted
            xSemaphoreGive(sd_mutex);
        }
    }
    
private:
    String buildCSVRecord(const SensorData& data) {
        String record = "";
        record += String(getCurrentTimestamp()) + ",";
        record += String(data.temperature, 2) + ",";
        record += String(data.humidity, 2) + ",";
        record += String(data.pressure, 2) + ",";
        record += String(data.h2s, 4) + ",";
        record += String(data.o3, 4) + ",";
        record += String(data.so2, 4) + ",";
        record += String(data.no2, 4) + ",";
        record += String(data.errors);
        return record;
    }
    
    void writeIndexEntry(uint32_t index, uint32_t position, uint8_t marker) {
        File idx_file = SD.open("/log.idx", FILE_WRITE);
        uint32_t offset = index * 5;  // 4 bytes position + 1 byte marker
        
        idx_file.seek(offset);
        idx_file.write((uint8_t*)&position, 4);
        idx_file.write(&marker, 1);
        idx_file.close();
    }
};
```

### Performance Optimization Guidelines

#### Memory Management Best Practices

**Stack Size Allocation:**
```cpp
// Task stack sizes based on actual usage
#define DATA_COLLECTION_STACK_SIZE  40960  // 40KB for sensor processing
#define DATA_TRANSMISSION_STACK_SIZE 40960  // 40KB for MQTT/JSON operations
#define SERIAL_PROXY_STACK_SIZE     4096   // 4KB for simple proxy operations
```

**Dynamic Memory Usage:**
```cpp
// Avoid frequent malloc/free operations
// Use static buffers where possible
static char csv_buffer[2048];
static char json_buffer[2048];

// Use object pools for frequently allocated objects
class JsonDocumentPool {
private:
    JsonDocument documents[5];
    bool in_use[5];
    
public:
    JsonDocument* acquire() {
        for (int i = 0; i < 5; i++) {
            if (!in_use[i]) {
                in_use[i] = true;
                documents[i].clear();
                return &documents[i];
            }
        }
        return nullptr;  // Pool exhausted
    }
    
    void release(JsonDocument* doc) {
        for (int i = 0; i < 5; i++) {
            if (&documents[i] == doc) {
                in_use[i] = false;
                break;
            }
        }
    }
};
```

#### Power Optimization Strategies

**Sleep Mode Implementation:**
```cpp
void enterDeepSleep(uint64_t sleep_duration_sec) {
    // Prepare for sleep
    pauseDataCollection();
    setSampling_OFF();
    
    // Put sensors in sleep mode
    sleepPM();    // Particulate matter sensor
    sleepMPS();   // Micro particle sensor
    sleepVOC();   // VOC sensor heater off
    
    // Disable GPS/GNSS
    modem_gnss_off();
    
    // Turn off sensor power
    shieldOFF();
    
    // Configure wake-up timer
    esp_sleep_enable_timer_wakeup(sleep_duration_sec * 1000000ULL);
    
    // Enter deep sleep
    esp_deep_sleep_start();
}
```

**Sensor Power Management:**
```cpp
class PowerManager {
private:
    bool sensors_powered;
    unsigned long last_activity;
    
public:
    void enableSensorPower() {
        if (!sensors_powered) {
            set3VP_ON();           // Enable 3.3V rail
            delay(100);            // Stabilization delay
            
            setIR_ON();            // Infrared sensors
            setPM_ON();            // Particulate matter
            setMPS_ON();           // MPS sensor
            
            sensors_powered = true;
            last_activity = millis();
        }
    }
    
    void disableSensorPower() {
        if (sensors_powered) {
            setIR_OFF();
            setPM_OFF();
            setMPS_OFF();
            shieldOFF();           // Disable all sensor power
            
            sensors_powered = false;
        }
    }
    
    void checkPowerTimeout() {
        if (sensors_powered && (millis() - last_activity > 300000)) {  // 5 minutes
            Serial.println("Sensor power timeout - entering sleep mode");
            disableSensorPower();
        }
    }
};
```

#### Communication Optimization

**MQTT Message Batching:**
```cpp
class MessageBatcher {
private:
    JsonDocument batch_message;
    int message_count;
    unsigned long last_send;
    
public:
    void addSensorReading(const SensorData& data) {
        batch_message["readings"][message_count] = createSensorJson(data);
        message_count++;
        
        // Send batch when full or timeout reached
        if (message_count >= 10 || (millis() - last_send > 60000)) {
            sendBatch();
        }
    }
    
private:
    void sendBatch() {
        if (message_count > 0) {
            batch_message["count"] = message_count;
            batch_message["timestamp"] = getCurrentTimestamp();
            
            String json_string;
            serializeJson(batch_message, json_string);
            
            mqtt_client->publish("AM/device/batch", json_string.c_str());
            
            // Reset batch
            batch_message.clear();
            message_count = 0;
            last_send = millis();
        }
    }
};
```

### Troubleshooting Guide

#### Common Issues and Solutions

**I2C Communication Failures:**
```cpp
// Diagnostic function for I2C issues
void diagnoseI2CIssues() {
    Serial.println("I2C Diagnostic Report:");
    
    // Test TCA multiplexer
    Wire.beginTransmission(0x70);
    uint8_t tca_result = Wire.endTransmission();
    Serial.printf("TCA9548 (0x70): %s\n", (tca_result == 0) ? "OK" : "FAILED");
    
    // Test each TCA channel
    for (int channel = 0; channel < 8; channel++) {
        Wire.beginTransmission(0x70);
        Wire.write(1 << channel);
        Wire.endTransmission();
        
        // Scan for devices on this channel
        Serial.printf("Channel %d devices: ", channel);
        for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.printf("0x%02X ", addr);
            }
        }
        Serial.println();
    }
}
```

**Memory Leak Detection:**
```cpp
void monitorMemoryUsage() {
    static unsigned long last_check = 0;
    static size_t last_free_heap = 0;
    
    if (millis() - last_check > 10000) {  // Check every 10 seconds
        size_t current_free = ESP.getFreeHeap();
        size_t min_free = ESP.getMinFreeHeap();
        
        Serial.printf("Heap: Current=%u, Min=%u, Change=%d\n", 
                     current_free, min_free, (int)(current_free - last_free_heap));
        
        if (current_free < 50000) {  // Less than 50KB free
            Serial.println("WARNING: Low memory condition");
        }
        
        last_free_heap = current_free;
        last_check = millis();
    }
}
```

**Sensor Calibration Validation:**
```cpp
bool validateSensorCalibration(uint8_t sensor_model) {
    float test_readings[10];
    float sum = 0;
    
    // Take multiple readings in clean air
    for (int i = 0; i < 10; i++) {
        test_readings[i] = readSensorValue(sensor_model);
        sum += test_readings[i];
        delay(1000);
    }
    
    float average = sum / 10.0;
    float variance = 0;
    
    // Calculate variance
    for (int i = 0; i < 10; i++) {
        variance += pow(test_readings[i] - average, 2);
    }
    variance /= 10.0;
    
    Serial.printf("Sensor %d - Average: %.4f, Variance: %.6f\n", 
                  sensor_model, average, variance);
    
    // Check if readings are stable (low variance) and near zero
    return (variance < 0.001 && abs(average) < 0.1);
}
```

This comprehensive implementation guide provides the practical foundation needed to recreate the air monitoring system with equivalent functionality, performance, and reliability characteristics.