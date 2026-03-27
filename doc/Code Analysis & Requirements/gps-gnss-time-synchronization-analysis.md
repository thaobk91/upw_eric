# GPS/GNSS Time Synchronization Analysis

## Overview

The air monitoring system implements a comprehensive time synchronization system using GPS/GNSS and cellular network time sources. The system maintains accurate timestamps for sensor data through multiple time sources with fallback mechanisms and validation procedures to ensure data integrity.

## Time Source Hierarchy

### Primary Time Sources
1. **GPS/GNSS Time**: Most accurate, satellite-based time synchronization
2. **Network Time**: Cellular network-provided time via LTE modem
3. **External RTC**: Hardware real-time clock (DS3231 or similar)
4. **Internal RTC**: ESP32 built-in RTC with compile-time initialization

### Time Source Priority
```cpp
// Priority order for time synchronization:
// 1. GPS time (if GPS enabled and available)
// 2. Network time (if LTE connected and GPS disabled)
// 3. External RTC (if hardware present and valid)
// 4. Internal RTC (fallback with compile-time base)
```

## GPS/GNSS Implementation

### GNSS Configuration and Setup
The system uses the Quectel BG95 modem's integrated GNSS functionality:

```cpp
bool modem_gnss_on(void) {
  if (_is_gnss_on == false) {
    // Configure GNSS for USA (GPS + GLONASS)
    modem.sendAT(GF("+QGPSCFG=\"gnssconfig\",1\r\n"));
    
    // Set GPS priority
    modem.sendAT(GF("+QGPSCFG=\"priority\",0,0\r\n"));
    
    // Enable GPS with high accuracy, continuous acquisition
    modem.sendAT(GF("+QGPS=1,3,0\r\n"));
    
    _is_gnss_on = true;
    return true;
  }
  return false;
}
```

### GNSS Configuration Parameters
- **GNSS Config**: GPS + GLONASS constellation (USA configuration)
- **Priority**: GPS prioritized over GLONASS
- **Mode**: High accuracy mode with continuous acquisition
- **Timeout**: 1-second response timeout for AT commands

### GPS Data Collection Process (`get_gnss_data()`)

#### Raw Data Retrieval
```cpp
bool get_gnss_data(void) {
  String gnss_str = modem.getGPSraw(); // Get raw NMEA data
  
  if (gnss_str.length() > 32) {
    // Parse NMEA fields using getfield() function
    // Extract: time, latitude, longitude, altitude, quality, etc.
    _is_gnss_ready = true;
    return true;
  }
  
  _is_gnss_ready = false;
  return false;
}
```

#### NMEA Data Parsing
The system parses NMEA sentences to extract GPS information:

| Field Index | Data Type | Variable | Description |
|-------------|-----------|----------|-------------|
| 0 | String | `_gnss_time` | UTC time (HHMMSS format) |
| 1 | Float | `_gnss_latitude` | Latitude in decimal degrees |
| 2 | Float | `_gnss_longitude` | Longitude in decimal degrees |
| 3 | Float | `_gnss_hdop` | Horizontal dilution of precision |
| 4 | Integer | `_gnss_altitude` | Altitude in meters |
| 5 | Integer | `_gnss_quality` | Fix quality indicator |
| 6 | Float | `_gnss_heading` | Course over ground |
| 7 | Float | `_gnss_speed` | Speed in km/h |
| 9 | String | `_gnss_date` | UTC date (DDMMYY format) |
| 10 | Integer | `_gnss_nsat` | Number of satellites in use |

### GPS Time Acquisition (`get_gps_time()`)

```cpp
bool get_gps_time(void) {
  int gps_year, gps_month, gps_day, gps_hour, gps_min, gps_sec;
  float gps_timezone = 0; // UTC timezone
  
  modem_gnss_on();
  taskDelay(60000, 5); // 60-second acquisition timeout
  
  if (_gps_enabled && _is_gnss_ready) {
    if (modem.getGPSTime(&gps_year, &gps_month, &gps_day, 
                         &gps_hour, &gps_min, &gps_sec)) {
      // Update RTC with GPS time
      setRTC(gps_year, gps_month, gps_day, gps_hour, gps_min, gps_sec, gps_timezone);
      _rtc_err = false;
      modem_gnss_off();
      return true;
    }
  }
  
  modem_gnss_off();
  return false;
}
```

### GPS Setup and Retry Logic (`setupGPS()`)

```cpp
bool setupGPS(void) {
  if (_gps_enabled == false) return false;
  
  // Enable GNSS if not already on
  if (_is_gnss_on == false) modem_gnss_on();
  
  // Attempt GPS data collection up to 10 times
  for (int i = 1; i <= 10; i++) {
    if (get_gnss_data() == false) {
      Serial.printf("GPS Fetch(%i/10), Failed ... retry in 30 sec\r\n", i);
      taskDelay(30000, 0); // 30-second retry delay
    } else {
      _lock_gps = false;
      break; // Success - exit retry loop
    }
  }
  
  modem_gnss_off();
  saveGpsConfig(); // Save GPS data to configuration
  return true;
}
```

## Network Time Synchronization

### Network Time Acquisition (`get_network_time()`)

```cpp
bool get_network_time(void) {
  int gsm_year, gsm_month, gsm_day, gsm_hour, gsm_min, gsm_sec;
  float gsm_timezone;
  
  if (_is_lte_connected == false) return false;
  
  if (modem.getNetworkTime(&gsm_year, &gsm_month, &gsm_day,
                          &gsm_hour, &gsm_min, &gsm_sec, &gsm_timezone)) {
    // Update RTC with network time
    setRTC(gsm_year, gsm_month, gsm_day, gsm_hour, gsm_min, gsm_sec, gsm_timezone);
    return true;
  }
  
  return false;
}
```

### Network Time Features
- **Automatic Timezone**: Network provides UTC offset information
- **LTE Dependency**: Requires active LTE connection
- **Fallback Role**: Used when GPS is disabled or unavailable
- **Immediate Availability**: No acquisition delay like GPS

## RTC Management System

### Dual RTC Architecture
The system maintains two RTC sources:

1. **External RTC** (`rtc`): Hardware DS3231 or similar
2. **Internal RTC** (`internalrtc`): ESP32 built-in RTC

### RTC Time Setting (`setRTC()`)

```cpp
void setRTC(int year_, int month_, int day_, int hour_, int min_, int sec_, float timezone_) {
  DateTime testTime = DateTime(year_, month_, day_, hour_, min_, sec_);
  
  // Validate input time
  if (!isValidDateTime(testTime)) {
    Serial.println("Invalid time provided to setRTC, ignoring");
    return;
  }
  
  // Convert to GMT
  DateTime gmt_ = testTime - TimeSpan(0, timezone_, 0, 0);
  
  // Validate GMT time
  if (!isValidDateTime(gmt_)) {
    Serial.println("Invalid GMT time calculated in setRTC, ignoring");
    return;
  }
  
  // Update both RTCs
  if (_is_RTC_on) {
    rtc.adjust(gmt_); // External RTC
  }
  internalrtc.adjust(gmt_); // Internal RTC
  
  _rtc_err = false;
}
```

### RTC Time Retrieval (`fetchRTC()`)

```cpp
unsigned long fetchRTC(void) {
  DateTime now = internalrtc.now();
  
  if (!_gpsTimeAquired) {
    // Validate internal RTC time
    if (!isValidDateTime(now)) {
      Serial.println("Internal RTC time is invalid, resetting to compile time");
      internalrtc.begin(DateTime(F(__DATE__), F(__TIME__)));
      now = internalrtc.now();
    }
    
    // Use external RTC if available and valid
    if (_is_RTC_on) {
      DateTime rtcTime = rtc.now();
      if (!isValidDateTime(rtcTime)) {
        _rtc_err = true;
      } else {
        internalrtc.adjust(rtcTime);
        now = rtcTime;
      }
    } else {
      _rtc_err = true;
    }
  }
  
  return now.unixtime();
}
```

## Time Validation System

### DateTime Validation (`isValidDateTime()`)

```cpp
bool isValidDateTime(DateTime dt) {
  DateTime compileTime = DateTime(F(__DATE__), F(__TIME__));
  uint16_t currentYear = compileTime.year();
  
  // Allow dates from current year to 5 years in future
  uint16_t minYear = currentYear;
  uint16_t maxYear = currentYear + 5;
  
  // Range validation
  if (dt.year() < minYear || dt.year() > maxYear) return false;
  if (dt.month() < 1 || dt.month() > 12) return false;
  if (dt.day() < 1 || dt.day() > 31) return false;
  if (dt.hour() > 23) return false;
  if (dt.minute() > 59) return false;
  if (dt.second() > 59) return false;
  
  return true;
}
```

### Validation Criteria
- **Year Range**: Current compile year to 5 years in future
- **Month Range**: 1-12
- **Day Range**: 1-31 (basic validation)
- **Hour Range**: 0-23
- **Minute Range**: 0-59
- **Second Range**: 0-59

## Time Synchronization Strategy

### Main Loop Time Management
```cpp
// In main collectDataTask loop:

// Get GPS time if RTC error and GPS enabled
if (_rtc_err && _gps_enabled) {
  _gpsTimeAquired = get_gps_time();
}

// Periodic GPS time acquisition (every 100 reports)
_gps_report_counter++;
if (_gps_report_counter >= 100) {
  _gpsTimeAquired = get_gps_time();
  _gps_report_counter = 0;
}

// Get network time if RTC error and GPS disabled
if (_rtc_err && !_gps_enabled) {
  get_network_time();
}

// Fetch current timestamp for data logging
_time_stamp = fetchRTC();
```

### Synchronization Triggers
1. **RTC Error Condition**: Automatic time sync when `_rtc_err` is true
2. **Periodic Sync**: GPS time every 100 data collection cycles
3. **System Startup**: Initial time synchronization during boot
4. **Manual Request**: MQTT command-triggered time sync

## GPS Data Storage and Configuration

### GPS Configuration Storage (`saveGpsConfig()`)

```cpp
void saveGpsConfig(void) {
  _config_flash["device"]["location"]["date"] = _gnss_date;
  _config_flash["device"]["location"]["time"] = _gnss_time;
  _config_flash["device"]["location"]["quality"] = _gnss_quality;
  _config_flash["device"]["location"]["longitude"] = _gnss_longitude;
  _config_flash["device"]["location"]["latitude"] = _gnss_latitude;
  _config_flash["device"]["location"]["altitude"] = _gnss_altitude;
  _config_flash["device"]["location"]["speed"] = _gnss_speed;
  _config_flash["device"]["location"]["heading"] = _gnss_heading;
  _config_flash["device"]["location"]["HDOP"] = _gnss_hdop;
  _config_flash["device"]["location"]["Sats"] = _gnss_nsat;
  _config_flash["device"]["location"]["lock_gps"] = _is_gnss_ready;
  _config_flash["network"]["lte"]["lock_gps"] = _is_gnss_ready;
  
  writeConfig(); // Persist to SD card and SPIFFS
}
```

### Stored GPS Parameters
- **Position Data**: Latitude, longitude, altitude
- **Time Data**: UTC date and time
- **Quality Metrics**: Fix quality, HDOP, satellite count
- **Motion Data**: Speed and heading
- **Status Flags**: GPS lock status and readiness

## Error Handling and Recovery

### RTC Error Detection
```cpp
// Error flag management
bool _rtc_err = true;  // Global RTC error flag
bool _gpsTimeAquired = false; // GPS time acquisition status

// Error reporting in sensor error collection
if (_rtc_err) _errors |= 64; // Set bit 6 for RTC error
```

### Error Recovery Mechanisms

#### GPS Acquisition Failure
- **Retry Logic**: Up to 10 attempts with 30-second delays
- **Fallback**: Network time if GPS fails
- **Status Tracking**: `_is_gnss_ready` flag indicates GPS status

#### Network Time Failure
- **LTE Dependency**: Requires active cellular connection
- **Automatic Retry**: Integrated with LTE connection management
- **Fallback**: External/internal RTC if network unavailable

#### RTC Hardware Failure
- **Detection**: Validation of RTC readings
- **Fallback**: Internal RTC with compile-time initialization
- **Error Reporting**: RTC error flag in system status

### UART Contention Management
```cpp
// GPS and LTE share UART interface
// Careful coordination required:

// 1. GPS operations turn off during LTE communication
modem_gnss_off(); // Before LTE operations

// 2. GPS enabled only when needed
if (_gps_enabled) modem_gnss_on(); // Conditional GPS activation

// 3. Sequential operations to avoid conflicts
setupGPS(); // GPS data collection
// ... then LTE operations
```

## Performance and Power Considerations

### GPS Acquisition Timing
- **Cold Start**: Up to 60 seconds for initial fix
- **Warm Start**: 10-30 seconds for subsequent fixes
- **Hot Start**: 1-5 seconds if recent fix available
- **Timeout**: 60-second maximum acquisition time

### Power Management
- **GPS On-Demand**: GNSS enabled only when needed
- **Automatic Shutdown**: GPS turned off after data collection
- **LTE Coordination**: GPS disabled during LTE operations
- **Sleep Integration**: GPS operations coordinated with system sleep

### Memory Usage
- **NMEA Buffer**: 128 bytes for raw GPS data parsing
- **Field Buffer**: 32 bytes for individual field extraction
- **Configuration Storage**: GPS data stored in JSON configuration
- **Global Variables**: Minimal memory footprint for GPS state

## Integration with System Architecture

### Task Coordination
- **Data Collection Task**: Manages periodic GPS time sync
- **LTE Communication Task**: Coordinates UART usage
- **Configuration Management**: Stores GPS data persistently
- **Error Reporting**: Integrates RTC errors with system diagnostics

### Configuration Dependencies
- **GPS Enable Flag**: `_gps_enabled` controls GPS functionality
- **LTE Priority**: LTE operations take precedence over GPS
- **Timezone Handling**: Automatic conversion to GMT for storage
- **Validation Requirements**: All time sources validated before use

### Data Flow Integration
- **Timestamp Generation**: Provides accurate timestamps for sensor data
- **MQTT Publishing**: GPS location data published to cloud
- **CSV Logging**: Timestamps included in all data records
- **System Monitoring**: Time sync status reported in system health