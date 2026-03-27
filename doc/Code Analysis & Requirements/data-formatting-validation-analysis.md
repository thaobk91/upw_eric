# Data Formatting and Validation Analysis

## Overview

The air monitoring system implements comprehensive data formatting and validation mechanisms to ensure data integrity, proper CSV structure, and reliable error detection. The system processes sensor data through averaging, validation, and structured formatting before storage and transmission.

## CSV Data Format Structure

### buildCSV() Function Implementation

```cpp
void buildCSV(void) {
    char scratch[2048];
    scratch[0] = 0;
    csvWBuffer[0] = 0;
    
    taskDelay(10,1); // yield to the OS
    
    // Build CSV string with concatenated sensor data
    strcpy(csvWBuffer, _device_id);
    sprintf(scratch, ",%lu,%lu,%u,%.1f,", _time_stamp, _loop_counter, _is_engineering, _cpu_temp);
    strcat(csvWBuffer, scratch);
    // ... continue building CSV fields
}
```

### CSV Field Structure (69 Fields Total)

#### Standard Data Fields (Fields 0-48)
```cpp
// Device identification and system status
"ID, Time, LoopCount, EFormat, CPUTemp, VBat, SOC, CRate, "

// Environmental sensors
"Temp_err, Temp, Humidity, ATM, bmeAQI, WindSpeed, WindDir, "

// VOC sensors
"voc_err, VOC, VOC_max, EVOC, ECO2, pm_err, pm1.0, pm2.5, pm10, pmAQI, "

// Infrared sensors
"ir_err, ir_C1, ir_C1_max, ir_CO2, ir_CO2_max, PID, PID_max, "

// MPS and electrochemical sensors
"mps_err, mps_c1, mps_c1_max, EC_err, H2S, H2S_max, O3, O3_max, SO2, SO2_max, "

// Additional electrochemical and error data
"NO2, NO2_max, nh3_err, mos_CO, mos_NO2, NH3, NH3_max, Errors, "
```

#### Extended Data Fields (Fields 49-68)
```cpp
// Temperature and humidity differentials
"Temp_dx, Humidity_dx, "

// Peak detection data for C1 sensor
"C1_signal, C1_base, C1_top, C1_dx, C1_found, "

// Peak detection data for H2S sensor
"H2S_signal, H2S_base, H2S_top, H2S_dx, H2S_found, "

// Peak detection data for VOC sensor and additional fields
"VOC_signal, VOC_base, VOC_top, VOC_dx, VOC_found, "

// Additional VOC data and SUMMA trigger state
"VOC_nocomp_max, summa_triggered"
```

### Field Data Types and Formatting

#### Device and System Fields
```cpp
strcpy(csvWBuffer, _device_id);                                    // Field 0: String
sprintf(scratch, ",%lu,%lu,%u,%.1f,", _time_stamp, _loop_counter,  // Fields 1-2: Unsigned long
        _is_engineering, _cpu_temp);                               // Fields 3-4: Unsigned int, Float
```

#### Battery and Power Fields
```cpp
sprintf(scratch, "%.2f,%.1f,%.1f,", _vbat, _state_off_charge, _bat_change_rate);
// Fields 5-7: Battery voltage (V), State of charge (%), Charge rate (%)
```

#### Environmental Sensor Fields
```cpp
sprintf(scratch, "%u,%.1f,%.1f,%.2f,%.1f,%u,%u,",
        _bme_err, _temperature, _humidity, _pressure, _bmeAQI, _wind_speed, _wind_dir);
// Fields 8-14: Error flag, Temperature (°C), Humidity (%), Pressure (hPa), AQI, Wind speed, Wind direction
```

#### Gas Sensor Fields
```cpp
sprintf(scratch, "%u,%0.4f,%f,%0.4f,%0.4f,", _voc_err, _voc, _voc_raw_max, _tvoc, _eco2);
// Fields 15-19: VOC error, VOC concentration, VOC max, TVOC, eCO2

sprintf(scratch, "%u,%0.4f,%f,%0.4f,%f,%0.4f,%f,",
        _ir_err, _c1, _c1_max, _co2, _co2_max, _pid, _pid_max);
// Fields 25-31: IR error, C1 concentration, C1 max, CO2 concentration, CO2 max, PID concentration, PID max
```

#### Electrochemical Sensor Fields
```cpp
sprintf(scratch, "%u,%0.4f,%f,%0.4f,%f,%0.4f,%f,%0.4f,%f,",
        _spec_err, _h2s, _h2s_max, _o3, _o3_max, _so2, _so2_max, _no2, _no2_max);
// Fields 35-43: SPEC error, H2S concentration/max, O3 concentration/max, SO2 concentration/max, NO2 concentration/max
```

#### Peak Detection Fields
```cpp
sprintf(scratch, "%0.4f,%0.4f,%0.4f,%0.4f,%u,", 
        _h2s_signal, _h2s_base, _h2s_top, _h2s_dx, _h2s_found);
// Fields 55-59: Peak signal strength, baseline, top value, derivative, found flag
```

## Data Validation and Error Handling

### Error Flag System (formatError Function)

```cpp
void formatError(void) {
    _errors = 0;
    
    // Bit 0: Infrared sensor errors
    if (_ir_err) _errors |= 1;
    if ((_ir_i2c == false) && (_c1_enable || _co2_enable || _pid_enable)) _errors |= 1;
    
    // Bit 1: MPS sensor errors
    if (_mps_err) _errors |= 2;
    if ((_mps_i2c == false) && (_mps_enable)) _errors |= 2;
    
    // Bit 2: VOC sensor errors
    if (_voc_err) _errors |= 4;
    if ((_voc_i2c == false) && (_voc_enable || _voc_mox_enable)) _errors |= 4;
    
    // Bit 3: NH3 sensor errors
    if (_nh3_err) _errors |= 8;
    if ((_nh3_i2c == false) && (_nh3_enable) && (_WS_version != 14)) _errors |= 8;
    if ((_ir_i2c == false) && (_nh3_enable) && (_WS_version == 14)) _errors |= 8;
    
    // Bit 4: Particulate matter sensor errors
    if (_pm_err > 0) _errors |= 16;
    if ((_pm_i2c == false) && (_pm_enable)) _errors |= 16;
    
    // Bit 5: Electrochemical sensor errors
    if (_spec_err || _o3_err || _so2_err || _no2_err || _h2s_err) _errors |= 32;
    if ((_SPEC_i2c == false) && (_h2s_enable || _so2_enable || _no2_enable || _o3_enable)) _errors |= 32;
    
    // Bit 6: RTC errors
    if (_rtc_err) _errors |= 64;
    
    // Bit 7: Environmental sensor errors
    if (_bme_err) _errors |= 128;
    if ((_grove_i2c == false) && (_is_bme280_on == false) && (_is_bme680_on == false)) _errors |= 128;
}
```

### Error Bit Mapping
- **Bit 0 (0x01)**: Infrared sensors (CO2, C1, PID)
- **Bit 1 (0x02)**: MPS (Micro Particle Sensor)
- **Bit 2 (0x04)**: VOC sensors
- **Bit 3 (0x08)**: NH3 sensors
- **Bit 4 (0x10)**: Particulate matter sensors (PM1.0, PM2.5, PM10)
- **Bit 5 (0x20)**: Electrochemical sensors (H2S, O3, SO2, NO2)
- **Bit 6 (0x40)**: Real-time clock
- **Bit 7 (0x80)**: Environmental sensors (BME280/BME680)

### Error Loop Counter
```cpp
if ((_errors & 0xBD) > 0) { // Exclude RTC and MPS errors (binary: 1011 1101)
    _err_loop += 1;
}
```

## Data Averaging and Sample Counting

### Environmental Sensors (BME280/BME680)
```cpp
// Temperature averaging
_temperature_sum += S1;
_temp_smp_count++;
_temperature = (_temperature_sum / _temp_smp_count) + _temperature_offset;

// Humidity averaging
_humidity_sum += S1;
_humidity_smp_count++;
_humidity = _humidity_sum / _humidity_smp_count;

// Pressure averaging
_pressure_sum += S1;
_pressure_smp_count++;
_pressure = _pressure_sum / _pressure_smp_count;
```

### Wind Sensors
```cpp
// Wind speed and direction averaging
_wind_speed += windSpeed;
_wind_dir += windDirection;
_wind_sample_count++;

// Format averaged values
void formatWind(void) {
    if (_wind_sample_count > 0) {
        _wind_speed = _wind_speed / _wind_sample_count;
        _wind_dir = _wind_dir / _wind_sample_count;
    }
    // Reset for next cycle
    _wind_sample_count = 0;
}
```

### VOC Sensors
```cpp
// VOC sample accumulation
_voc_sample_count++;

// Format averaged values
void formatVOC() {
    if (_voc_sample_count > 0) {
        _voc_raw = _voc_raw / _voc_sample_count;
        _tvoc = _tvoc / _voc_sample_count;   // SGP30
        _eco2 = _eco2 / _voc_sample_count;   // SGP30
    }
    _voc_sample_count = 0;
}
```

### Particulate Matter Sensors
```cpp
// PM sample accumulation
_pm_sample_count += 1;

// Format averaged values
void formatPM(void) {
    if (_pm_sample_count > 0) {
        _pm1 = _pm1 / _pm_sample_count;
        _pm2_5 = _pm2_5 / _pm_sample_count;
        _pm10 = _pm10 / _pm_sample_count;
        _pmAQI = _pmAQI / _pm_sample_count;
    }
}
```

### Global Sample Counter
```cpp
int _sample_count = 0;

void collectSensors(String output) {
    // ... collect all sensor data
    _sample_count++;
    Serial.printf("============= EOF Collect %i/%i\r\n", _sample_count, _report_interval_count);
}
```

## JSON Data Formatting (buildJSON Function)

### Field Bounds Checking
```cpp
// Bounds checking and diagnostics
const size_t EXPECTED_FIELDS = 69; // Current field count including SUMMA
size_t n = (size_t)index;
Serial.printf("buildJSON(): tokens=%u expected=%u\n", (unsigned)n, (unsigned)EXPECTED_FIELDS);

// Safe bounds check for error field access
if (n > 49) {
    LoggedErr = atoi(strings[49]); // Error field at index 49
}
```

### Bounds-Checked Field Mapping
```cpp
// Device and system fields
if (limit > 0) _mqtt_buffer["device_id"] = strings[0];
if (limit > 1) _mqtt_buffer["timestamp"] = atoi(strings[1]);
if (limit > 4) _mqtt_buffer["cpu_temp"]["value"] = atof(strings[4]);

// Battery fields with bounds checking
if (limit > 5) _mqtt_buffer["battery_voltage"]["value"] = atof(strings[5]);
if (limit > 6) _mqtt_buffer["battery_gauge"]["value"] = atof(strings[6]);

// Error field with bounds check
if (limit > 49) {
    _mqtt_buffer["errors"]["value"] = atoi(strings[49]);
}
```

## Data Storage and Indexing

### CSV Data Storage
```cpp
bool log_Store(void) {
    // Store CSV data to SD card
    File csvFile = SD.open("/log.csv", FILE_APPEND);
    if (!csvFile) {
        return false;
    }
    
    _csvPointer = csvFile.position(); // Record file position
    csvFile.print(csvWBuffer);        // Write CSV data
    csvFile.flush();
    csvFile.close();
    
    logIdx_Store(); // Store index entry
    return true;
}
```

### Index File Management
```cpp
void logIdx_Store(void) {
    uint32_t _idxPointer = 0;
    byte _idxMarker = 0x55; // 0x55=new, 0xAA=sent, 0xA5=error
    
    File idxFile = SD.open("/log.idx", FILE_WRITE);
    _idxPointer = _csvWIdx * (sizeof(_csvPointer) + sizeof(_idxMarker));
    
    if (idxFile.seek(_idxPointer) == true) {
        idxFile.write((uint8_t*)&_csvPointer, sizeof(_csvPointer)); // File position
        idxFile.write((uint8_t*)&_idxMarker, sizeof(_idxMarker));   // Status marker
        idxFile.flush();
        idxFile.close();
    }
}
```

### Index Marker System
- **0x55**: New record (not transmitted)
- **0xAA**: Successfully transmitted
- **0xA5**: Transmission error

## Data Validation Mechanisms

### Bounds Checking for Index Operations
```cpp
// Bounds check for _idxPointer
if (_idxPointer < 0) {
    Serial.printf("logIdx_Mark_422: Invalid _idxPointer %i, resetting to 0\r\n", _idxPointer);
    _idxPointer = 0;
}

// Bounds check for _csvRIdx_current
if (_csvRIdx_current < 0) {
    Serial.printf("logIdx_Fetch_422: Invalid _csvRIdx_current %i, resetting to 0\r\n", _csvRIdx_current);
    _csvRIdx_current = 0;
}
```

### NaN and Invalid Value Handling
```cpp
// Temperature sensor validation
S1 = bme280.readTemperature();
if (!isnan(S1)) {
    _temperature_sum += S1;
    _temp_smp_count++;
    _temperature = (_temperature_sum / _temp_smp_count) + _temperature_offset;
}

// Pressure sensor validation
S1 = bme280.readPressure() / 100.0;
if (!isnan(S1)) {
    _pressure_sum += S1;
    _pressure_smp_count++;
    _pressure = _pressure_sum / _pressure_smp_count;
}
```

### Negative Value Protection
```cpp
// Prevent negative concentrations
if (concentration < 0) { concentration = 0; }
if (S1 < 0) { S1 = 0; }
if (offset < 0) { offset = 0; }
```

## Error Reporting and Diagnostics

### Serial Debug Output
```cpp
Serial.printf("Err#/Tca/Count/Ctx:%u/%u/%u/%u  rtc:%u  mps/cnt:%u/%u  ir:%u  voc:%u  nh3:%u  pm:%u  spec:%u  bme:%u\r\n",
    _errors, _tcaError, _err_loop, _ctxerr_count, _rtc_err, _mps_err, _mps_err_count, 
    _ir_err, _voc_err, _nh3_err, _pm_err, _spec_err, _bme_err);

Serial.printf("SPEC:%u  H2S:%u  O3:%u  SO2:%u  NO2:%u\r\n", 
    _spec_err, _h2s_err, _o3_err, _so2_err, _no2_err);
```

### MQTT Error Publishing
```cpp
bool publish_errors(bool userRqt = false) {
    char scratch[128];
    sprintf(scratch, "Err#/Tca/Count/Ctx:%u/%u/%u/%u ",
        _errors, _tcaError, _err_loop, _ctxerr_count);
    
    sprintf(subscratch, "rtc:%u  mps/cnt:%u/%u  ir:%u  voc:%u  pm:%u  bme:%u  ",
        _rtc_err, _mps_err, _mps_err_count, _ir_err, _voc_err, _pm_err, _bme_err);
    
    // Publish error status via MQTT
    return client->publish(buildTopic("/errors").c_str(), scratch);
}
```

## Data Integrity Features

### CSV Header Validation
```cpp
// Create CSV header with all field names
strcat(csvBuffer, "ID, Time, LoopCount, EFormat, CPUTemp, VBat, SOC, CRate, ");
strcat(csvBuffer, "Temp_err, Temp, Humidity, ATM, bmeAQI, WindSpeed, WindDir, ");
// ... complete header with 69 fields
```

### Field Count Validation
```cpp
const size_t EXPECTED_FIELDS = 69;
size_t n = (size_t)index;
if (n != EXPECTED_FIELDS) {
    Serial.printf("WARNING: Field count mismatch: %u expected %u\n", 
                  (unsigned)n, (unsigned)EXPECTED_FIELDS);
}
```

### Data Type Consistency
- **Integers**: Loop counters, error flags, boolean states
- **Floats**: Sensor readings, concentrations, voltages
- **Strings**: Device ID, timestamps
- **Fixed Precision**: Consistent decimal places for each sensor type

### Memory Management
```cpp
char scratch[2048];        // Temporary buffer for sprintf operations
scratch[0] = 0;           // Initialize buffer
csvWBuffer[0] = 0;        // Clear main CSV buffer
taskDelay(10,1);          // Yield to OS during processing
```

## Performance Considerations

### Efficient String Building
- Uses `sprintf()` for formatted number conversion
- Uses `strcat()` for efficient string concatenation
- Minimizes memory allocations with fixed buffers

### Sample Rate Management
- Configurable sample intervals via `_report_interval_count`
- Averaging reduces noise and storage requirements
- Peak detection captures transient events

### Error Recovery
- Automatic sensor reinitialization on I2C errors
- Graceful degradation when sensors fail
- Persistent error logging for diagnostics