# LTE Modem Management Analysis

## Overview

The air monitoring system uses a Quectel BG95 LTE modem for cellular connectivity, data transmission, and GPS/GNSS functionality. The modem management is implemented in `myLTE.cpp` and provides comprehensive control over modem initialization, connection establishment, and monitoring procedures.

## Hardware Configuration

### Pin Assignments
- **MODEN_PWR_EN (Pin 13)**: Power enable pin (only with qTop board)
- **MODEN_PWKEY (Pin 32)**: Power key control pin for modem on/off
- **Serial Interface**: Hardware Serial 2 at 115200 baud rate

### Power Control
The modem uses a power key (PWKEY) toggle mechanism for on/off control:
```cpp
void togglePWKEY(bool bWait = false) {
  Serial.println("Toggle BG95 PWKEY 1sec ...");
  digitalWrite(MODEN_PWKEY, HIGH); // PWKey - Low 
  delay(1000);                      // 1 second pulse
  digitalWrite(MODEN_PWKEY, LOW);  // PWKey - High 
  if (bWait) { 
    Serial.println("Waiting 10sec for BG95 to initialize ...");
    delay(10000); 
  }
}
```

## Modem Initialization Process

### Setup Function (`setupLTE()`)
1. **Pin Configuration**: Sets up power enable and power key pins as outputs
2. **Initial State**: Sets PWKEY low and turns modem off
3. **Configuration Loading**: Loads config.json settings
4. **Connection Flag**: Initializes `_is_lte_connected` to false

### Modem Startup (`lte_ON()`)
The modem startup follows a specific sequence:

1. **Brownout Protection**: Disables brownout detector during initialization
2. **Power Sequence**: 
   - Calls `setModem_ON()` to enable power
   - Toggles PWKEY with 10-second initialization wait
3. **SSL Certificate Setup**: Configures AWS certificates for secure MQTT
4. **Serial Communication**: Initializes serial interface at 115200 baud
5. **Modem Initialization**: Calls `modem.begin()` and waits 10 seconds
6. **Information Retrieval**: Fetches modem details (IMEI, IMSI, CCID, etc.)
7. **Network Registration**: Connects to cellular network and GPRS

## Connection Establishment

### Network Connection Process
```cpp
// Network registration and GPRS connection
modem.sendAT("+CREG=1");  // Enable network registration
if (modem.waitForNetwork(60000)) {  // 1-minute timeout
    modem.gprsConnect(_apn);  // Connect to APN
    if (modem.isGprsConnected()) {
        _is_lte_connected = true;
        _rssi = modem.getSignalQuality();
    }
}
```

### APN Configuration
The system supports multiple APNs:
- **"teal"**: Teal network (includes EID retrieval)
- **"isp.telus.com"**: Telus network
- APN is configured via JSON configuration file

### Connection Monitoring
- **Signal Quality**: Retrieved via `modem.getSignalQuality()`
- **Registration Status**: Monitored via `modem.getRegistrationStatus()`
- **Connection State**: Tracked with `_is_lte_connected` global flag

## Modem Information Retrieval

The system collects comprehensive modem information:
```cpp
_chip = modem.getModemInfo();  // Modem model/version
_imei = modem.getIMEI();       // International Mobile Equipment Identity
_imsi = modem.getIMSI();       // International Mobile Subscriber Identity
_ccid = modem.getSimCCID();    // SIM card ID
_rssi = modem.getSignalQuality(); // Signal strength
_eid = getEID();               // eUICC ID (for Teal network)
```

### EID Retrieval (Teal Network)
For Teal network connections, the system retrieves the eUICC ID using AT commands:
```cpp
String getEID(void) {
    modem.sendAT("+CSIM=10,\"0070000000\"");
    modem.sendAT("+CSIM=42,\"01A4040010A0000005591010FFFFFFFF8900000200\"");
    modem.sendAT("+CSIM=10,\"81CA005A00\"");
    // Parse response to extract EID
}
```

## Connection Management and Recovery

### Retry Mechanism (`setupConnection()`)
The system implements a robust retry mechanism:
1. **LTE Connection Retry**: Up to 10 attempts with 5-minute delays
2. **MQTT Setup Retry**: Up to 3 attempts with 1-minute delays
3. **Maximum Retry Handling**: After 10 failures, modem is turned off and retry cycle restarts

### Error Handling
- **Connection Failures**: Automatic modem shutdown and retry
- **MQTT Failures**: Separate retry counter with shorter intervals
- **Watchdog Reset**: Regular `esp_task_wdt_reset()` calls during long operations

### Graceful Shutdown (`lte_OFF()`)
```cpp
void lte_OFF(bool waitMQTT = false) {
    _is_lte_connected = false;
    if (waitMQTT) {
        delay(5000);  // Wait for MQTT transmission completion
    }
    digitalWrite(MODEN_PWKEY, LOW);
    setModem_OFF();
}
```

## SSL/TLS Certificate Management

### AWS Certificate Configuration
The system uses hardcoded AWS certificates for secure MQTT connections:
- **Root CA Certificate**: Amazon Root CA 1
- **Device Certificate**: AWS IoT device-specific certificate
- **Private Key**: Device private key for authentication

### Certificate Setup
```cpp
secure_layer.setCACert(AWS_CERT_CA);
secure_layer.setCertificate(AWS_CERT_CRT);
secure_layer.setPrivateKey(AWS_CERT_PRIVATE);
client = new PubSubClient(_mqtt_host, _mqtt_port, callback, secure_layer);
```

## Network Time Synchronization

### Network Time Retrieval (`get_network_time()`)
```cpp
bool get_network_time(void) {
    if (modem.getNetworkTime(&gsm_year, &gsm_month, &gsm_day, 
                            &gsm_hour, &gsm_min, &gsm_sec, &gsm_timezone)) {
        setRTC(gsm_year, gsm_month, gsm_day, gsm_hour, gsm_min, gsm_sec, gsm_timezone);
        return true;
    }
    return false;
}
```

### Time Synchronization Features
- **Automatic RTC Update**: Network time automatically updates real-time clock
- **Timezone Support**: Handles UTC/GMT offset from network
- **Fallback Mechanism**: GPS time used as backup if network time fails

## Serial Modem Proxy

### Debug Interface (`serialModemProxyTask()`)
The system provides a debug interface for direct AT command communication:
```cpp
void serialModemProxyTask(void* parameter) {
    while (true) {
        if (Serial.available() > 0) {
            String command = Serial.readStringUntil('\n');
            modem.sendAT(command);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

## Integration with System Tasks

### Task Coordination
- **Data Collection**: LTE status affects data transmission scheduling
- **MQTT Communication**: Requires active LTE connection
- **GPS Operations**: Coordinated with LTE to avoid UART contention
- **Power Management**: LTE shutdown integrated with sleep modes

### Global State Variables
- **`_is_lte_connected`**: Primary connection status flag
- **`_ota_status`**: OTA update status affecting LTE operations
- **`_lte_enabled`**: Configuration flag for LTE functionality

## Configuration Parameters

### JSON Configuration Elements
- **APN Settings**: Network access point configuration
- **MQTT Host/Port**: Server connection parameters
- **GPS Enable**: GPS functionality control
- **Retry Delays**: Connection retry timing parameters

### Runtime Configuration Storage
- Modem information saved to configuration via `saveLteConfig()`
- Network parameters updated during connection establishment
- Signal quality and connection status continuously monitored

## Error Conditions and Recovery

### Common Error Scenarios
1. **Modem Initialization Failure**: Hardware or firmware issues
2. **Network Registration Failure**: Coverage or SIM card issues
3. **GPRS Connection Failure**: APN or network configuration issues
4. **MQTT Connection Failure**: Server or certificate issues

### Recovery Strategies
- **Automatic Retry**: Built-in retry mechanisms with exponential backoff
- **Modem Reset**: Power cycling for hardware recovery
- **Configuration Reload**: Fresh configuration loading on failures
- **System Reboot**: Ultimate recovery mechanism for persistent failures

## Performance Considerations

### Timing Constraints
- **Initialization Time**: 10-second modem startup delay
- **Network Registration**: 60-second timeout for network connection
- **MQTT Setup**: 1-minute retry intervals
- **Graceful Shutdown**: 5-second MQTT completion wait

### Resource Management
- **Memory Usage**: Dynamic PubSubClient allocation
- **Power Consumption**: Coordinated with system power management
- **UART Contention**: Managed sharing between LTE and GPS functions
- **Watchdog Management**: Regular resets during long operations