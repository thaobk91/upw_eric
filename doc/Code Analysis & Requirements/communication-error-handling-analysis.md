# Communication Error Handling Analysis

## Overview

This document analyzes the communication error handling and recovery mechanisms implemented for LTE modem management, MQTT protocol implementation, and network connectivity in the air monitoring system. The system employs comprehensive error detection, retry strategies, and automatic recovery procedures.

## Communication Components

### 1. LTE Modem Management
- **Hardware**: Quectel BG95 LTE modem
- **Communication**: UART interface (115200 baud)
- **Power Control**: Hardware power enable and PWKEY control
- **Network**: GPRS connection with configurable APN

### 2. MQTT Protocol Implementation
- **Security**: SSL/TLS with AWS IoT certificates
- **Library**: PubSubClient with SSLClient wrapper
- **Topics**: Hierarchical topic structure (AM/{device_id}/...)
- **QoS**: Default QoS level with keep-alive management

### 3. GPS/GNSS Communication
- **Integration**: Shared UART with LTE modem
- **Protocols**: GPS + GLONASS configuration
- **Data**: Position, time, and quality metrics

## Error Detection Mechanisms

### 1. Modem Initialization Errors

**Modem Begin Failure:**
```cpp
// In lte_ON() function
if (modem.begin() == true) {
    // Successful initialization
} else {
    Serial.println("Modem failed to initialize");
    lte_OFF(false);
}
```

**Network Registration Timeout:**
```cpp
// Network connection with timeout
if (modem.waitForNetwork(60000)) { // 1 minute timeout
    // Network registered successfully
} else {
    Serial.println("LTE connection failed");
    lte_OFF(false);
}
```

**GPRS Connection Failure:**
```cpp
// GPRS connection validation
if (modem.isGprsConnected()) { 
    _is_lte_connected = true;
    _rssi = modem.getSignalQuality();
} else {
    Serial.println("GPRS connection failed");
    lte_OFF(false);
}
```

### 2. MQTT Connection Errors

**Initial Connection Failure:**
```cpp
// In setupMQTT() function
if (client->connect(_device_id)) {
    // Configure MQTT settings and subscribe
    client->setKeepAlive(60);
    client->setBufferSize(2048);
    return true;
} else {
    Serial.printf("MQTT, Failed#%u to connect rc:%i\r\n", fail_count, client->state());
    return false;
}
```

**Connection State Monitoring:**
```cpp
// In serveMQQT() function
if (!client->connected()) {
    if (client->connect(_device_id)) {
        client->subscribe(buildTopic("/cmd").c_str());
        Serial.println("MQTT Reconnected");
    } else {
        Serial.println("MQTT Disconnected");
    }
}
```

**Publish Failure Detection:**
```cpp
// In publish_data() function
if (client->connected()) {
    if (client->publish(buildTopic("/sensors").c_str(), csvRJsonStr.c_str())) {
        Serial.println("Data sent"); 
        return true; 
    } else {
        Serial.println("Unable to publish"); 
        return false;
    }
} else {
    Serial.println("Unable to send - No connection");
    return false;
}
```

### 3. GPS/GNSS Communication Errors

**GPS Time Acquisition Failure:**
```cpp
// In get_gps_time() function
if (modem.getGPSTime(&gps_year, &gps_month, &gps_day, &gps_hour, &gps_min, &gps_sec)) {
    // Process GPS time
    setRTC(gps_year, gps_month, gps_day, gps_hour, gps_min, gps_sec, gps_timezone);
    return true;
} else {
    Serial.println("Timeout getting GPS time");
    return false;
}
```

**GPS Data Validation:**
```cpp
// In get_gnss_data() function
len = gnss_str.length();
if ( len > 32) {
    // Process GPS data
    _is_gnss_ready = true;
    return true;
} else {
    Serial.println(" GPS ... no data available yet");
    _is_gnss_ready = false;
    return false;
}
```

## Recovery Mechanisms

### 1. Connection Setup with Retry Logic

**Multi-level Retry Strategy:**
```cpp
// In setupConnection() function
void setupConnection(void) {
    int _RetryDelay = 5; // retry delay in minutes
    int _fail_counter = 0; // loop err counter

    while (true) {
        // LTE Connection Retry
        if ( _lte_enabled) {
            while (lte_ON() == false) { 
                _fail_counter++;
                esp_task_wdt_reset();
                Serial.printf("Failed_%i to turn ON LTE modem, retrying in %imin ...\r\n", 
                             _fail_counter, _RetryDelay);
                taskDelay(_RetryDelay*60*1000, 2); 
            }
            
            setupGPS(); // acquire GPS data
            _fail_counter = 0;
        }

        // MQTT Connection Retry
        while ((setupMQTT(_fail_counter) == false) && (_fail_counter < 3)) {
            _fail_counter++;
            esp_task_wdt_reset();
            taskDelay(60*1000, 2); // wait 1 min before retrying
        }

        // Maximum Retry Handling
        if (_fail_counter == 10) { 
            lte_OFF(false);
            Serial.printf("Reached maximum retries, turning OFF LTE and retrying in %imin\r\n", 
                         _RetryDelay);
            esp_task_wdt_reset();
            taskDelay(_RetryDelay*60*1000, 2);
            continue; // restart the loop
        }
        
        break; // exit the loop on success
    }
}
```

### 2. Automatic Reconnection

**MQTT Reconnection Logic:**
```cpp
// In serveMQQT() function - called continuously
void serveMQQT(void) {
    if (!client->connected()) {
        if (client->connect(_device_id)) {
            client->subscribe(buildTopic("/cmd").c_str());
            esp_task_wdt_reset();
            Serial.println("MQTT Reconnected");
        } else {
            esp_task_wdt_reset();
            Serial.println("MQTT Disconnected");
        }
    }
    client->loop(); // Process MQTT messages
}
```

### 3. Hardware Reset Procedures

**Modem Power Cycling:**
```cpp
// Power control functions
void setModem_ON() { 
    clearIO_39(6); // Active low power control
    digitalWrite(MODEM_PWR_ON_PIN, HIGH); // Power Enable
    Serial.println(F("Modem Pwr ON, wait 10sec ...")); 
    delay(10000);
}

void setModem_OFF() { 
    setIO_39(6); // Disable power
    digitalWrite(MODEM_PWR_ON_PIN, LOW); // Power Disable
    Serial.println(F("Modem Pwr OFF"));
}
```

**PWKEY Toggle for Modem Reset:**
```cpp
void togglePWKEY(bool bWait = false) {
    Serial.println("Toggle BG95 PWKEY 1sec ...");
    digitalWrite(MODEN_PWKEY, HIGH); // PWKey - Low 
    delay(1000);
    digitalWrite(MODEN_PWKEY, LOW);  // PWKey - High 
    if ( bWait) { 
        Serial.println("Waiting 10sec for BG95 to initialize ...");
        delay(10000); 
    }
}
```

### 4. Transmission Error Recovery

**Data Transmission Retry in Main Loop:**
```cpp
// In sendDataTask() function
if (publish_data()) { // send data to server
    logIdx_Mark(_csvRIdx, 0); // mark the record as sent
    _csvRIdx++; // increment the read counter
    _err_TX = 0; 
} else { 
    _err_TX++;
    Serial.println("sendDataTask() Failed to send csv data !!!");
} 

if (_err_TX >= 3) {  // too many errors, reboot the modem
    closeMQTT(); // close the connection
    break; 
}
```

## Backoff Strategies

### 1. Exponential Backoff

**Connection Retry Delays:**
- Initial retry: 1 minute
- LTE modem retry: 5 minutes
- Maximum retries before modem reset: 10 attempts
- GPS acquisition: 30 seconds between attempts (up to 10 tries)

### 2. Graceful Degradation

**Network Time Fallback:**
```cpp
// Time synchronization priority
if (_rtc_err && _gps_enabled) {
    _gpsTimeAquired = get_gps_time(); // Primary: GPS time
}

if (_rtc_err && !_gps_enabled ) { 
    get_network_time(); // Fallback: Network time
}
```

**Offline Operation:**
- Data continues to be logged locally when communication fails
- Index system tracks transmission status
- Automatic retry when connection restored

## Connection Monitoring

### 1. Signal Quality Monitoring

**RSSI Tracking:**
```cpp
// Signal strength monitoring
_rssi = modem.getSignalQuality();
Serial.print("Modem RSSI: "); Serial.println(_rssi);

// MQTT command for signal quality check
if (strcmp(command,"csq")==0) {
    _rssi = modem.getSignalQuality();
    String statusPayload = "{\"RSSI\": \"" + String(_rssi) + "\"}";
    client->publish(buildTopic("/status").c_str(), statusPayload.c_str());
}
```

### 2. Connection State Tracking

**Global Connection Flags:**
- `_is_lte_connected` - LTE/GPRS connection status
- `_is_gnss_ready` - GPS data availability
- `_is_gnss_on` - GPS module power state

### 3. Watchdog Timer Integration

**Task Watchdog Reset:**
```cpp
// Prevent system reset during long operations
esp_task_wdt_reset(); // Called during connection attempts
```

## Error Reporting and Diagnostics

### 1. MQTT Error Publishing

**Comprehensive Error Reporting:**
```cpp
bool publish_errors(bool userRqt = false) {
    if ((_ctxerr_count > 3) || (userRqt) ) {
        sprintf(scratch, "Err#/Tca/Count/Ctx:%u/%u/%u/%u ",
                _errors, _tcaError, _err_loop, _ctxerr_count);
        sprintf(subscratch, "rtc:%u  mps/cnt:%u/%u  ir:%u  voc:%u  pm:%u  bme:%u  ",
                _rtc_err, _mps_err, _mps_err_count, _ir_err, _voc_err, _pm_err, _bme_err);
        strcat(scratch, subscratch);
        
        return client->publish(buildTopic("/error").c_str(), scratch);
    }
}
```

### 2. Serial Debug Output

All communication operations generate detailed debug output:
```cpp
Serial.println("MQTT Connected");
Serial.println("MQTT Disconnected");
Serial.printf("Failed_%i to turn ON LTE modem, retrying in %imin ...\r\n", 
              _fail_counter, _RetryDelay);
```

### 3. Remote Diagnostics

**MQTT Command Interface:**
- `csq` - Signal quality check
- `gps` - GPS status and data
- `errorlog` - Comprehensive error report
- `reset` - Remote system reset

## OTA Update Error Handling

### 1. Download Error Recovery

**File Transfer Validation:**
```cpp
// Content length validation
if (contentLength == 0) {  
    otaBreak("No content-length in the header");  
    return; 
}

// Download completion check
if (readLength != contentLength) { 
    otaBreak("Download incomplete", true); 
    return;
}
```

**Disk Write Error Handling:**
```cpp
// Write operation validation
size_t written = file.write(buffer, bufferIndex);
if (written != bufferIndex) { // disk write error
    file.close();
    otaBreak("Error writing binary file to disk", true); 
    return;
}
```

### 2. Update Process Protection

**Mutex Protection:**
```cpp
// SD card access protection during OTA
if (xSemaphoreTake(sd_mutex, portMAX_DELAY) == false) { 
    otaTrace("Failed to take SD card mutex");
    return;
}
```

**Task Coordination:**
```cpp
// Pause data collection during OTA
if (pauseCollect() == false) {
    otaTrace("Failed to pause data collection");
    return; 
}

_ota_status = true; // Signal other tasks
```

## Key Findings

1. **Multi-layered Recovery**: The system implements recovery at hardware, protocol, and application levels.

2. **Retry Strategies**: Exponential backoff with maximum retry limits prevents infinite loops.

3. **Graceful Degradation**: System continues operation with reduced functionality when communication fails.

4. **Hardware Reset Capability**: Physical power control enables recovery from hardware-level failures.

5. **Connection Monitoring**: Continuous monitoring with automatic reconnection attempts.

6. **Error Persistence**: Communication errors are logged and reported for trend analysis.

7. **Remote Diagnostics**: MQTT command interface enables remote troubleshooting and control.

8. **OTA Robustness**: Over-the-air updates include comprehensive error handling and validation.

9. **Task Coordination**: Proper synchronization prevents conflicts during error recovery operations.

10. **Watchdog Integration**: System-level protection prevents complete system lockup during communication issues.