# JSON Configuration Structure Analysis

## Overview

The air monitoring system uses a comprehensive JSON configuration file (`config.json`) to manage all system parameters, sensor settings, network configurations, and operational modes. The configuration system supports dual storage locations (SD card and SPIFFS) with automatic fallback mechanisms.

## Configuration Loading Architecture

### File Location Priority
1. **Primary**: SD card (`/config.json`)
2. **Fallback**: SPIFFS flash memory (`/config.json`)

### Loading Process
```cpp
bool readConfig(void) {
    _config_id = 0;  // 0 = SD card, 1 = SPIFFS
    String src = "SD card";
    
    File SDconfig = SD.open("/config.json", "r");
    if (!SDconfig) {
        Serial.println("Failed to open config.json for reading on SD card");
        _config_id = 1;
        src = "SPIFSS card";
        File FSconfig = SPIFFS.open("/config.json", "r");
        if (!FSconfig) {
            Serial.println("Failed to open config.json for reading on SPIFFS");
            return false;
        }
        else {
            deserializeJson(_config_flash, FSconfig);
            FSconfig.close();
        }
    } 
    else {
        deserializeJson(_config_flash, SDconfig);
        SDconfig.close(); 
    }
    
    if (!fetchConfig()) {  
        goSleep("readConfig() Invalid config.json, reboot in 30min", 60*30);
        return false;
    }
    return true;
}
```

### Configuration Parsing
The `fetchConfig()` function parses the JSON document and validates all parameters, applying defaults for missing values and performing bounds checking.

## Configuration Structure

### Device Information
```json
{
  "device": {
    "id": "AM-6003",
    "firmware": "4.40",
    "verbose_level": 1,
    "location": {
      "date": "251022",
      "time": "142723",
      "quality": 3,
      "latitude": -97.59014,
      "longitude": 30.45492,
      "altitude": 171,
      "speed": 0,
      "heading": 0,
      "HDOP": 1.7,
      "Sats": 4,
      "label": "Terra",
      "group": "Main"
    },
    "owner": "TerraSLS"
  }
}
```

**Parameters:**
- `id`: Device identifier (required, validated for non-empty string)
- `firmware`: Automatically updated with current version
- `location`: GPS/GNSS coordinates and metadata
- `owner`: Device ownership information

### Hardware Configuration
```json
{
  "hardware": {
    "weather_shield_version": 14,
    "panel_watts": 25,
    "battery": {
      "low_power_voltage": 3.4,
      "battery_ah": 30,
      "low_power_nap": "2:00"
    }
  }
}
```

**Parameters:**
- `weather_shield_version`: Hardware version (10=VF2, 11=VF3, 12=VG, 13=VH, 14=VH2 CAN)
- `battery.low_power_voltage`: Battery protection threshold (default: 3.4V)
- `battery.battery_ah`: Battery capacity in amp-hours
- `battery.low_power_nap`: Sleep duration when battery is low

### Sensor Configuration
```json
{
  "hardware": {
    "sensors": {
      "cal_date_time": "10/10/2024 20:11:15",
      "cal_time_stamp": 1234,
      "cal_temp": 20.0,
      "temp_offset": -3.2,
      "temp_dx": 0.0
    }
  }
}
```

#### Environmental Sensors
- `temp_offset`: Temperature calibration offset
- `temp_dx`: Temperature differential calculation
- `cal_temp`: Calibration reference temperature (default: 20.0°C)

#### Particulate Matter (PM) Sensors
```json
{
  "pm_enable": false,
  "pm25_restime_min": 0
}
```
- `pm_enable`: Enable/disable PM sensor
- `pm25_restime_min`: Rest time between measurements

#### VOC Sensors
```json
{
  "voc_enable": false,
  "voc_mox_enable": false,
  "voc_offset": 0,
  "voc_gain": 0.001,
  "voc_trig": 60,
  "voc_tcomp": 0.0,
  "voc_exp_C": 0.0,
  "voc_exp_b": 0.0,
  "voc_exp_A": 0.0,
  "voc_lin_m": 0,
  "voc_lin_b": 0
}
```
- Exponential conversion: `y = A * exp(b * x) + C`
- Linear conversion: `y = m * x + b`
- Temperature compensation and trigger thresholds

#### Wind Sensors
```json
{
  "wind_enable": true,
  "wind_interval_sec": 30
}
```

#### Infrared Sensors (CO2, CH4, PID)
```json
{
  "pid_enable": true,
  "pid_offset_volt": 0.0,
  "pid_gain": 0,
  "pid_range_ppm": 0,
  "pid_trig": 0.0,
  "pid_tcomp": 0.0,
  "pid_lin_b": 0,
  "pid_lin_m": 0,
  
  "c1_enable": false,
  "c1_offset_volt": 0.1,
  "c1_gain": 1,
  "c1_range_ppm": 50000,
  "c1_trig": 0.001,
  "c1_tcomp": 0.0,
  
  "co2_enable": false,
  "co2_offset_volt": 0.1,
  "co2_gain": 1,
  "co2_range_ppm": 300000,
  "co2_trig": 0.001,
  "co2_tcomp": 0.0
}
```

#### Electrochemical Sensors (H2S, O3, SO2, NO2, NH3)
```json
{
  "h2s_enable": false,
  "h2s_offset_volt": 0.410,
  "h2s_gain": 39.3,
  "h2s_trig": 0.0005,
  "h2s_tcomp": 0.0,
  
  "o3_enable": false,
  "o3_offset_volt": 1.024,
  "o3_gain": -33.4,
  "o3_trig": 0.0005,
  "o3_tcomp": 0.0,
  
  "so2_enable": false,
  "so2_offset_volt": 1.024,
  "so2_gain": 80.2,
  "so2_trig": 0.0005,
  "so2_tcomp": 0.0,
  
  "no2_enable": false,
  "no2_offset_volt": 1.024,
  "no2_gain": -66.8,
  "no2_trig": 0.0005,
  "no2_tcomp": 0.0,
  
  "nh3_enable": false,
  "nh3_offset_volt": 1.024,
  "nh3_gain": -29.26,
  "nh3_trig": 0.0005,
  "nh3_tcomp": 0.0,
  
  "red_r0": 100000,
  "ox_r0": 800,
  "nh3_r0": 10000
}
```

**Common Parameters:**
- `*_enable`: Enable/disable individual sensors
- `*_offset_volt`: Voltage offset for calibration
- `*_gain`: Sensitivity gain (mV/ppm)
- `*_trig`: Peak detection trigger threshold
- `*_tcomp`: Temperature compensation coefficient
- `*_lin_m`, `*_lin_b`: Linear calibration coefficients

#### MPS (Micro Particle Sensor)
```json
{
  "mps_enable": false
}
```

### SUMMA Canister Configuration
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

**Parameters:**
- `enable`: Enable/disable SUMMA canister functionality
- `i2c_address`: I2C address of relay controller (hex string format)
- `open_channel`/`close_channel`: Relay channel numbers (1-8)
- `pid_threshold_ppm`: PID concentration threshold for triggering
- `dwell_seconds`: Time threshold must be exceeded before triggering
- `open_pulse_ms`/`close_pulse_ms`: Relay pulse durations
- `unlatch_after_seconds`: Auto-close timer duration
- `triggered`: Persistent trigger state (0/1)
- `triggered_at`: Timestamp when triggered

**Validation Rules:**
- I2C address: 0x08-0x77 range
- Channel numbers: 1-8 range
- PID threshold: Must be positive
- Dwell time: 1-3600 seconds
- Pulse durations: 10-5000ms
- Unlatch time: 60-7200 seconds

### Network Configuration
```json
{
  "network": {
    "no_coms_nap": "0:10",
    "wifi": {
      "ap_always_on": false,
      "ap_password": "",
      "enabled": false,
      "antenna": false,
      "ssid": "TerraSLS",
      "password": "weslowifi"
    },
    "lte": {
      "chip": "Quectel BG95-M3 Revision: BG95M1LAR02A04",
      "carrier": "Teal",
      "apn": "teal",
      "enabled": true,
      "attach_timeout": "0:03",
      "IMEI": "860111058858410",
      "IMSI": "234500024594022",
      "CCID": "8901990000001524597F",
      "RSSI": 33,
      "gps_enabled": true,
      "lock_gps": false
    },
    "offline_mode": false
  }
}
```

**WiFi Parameters:**
- `enabled`: Enable WiFi connectivity
- `ssid`/`password`: Network credentials
- `ap_always_on`: Access point mode

**LTE Parameters:**
- `enabled`: Enable LTE connectivity
- `apn`: Access Point Name
- `gps_enabled`: Enable GPS functionality
- `chip`/`IMEI`/`IMSI`/`CCID`: Modem identification (auto-populated)
- `RSSI`: Signal strength (auto-updated)

### Server Configuration
```json
{
  "server": {
    "mqtt": {
      "host": "a1njj292w2vjt1-ats.iot.us-west-2.amazonaws.com",
      "port": 8883,
      "ssl": {
        "label": "global",
        "ca": "-----BEGIN CERTIFICATE-----...",
        "cert": "-----BEGIN CERTIFICATE-----...",
        "key": "-----BEGIN RSA PRIVATE KEY-----..."
      }
    },
    "update": {
      "endpoint": "airmonitor-utils.terrasls.com/softwareupdate",
      "automatic_updates": false,
      "check_interval": "24:00"
    },
    "error_log": {
      "endpoint": "airmonitor-utils.terrasls.com/erorLog"
    }
  }
}
```

**MQTT Parameters:**
- `host`: MQTT broker hostname
- `port`: MQTT broker port (typically 8883 for SSL)
- `ssl.ca`: Certificate Authority certificate
- `ssl.cert`: Client certificate
- `ssl.key`: Client private key

### Sampling Configuration
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

**Parameters:**
- `pumping_time`: Sampling pump duration (0 = always on)
- `sampling_interval_sec`: Time between sensor readings (minimum 2 seconds)
- `report_interval_count`: Number of samples before reporting
- `sleep_time_sec`: Deep sleep duration between reports
- `engineering`: Enable engineering data format

## Configuration Validation and Error Handling

### Parameter Validation
The system performs extensive validation during configuration loading:

1. **Required Fields**: Device ID must be present and non-empty
2. **Range Checking**: Numeric values validated against acceptable ranges
3. **Type Validation**: Ensures correct data types for all parameters
4. **Default Values**: Missing parameters get safe default values
5. **Bounds Checking**: Prevents invalid configurations that could cause system instability

### Error Recovery
- Invalid configuration triggers 30-minute sleep and retry
- Missing configuration files cause system reboot
- Corrupted JSON data results in fallback to SPIFFS or factory defaults

### Configuration Persistence
The system automatically saves configuration changes for:
- GPS coordinates when acquired
- LTE modem information when connected
- Sensor calibration updates
- SUMMA canister trigger state
- Temperature calibration adjustments

### Save Functions
```cpp
void saveGpsConfig(void);     // GPS coordinate updates
void saveLteConfig(void);     // LTE modem information
void saveIRConfig(byte model); // IR sensor calibration
void saveSPEConfig(byte model); // Electrochemical sensor calibration
void saveTempConfig();        // Temperature calibration
void saveSummaConfig(void);   // SUMMA canister state
```

## Configuration Loading from SD Card vs SPIFFS

### Dual Storage Strategy
The system maintains configuration files in both locations:
1. **SD Card**: Primary storage, user-accessible
2. **SPIFFS**: Backup storage, embedded in firmware

### Loading Priority
1. Attempt to load from SD card first
2. If SD card fails, fallback to SPIFFS
3. If both fail, system enters error state and reboots

### Configuration ID Tracking
- `_config_id = 0`: Configuration loaded from SD card
- `_config_id = 1`: Configuration loaded from SPIFFS

This allows the system to track configuration source and handle EEPROM calibration data appropriately.

## Configuration Update Mechanisms

### Runtime Updates
Configuration can be updated during runtime through:
- MQTT commands for calibration adjustments
- GPS coordinate acquisition
- LTE modem information updates
- SUMMA canister state changes

### Persistence Strategy
All configuration changes are immediately written to both storage locations to ensure data integrity and prevent loss during power cycles or system resets.

### Validation on Load
Every configuration load operation includes comprehensive validation to ensure system stability and prevent invalid parameter combinations that could cause sensor errors or communication failures.