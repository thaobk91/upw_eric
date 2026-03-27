# Data Logging and Indexing System Analysis

## Overview

The air monitoring system implements a sophisticated data logging and indexing system that stores sensor readings in CSV format with a binary index file for efficient data retrieval and transmission tracking. The system uses SD card storage with mutex-protected access and includes recovery mechanisms for reliable data management.

## File Structure and Organization

### Primary Data Files
- **`/log.csv`**: Main data file containing sensor readings in CSV format
- **`/log.idx`**: Binary index file mapping CSV record positions
- **`/mark.idx`**: Marker file for transmission status tracking (RAM-based with periodic saves)
- **`/config.json`**: Configuration file stored on both SD card and SPIFFS

### File Locations
- **SD Card**: Primary storage location for data files
- **SPIFFS**: Backup storage for configuration files
- **RAM Buffer**: Temporary storage for index markers (`_markIdx[]`)

## CSV Data Format and Structure

### CSV Record Format (`buildCSV()`)
The CSV format contains 69 fields in a fixed structure:

```csv
ID,Time,LoopCount,EFormat,CPUTemp,VBat,SOC,CRate,
Temp_err,Temp,Humidity,ATM,bmeAQI,WindSpeed,WindDir,
voc_err,VOC,VOC_max,EVOC,ECO2,pm_err,pm1.0,pm2.5,pm10,pmAQI,
ir_err,ir_C1,ir_C1_max,ir_CO2,ir_CO2_max,PID,PID_max,
mps_err,mps_c1,mps_c1_max,EC_err,H2S,H2S_max,O3,O3_max,SO2,SO2_max,
NO2,NO2_max,nh3_err,mos_CO,mos_NO2,NH3,NH3_max,Errors,
Temp_dx,Humidity_dx,
C1_signal,C1_base,C1_top,C1_dx,C1_found,
H2S_signal,H2S_base,H2S_top,H2S_dx,H2S_found,
VOC_signal,VOC_base,VOC_top,VOC_dx,VOC_found,VOC_nocomp_max,
summa_triggered
```

### Field Categories

#### Standard Data Fields (0-49)
- **Device Information**: Device ID, timestamp, loop counter
- **System Status**: Engineering mode flag, CPU temperature
- **Power Management**: Battery voltage, state of charge, charge rate
- **Environmental Data**: Temperature, humidity, pressure, BME AQI
- **Wind Data**: Wind speed and direction
- **Gas Sensors**: VOC, TVOC, eCO2 readings
- **Particulate Matter**: PM1.0, PM2.5, PM10, PM AQI
- **Infrared Sensors**: C1, CO2, PID readings with max values
- **MPS Sensors**: Micro-particle sensor concentration
- **Electrochemical Sensors**: H2S, O3, SO2, NO2 with max values
- **NH3 Sensors**: Ammonia readings
- **Error Flags**: Binary error code (field 49)

#### Extended Data Fields (50-68)
- **Temperature Differentials**: Temperature and humidity deltas
- **Signal Processing Data**: Signal, base, top, and differential values for C1, H2S, VOC
- **Peak Detection**: Found flags for various sensors
- **SUMMA Control**: Canister trigger state (field 68)

### CSV Building Process
```cpp
void buildCSV(void) {
  char scratch[2048];
  csvWBuffer[0] = 0;
  
  // Build CSV string with formatted sensor data
  strcpy(csvWBuffer, _device_id);
  sprintf(scratch, ",%lu,%lu,%u,%.1f,", _time_stamp, _loop_counter, _is_engineering, _cpu_temp);
  strcat(csvWBuffer, scratch);
  
  // Continue building with all sensor data...
  sprintf(scratch, "%u\r\n", _summa_triggered); // Final field
  strcat(csvWBuffer, scratch);
}
```

## Data Storage System

### CSV File Management (`log_Store()`)
```cpp
bool log_Store(void) {
  if (xSemaphoreTake(sd_mutex, portMAX_DELAY)) {
    File csvFile = SD.open("/log.csv", FILE_APPEND);
    _csvPointer = csvFile.position(); // Record file position
    csvFile.print(csvWBuffer);
    csvFile.flush();
    csvFile.close();
    xSemaphoreGive(sd_mutex);
    return true;
  }
  return false;
}
```

### File Creation and Initialization (`log_Create()`)
1. **Header Creation**: Writes CSV header with field names
2. **Index File Reset**: Removes existing index files
3. **Marker Initialization**: Creates marker index system
4. **Pointer Initialization**: Sets up read/write pointers

### File Clearing (`log_Clear()`)
- Removes existing log.csv file
- Resets all indexing structures
- Prepares for fresh data collection

## Binary Index System

### Index File Structure (`/log.idx`)
Each index record contains:
- **CSV Pointer** (4 bytes): File position of corresponding CSV record
- **Status Marker** (1 byte): Transmission status flag

### Index Record Format
```cpp
struct IndexRecord {
  uint32_t csvPointer;  // Position in CSV file
  uint8_t statusMarker; // 0x55=new, 0xAA=sent, 0xA5=error
};
```

### Index Storage (`logIdx_Store()`)
```cpp
void logIdx_Store(void) {
  uint32_t _idxPointer = _csvWIdx * (sizeof(_csvPointer) + sizeof(_idxMarker));
  byte _idxMarker = 0x55; // New record marker
  
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

### Index Retrieval (`logIdx_Fetch()`)
```cpp
uint32_t logIdx_Fetch(int _idxPointer) {
  uint32_t _csvPointer;
  byte _idxMarker = 0;
  
  if (xSemaphoreTake(sd_mutex, portMAX_DELAY)) {
    File idxFile = SD.open("/log.idx", FILE_READ);
    uint32_t position = _idxPointer * (sizeof(_csvPointer) + sizeof(_idxMarker));
    idxFile.seek(position);
    idxFile.read((uint8_t*)&_csvPointer, sizeof(_csvPointer));
    idxFile.read((uint8_t*)&_idxMarker, sizeof(_idxMarker));
    idxFile.close();
    xSemaphoreGive(sd_mutex);
    
    return (_idxMarker == 0x55) ? _csvPointer : -1; // Only return valid records
  }
  return -1;
}
```

## Transmission Status Tracking

### Status Marker System
- **0x55**: New record (not transmitted)
- **0xAA**: Successfully transmitted
- **0xA5**: Transmission error

### RAM-Based Marker System
```cpp
byte _markIdx[32768]; // Each bit represents a record status
```

### Marker Operations (`logIdx_Mark()`)
```cpp
void logIdx_Mark(int _idxPointer, byte _idxMarker) {
  // Bounds checking
  if (_idxPointer < 0 || _idxPointer > MAX_IDX_VALUE) {
    _idxPointer = 0; // Reset to safe value
  }
  
  // Update marker in RAM buffer
  int _D1 = _idxPointer / 8;  // Byte index
  int _D2 = _idxPointer % 8;  // Bit index
  byte _b1 = 1 << _D2;       // Bit mask
  
  if (_idxMarker == 0) {
    _markIdx[_D1] &= ~_b1;    // Clear bit (mark as sent)
  } else {
    _markIdx[_D1] |= _b1;     // Set bit (mark as new/error)
  }
}
```

### Marker File Persistence (`wMarker()`)
```cpp
void wMarker(void) {
  if (xSemaphoreTake(sd_mutex, portMAX_DELAY)) {
    File markFile = SD.open("/mark.idx", FILE_WRITE);
    markFile.seek(0);
    markFile.write(_markIdx, sizeof(_markIdx));
    markFile.flush();
    markFile.close();
    xSemaphoreGive(sd_mutex);
  }
}
```

## Data Retrieval System

### CSV Record Fetching (`log_Fetch()`)
```cpp
int log_Fetch(uint32_t _csvPointer) {
  if (xSemaphoreTake(sd_mutex, portMAX_DELAY)) {
    File file = SD.open("/log.csv", FILE_READ);
    if (file.seek(_csvPointer)) {
      size_t bytesRead = file.readBytesUntil('\n', csvRBuffer, sizeof(csvRBuffer) - 1);
      csvRBuffer[bytesRead] = '\0';
      file.close();
      xSemaphoreGive(sd_mutex);
      return _csvPointer;
    }
    file.close();
    xSemaphoreGive(sd_mutex);
  }
  return -1;
}
```

### Data Transmission Flow
1. **Index Lookup**: `logIdx_Fetch(_csvRIdx)` retrieves CSV file position
2. **Data Retrieval**: `log_Fetch(_csvPointer)` reads CSV record
3. **JSON Conversion**: `buildJSON()` converts CSV to JSON format
4. **Transmission**: `publish_data()` sends data via MQTT
5. **Status Update**: `logIdx_Mark(_csvRIdx, 0)` marks record as sent

## Queue Management and Recovery

### Read/Write Pointer Management
- **`_csvWIdx`**: Write index (next record to write)
- **`_csvRIdx`**: Read index (next record to transmit)
- **`_csvPointer`**: Current CSV file position

### Recovery Mechanisms

#### Advanced Index Fetching (`logIdx_Fetch_422()`)
Implements bidirectional search for untransmitted records:
```cpp
uint32_t logIdx_Fetch_422(int _idxPointer) {
  // Search forward and backward for untransmitted records
  // Handles recovery scenarios and transmission gaps
  
  // Forward search
  for (int i = _csvRIdx_current; i < _csvWIdx; i++) {
    if (marker == 0x55) return csvPointer; // Found untransmitted record
  }
  
  // Backward search for missed records
  for (int i = _csvRIdx_current - 1; i >= 0; i--) {
    if (marker == 0x55) return csvPointer; // Found missed record
  }
}
```

#### Transmission Queue Recovery
- **Gap Detection**: Identifies missing transmissions in sequence
- **Retry Logic**: Attempts retransmission of failed records
- **Error Handling**: Marks persistently failing records with error status

### Bounds Checking and Safety
All index operations include comprehensive bounds checking:
```cpp
if (_idxPointer < 0) {
  Serial.printf("Invalid _idxPointer %i, resetting to 0\r\n", _idxPointer);
  _idxPointer = 0;
}
if (_idxPointer > MAX_IDX_VALUE) {
  Serial.printf("_idxPointer %i too large, resetting to 0\r\n", _idxPointer);
  _idxPointer = 0;
}
```

## Mutex Protection and Concurrency

### SD Card Access Protection
All SD card operations use mutex protection:
```cpp
extern SemaphoreHandle_t sd_mutex;

if (xSemaphoreTake(sd_mutex, portMAX_DELAY)) {
  // Perform SD card operations
  xSemaphoreGive(sd_mutex);
} else {
  Serial.println("Failed to take SD mutex");
}
```

### Task Coordination
- **Data Collection Task**: Writes new records
- **Data Transmission Task**: Reads and transmits records
- **Configuration Tasks**: Access configuration files
- **OTA Update Tasks**: Temporary exclusive access

## Storage Capacity and Limits

### File Size Calculations
- **Record Size**: ~200-300 bytes per CSV record
- **Index Size**: 5 bytes per index record
- **Capacity**: Designed for 90 days of 30-second intervals
- **Total Records**: 60 × 2 × 24 × 90 = 259,200 records

### Memory Management
- **CSV Buffer**: 2048 bytes for record building
- **Read Buffer**: 2048 bytes for record retrieval
- **Marker Array**: 32,768 bytes (256KB bits) for status tracking
- **JSON Buffer**: Dynamic allocation for message formatting

## Error Handling and Diagnostics

### File Operation Errors
- **File Open Failures**: Logged with specific error messages
- **Seek Operation Failures**: Handled with position validation
- **Write Failures**: Detected through return value checking
- **Mutex Timeout**: Prevents deadlock conditions

### Data Integrity Checks
- **Bounds Validation**: All array and file accesses bounds-checked
- **Pointer Validation**: CSV pointers validated before use
- **Marker Consistency**: Status markers validated during operations
- **File Size Monitoring**: Prevents excessive file growth

### Recovery Procedures
- **File Recreation**: Automatic recreation of corrupted files
- **Index Rebuilding**: Recovery from index file corruption
- **Pointer Reset**: Safe reset of corrupted pointers
- **Graceful Degradation**: Continues operation with reduced functionality

## Performance Considerations

### Optimization Strategies
- **Append-Only Writes**: Efficient sequential writing to CSV file
- **Binary Index**: Fast random access to specific records
- **RAM Buffering**: Reduces SD card write cycles for markers
- **Mutex Minimization**: Short critical sections for concurrent access

### Storage Efficiency
- **Compact Binary Format**: Efficient index record storage
- **Bit-Packed Markers**: Minimal memory usage for status tracking
- **Selective Transmission**: Only transmits new/failed records
- **Periodic Cleanup**: Removes old transmitted records (when implemented)

## Integration with System Architecture

### Configuration Integration
- **Storage Settings**: SD card enable/disable configuration
- **Capacity Limits**: Configurable storage limits and rotation
- **Backup Strategy**: SPIFFS backup for critical configuration

### Communication Integration
- **MQTT Transmission**: Seamless integration with MQTT publishing
- **JSON Conversion**: Automatic CSV-to-JSON transformation
- **Status Reporting**: Transmission status feedback to monitoring systems
- **Error Reporting**: Integration with system error reporting mechanisms