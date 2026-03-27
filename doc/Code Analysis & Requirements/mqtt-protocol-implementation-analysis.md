# MQTT Protocol Implementation Analysis

## Overview

The air monitoring system implements MQTT (Message Queuing Telemetry Transport) protocol for bidirectional communication with AWS IoT Core. The implementation uses SSL/TLS encryption with AWS certificates and supports both data publishing and command reception through a comprehensive topic structure.

## MQTT Client Configuration

### Library and Dependencies
- **PubSubClient Library**: Arduino MQTT client library
- **SSLClient**: Provides SSL/TLS encryption layer
- **TinyGsmClient**: Cellular modem client interface
- **AWS IoT Core**: Target MQTT broker

### Client Initialization
```cpp
TinyGsmClient base_client(modem, 0);
SSLClient secure_layer(&base_client);
PubSubClient* client;

// During LTE connection setup
client = new PubSubClient(_mqtt_host, _mqtt_port, callback, secure_layer);
```

### Connection Parameters
- **Host**: Configured via `_mqtt_host` (from JSON config)
- **Port**: Configured via `_mqtt_port` (typically 8883 for SSL)
- **Keep Alive**: 60 seconds
- **Buffer Size**: 2048 bytes for large JSON payloads
- **Client ID**: Uses `_device_id` for unique identification

## SSL/TLS Certificate Management

### Certificate Types
The system uses three AWS certificates stored in program memory:

1. **Root CA Certificate** (`AWS_CERT_CA`):
   - Amazon Root CA 1
   - Validates server authenticity
   - Hardcoded in program memory

2. **Device Certificate** (`AWS_CERT_CRT`):
   - AWS IoT device-specific certificate
   - Identifies the device to AWS IoT Core
   - Unique per device

3. **Private Key** (`AWS_CERT_PRIVATE`):
   - RSA private key for device authentication
   - Used for SSL handshake and encryption

### Certificate Setup
```cpp
secure_layer.setCACert(AWS_CERT_CA);
secure_layer.setCertificate(AWS_CERT_CRT);
secure_layer.setPrivateKey(AWS_CERT_PRIVATE);
```

## Topic Structure and Naming

### Topic Naming Convention
All topics follow the pattern: `AM/{device_id}/{subtopic}`

```cpp
String buildTopic(const char *topic) {
  String topic_S = String("AM/" + String(_device_id) + topic);
  return topic_S;
}
```

### Published Topics

#### Data Topics
- **`/sensors`**: Primary sensor data in JSON format
- **`/status`**: Device status and acknowledgments
- **`/debug`**: Debug messages and connection status
- **`/gps`**: GPS location data
- **`/reply/updating`**: OTA update progress messages

#### Configuration Topics
- **`/config/device`**: Device information and firmware version
- **`/config/hardware`**: Hardware configuration and sensor settings
- **`/config/server`**: Server connection parameters (excluding SSL keys)
- **`/config/network`**: Network configuration settings
- **`/config/sampling`**: Sampling intervals and data collection settings

### Subscribed Topics
- **`/cmd`**: Command reception for remote control and configuration

## MQTT Connection Management

### Connection Establishment (`setupMQTT()`)
```cpp
bool setupMQTT(int fail_count) {
  if (_is_lte_connected || _network_wifi_enabled) {
    if (client->connect(_device_id)) {
      client->setKeepAlive(60);
      client->setBufferSize(2048);
      
      // Publish initial status and configuration
      client->publish(buildTopic("/status").c_str(), _version);
      publish_config();
      publish_gps();
      
      // Subscribe to command topic
      client->subscribe(buildTopic("/cmd").c_str());
      return true;
    }
  }
  return false;
}
```

### Connection Monitoring (`serveMQQT()`)
```cpp
void serveMQQT(void) {
  if (!client->connected()) {
    if (client->connect(_device_id)) {
      client->subscribe(buildTopic("/cmd").c_str());
      Serial.println("MQTT Reconnected");
    }
  }
}
```

### Graceful Disconnection
```cpp
void closeMQTT(void) {
  if (client->connected()) {
    client->disconnect();
  }
  delete client;
  Serial.println("MQTT Disconnected");
}
```

## Message Publishing

### Sensor Data Publishing (`publish_data()`)
Primary function for publishing sensor readings:
```cpp
bool publish_data() {
  if (client->connected()) {
    if (client->publish(buildTopic("/sensors").c_str(), csvRJsonStr.c_str())) {
      publish_errors(); // Publish any error conditions
      return true;
    }
  }
  return false;
}
```

### Configuration Publishing (`publish_config()`)
Publishes complete device configuration across multiple topics:
```cpp
void publish_config(void) {
  String jsonstr = "";
  
  // Device configuration
  serializeJson(_config_flash["device"], jsonstr);
  client->publish(buildTopic("/config/device").c_str(), jsonstr.c_str());
  
  // Hardware configuration
  serializeJson(_config_flash["hardware"], jsonstr);
  client->publish(buildTopic("/config/hardware").c_str(), jsonstr.c_str());
  
  // Server configuration (excluding SSL keys)
  _config_flash["server"]["mqtt"].remove("ssl");
  serializeJson(_config_flash["server"], jsonstr);
  client->publish(buildTopic("/config/server").c_str(), jsonstr.c_str());
  
  // Network and sampling configuration
  serializeJson(_config_flash["network"], jsonstr);
  client->publish(buildTopic("/config/network").c_str(), jsonstr.c_str());
  
  _config_flash["sampling"]["engineering"] = _is_engineering;
  serializeJson(_config_flash["sampling"], jsonstr);
  client->publish(buildTopic("/config/sampling").c_str(), jsonstr.c_str());
}
```

### GPS Data Publishing (`publish_gps()`)
```cpp
void publish_gps(void) {
  String jsonstr = "";
  serializeJson(_config_flash["device"]["location"], jsonstr);
  client->publish(buildTopic("/gps").c_str(), jsonstr.c_str());
}
```

### Acknowledgment Publishing (`publish_ack()`)
```cpp
void publish_ack(const char *command, bool bAck) {
  String statusPayload = "{\"command\": \"" + String(command) + 
                        "\" , \"Ack\": " + String(bAck) + "}";
  client->publish(buildTopic("/status").c_str(), statusPayload.c_str());
}
```

## JSON Message Structure

### Sensor Data Message Format
The primary sensor data message (`/sensors` topic) contains:

```json
{
  "device_id": "AM-XXXX",
  "timestamp": 1234567890,
  "loopcounter": {"value": 123},
  "cpu_temp": {"value": 45.6},
  "battery_voltage": {"value": 3.7},
  "battery_gauge": {"value": 85.2},
  "battery_crate": {"value": -0.1},
  "temperature": {"value": 22.5},
  "humidity": {"value": 65.3},
  "pressure": {"value": 1013.25},
  "bmeAQI": {"value": 50},
  "windSpeed": {"value": 2.3},
  "winDir": {"value": 180},
  "tvoc": {"value": 150},
  "tvoc_max": {"value": 200},
  "pm1.0": {"value": 5.2},
  "pm2.5": {"value": 12.8},
  "pm10.0": {"value": 18.5},
  "pmAQI": {"value": 45},
  "ir_co2": {"value": 420},
  "h2s": {"value": 0.05},
  "ozone": {"value": 0.08},
  "so2": {"value": 0.02},
  "no2": {"value": 0.03},
  "mos_nh3": {"value": 0.1},
  "errors": {"value": 0},
  "summa_triggered": {"value": 0}
}
```

### Message Building Process (`buildJSON()`)
1. **CSV Parsing**: Parses CSV data into string tokens
2. **Bounds Checking**: Validates field count and prevents buffer overruns
3. **Conditional Inclusion**: Only includes sensor data if sensors are enabled and error-free
4. **Error Masking**: Uses bit-masked error flags to determine data validity
5. **JSON Serialization**: Converts structured data to JSON string

### Error Handling in Messages
Error conditions are tracked using bit-masked flags:
- **Bit 0 (1)**: IR sensor errors
- **Bit 1 (2)**: MPS sensor errors  
- **Bit 2 (4)**: VOC sensor errors
- **Bit 3 (8)**: NH3 sensor errors
- **Bit 4 (16)**: PM sensor errors
- **Bit 5 (32)**: SPEC sensor errors
- **Bit 7 (128)**: BME sensor errors

## Command Reception and Processing

### Command Message Format
Commands are received on the `/cmd` topic in JSON format:
```json
{
  "msg": "command_name",
  "parameter1": "value1",
  "parameter2": "value2"
}
```

### Callback Function (`callback()`)
The MQTT callback processes incoming commands:

```cpp
void callback(char* topic, byte* payload, unsigned int length) {
  // Parse JSON command
  deserializeJson(_user_cmd, payload_string);
  const char* command = _user_cmd["msg"];
  
  // Process command and set acknowledgment flag
  bool bAck = false;
  
  // Command processing logic...
  
  // Send acknowledgment
  publish_ack(command, bAck);
}
```

### Supported Commands

#### System Commands
- **`config`**: Request configuration republish
- **`errorlog`**: Request error log publication
- **`reset`**: System reset with optional delay
- **`rstlog`**: Clear data log files
- **`csq`**: Request signal quality report
- **`gps`**: Request GPS data publication

#### OTA Update Commands
- **`update`**: Firmware update with version specification
- **`json`**: Configuration file update

#### Sensor Calibration Commands
- **`configureSPEC`**: Configure electrochemical sensors
- **`calibrateSPEC`**: Zero calibration for SPEC sensors
- **`calibNH3`**: NH3 sensor calibration
- **`calibSPECdetect`**: Peak detection calibration
- **`configureIR`**: Configure infrared sensors
- **`calibIR`**: Zero calibration for IR sensors
- **`calibIRDetect`**: IR peak detection calibration
- **`caltemp`**: Temperature offset calibration

#### SUMMA Canister Commands
- **`summaReset`**: Reset SUMMA trigger state
- **`summaOpen`**: Manual valve open (debug)
- **`summaClose`**: Manual valve close (debug)
- **`summaTest`**: Run comprehensive tests
- **`summaConfig`**: Display current configuration

#### Configuration Commands
- **`updateSensorJson`**: Update individual sensor parameters
- **`eemode`**: Toggle engineering mode

## Error Handling and Recovery

### Connection Error Handling
- **Retry Mechanism**: Up to 3 MQTT connection attempts
- **Exponential Backoff**: 1-minute delays between retries
- **Connection Monitoring**: Continuous connection status checking
- **Automatic Reconnection**: Attempts reconnection on disconnect

### Message Delivery Reliability
- **Connection Verification**: Checks connection before publishing
- **Error Reporting**: Publishes error conditions alongside data
- **Acknowledgment System**: Confirms command reception and processing
- **Buffer Management**: 2048-byte buffer for large JSON payloads

### SSL/TLS Error Handling
- **Certificate Validation**: Automatic certificate chain validation
- **Handshake Monitoring**: SSL handshake error detection
- **Secure Channel**: Encrypted communication channel maintenance

## Performance Considerations

### Message Timing
- **Publish Delays**: 1-second delays between configuration messages
- **Keep-Alive**: 60-second heartbeat interval
- **Reconnection Timeout**: 1-minute retry intervals
- **Command Processing**: Immediate command acknowledgment

### Memory Management
- **Dynamic Allocation**: PubSubClient created dynamically during connection
- **Buffer Sizing**: 2048-byte buffer for JSON payloads
- **JSON Document**: Reusable JSON document for message parsing
- **String Management**: Efficient string handling for topic construction

### Network Efficiency
- **Selective Publishing**: Only publishes valid sensor data
- **Configuration Caching**: Avoids redundant configuration publishing
- **Error Aggregation**: Combines multiple error conditions in single message
- **Topic Optimization**: Hierarchical topic structure for efficient routing

## Integration with System Architecture

### Task Coordination
- **Data Collection Task**: Triggers sensor data publishing
- **Send Data Task**: Manages MQTT communication timing
- **Connection Management**: Coordinates with LTE modem operations
- **OTA Updates**: Pauses data collection during firmware updates

### Configuration Integration
- **JSON Configuration**: MQTT parameters loaded from config.json
- **Runtime Updates**: Configuration changes via MQTT commands
- **Persistent Storage**: Configuration saved to SD card and SPIFFS
- **Parameter Validation**: Input validation for configuration changes

### Error Reporting Integration
- **Sensor Errors**: Automatic error condition reporting
- **System Status**: Regular status updates and heartbeats
- **Diagnostic Information**: Debug message publishing
- **Recovery Notifications**: Status updates during error recovery