# Environmental Sensors Analysis

## Overview

The air monitoring system includes environmental sensors for measuring temperature, humidity, and atmospheric pressure using either BME280 or BME680 sensors. The BME680 additionally provides air quality index (AQI) measurements through gas resistance sensing. The system also monitors ESP32 internal CPU temperature for diagnostic purposes.

## Hardware Architecture

### Sensor Types and I2C Configuration

**I2C Address:**
- 0x76: BME280/BME680 sensor (Grove connector)

**Sensor Detection Priority:**
1. **BME280**: Basic temperature, humidity, pressure sensor
2. **BME680**: Advanced sensor with additional gas sensing for AQI calculation

The system automatically detects which sensor is present and configures accordingly.

### Grove Connector Interface

The environmental sensor connects via the Grove connector on the Weather Shield:
- **Grove I2C Check**: `(_tcaError & 0x01) == 0x01` indicates Grove connector issues
- **Fallback Operation**: System can operate without Grove connection for some configurations

## BME280 Sensor Processing

### Data Collection Process

The BME280 provides basic environmental measurements:

```cpp
// Temperature reading with offset compensation
float temp = bme280.readTemperature();
_temperature = (temp_average) + _temperature_offset;

// Pressure reading (converted from Pa to hPa)
float pressure = bme280.readPressure() / 100.0;

// Humidity reading
float humidity = bme280.readHumidity();
```

### Data Variables

**Temperature:**
```cpp
float _temperature;        // Current averaged temperature (°C)
float _temperature_sum;    // Accumulated temperature sum
float _temperature_offset; // Calibration offset
unsigned short _temp_smp_count; // Sample count
```

**Pressure:**
```cpp
float _pressure;           // Current averaged pressure (hPa)
float _pressure_sum;       // Accumulated pressure sum
unsigned short _pressure_smp_count; // Sample count
```

**Humidity:**
```cpp
float _humidity;           // Current averaged humidity (%RH)
float _humidity_sum;       // Accumulated humidity sum
unsigned short _humidity_smp_count; // Sample count
```

## BME680 Sensor Processing

### BSEC Library Integration

The BME680 uses Bosch's BSEC (Bosch Sensortec Environmental Cluster) library for advanced processing:

**Library Configuration:**
```cpp
bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
};
```

**Sample Rate:**
- `BSEC_SAMPLE_RATE_LP`: Low power mode (0.33 Hz sampling)

### Air Quality Index (AQI) Calculation

The BME680 provides sophisticated air quality measurements:

**AQI Variables:**
```cpp
float _bmeAQI;             // Current averaged AQI
float _bmeAQI_sum;         // Accumulated AQI sum
unsigned short _bmeAQI_smp_count; // AQI sample count
```

**Additional BME680 Outputs:**
- `bme680.gasResistance`: Raw gas sensor resistance (Ohms)
- `bme680.iaq`: Indoor Air Quality index
- `bme680.iaqAccuracy`: IAQ measurement accuracy (0-3)
- `bme680.co2Equivalent`: Estimated CO2 equivalent (ppm)
- `bme680.breathVocEquivalent`: Breath VOC equivalent
- `bme680.staticIaq`: Static IAQ value

### Data Collection Process

```cpp
if (bme680.run()) { // New data available
    // Process pressure
    _pressure_sum += bme680.pressure;
    _pressure = _pressure_sum / _pressure_smp_count;
    
    // Process AQI
    _bmeAQI_sum += bme680.iaq;
    _bmeAQI = _bmeAQI_sum / _bmeAQI_smp_count;
    
    // Process temperature with offset
    _temperature = (temp_average) + _temperature_offset;
    
    // Process humidity
    _humidity = humidity_average;
}
```

## ESP32 CPU Temperature Monitoring

### Internal Temperature Sensor

The system monitors ESP32 internal temperature for diagnostic purposes:

```cpp
float readTempESP() {
    SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR, 3, SENS_FORCE_XPD_SAR_S);
    SET_PERI_REG_BITS(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_CLK_DIV, 10, SENS_TSENS_CLK_DIV_S);
    CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
    CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP_FORCE);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
    ets_delay_us(100);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
    ets_delay_us(5);
    
    float temp_f = (float)GET_PERI_REG_BITS2(SENS_SAR_SLAVE_ADDR3_REG, SENS_TSENS_OUT, SENS_TSENS_OUT_S);
    float temp_c = (temp_f - 32) / 1.8;
    return temp_c;
}
```

**CPU Temperature Variables:**
```cpp
float _cpu_temp;           // Current averaged CPU temperature (°C)
float _cpu_temp_sum;       // Accumulated CPU temperature sum
unsigned short _cpu_temp_smp_count; // CPU temperature sample count
```

## Temperature Compensation and Calibration

### Temperature Offset Calibration

The system supports temperature offset calibration to correct for sensor mounting effects:

```cpp
void calibTemp(float offset) {
    _temperature_offset = offset;
    saveTempConfig();
}
```

**Application:**
```cpp
_temperature = (raw_temperature_average) + _temperature_offset;
```

### Temperature Memory Variables

For differential calculations and compensation:

```cpp
float _temperature_mem;    // Previous temperature for differential calculations
float _temperature_dx;     // Temperature differential
float _humidity_mem;       // Previous humidity for compensation
float _humidity_dx;        // Humidity differential
```

These variables support temperature compensation algorithms in other sensor systems.

## Data Averaging and Processing

### Sample Accumulation

All environmental measurements use accumulation averaging:

```cpp
// Temperature processing
_temperature_sum += raw_reading;
_temp_smp_count++;
_temperature = (_temperature_sum / _temp_smp_count) + _temperature_offset;
```

### Data Clearing

The `clearTPH()` function resets all accumulation variables:

```cpp
void clearTPH() {
    _temperature_sum = 0;
    _humidity_sum = 0;
    _pressure_sum = 0;
    _bmeAQI_sum = 0;
    _cpu_temp_sum = 0;
    
    _temp_smp_count = 0;
    _humidity_smp_count = 0;
    _pressure_smp_count = 0;
    _bmeAQI_smp_count = 0;
    _cpu_temp_smp_count = 0;
}
```

## Error Handling and Validation

### NaN Detection

The system validates all sensor readings for NaN (Not a Number) values:

```cpp
if (!isnan(sensor_reading)) {
    // Process valid reading
    _sum += sensor_reading;
    _count++;
} else {
    _bme_err = true; // Set error flag
}
```

### Temperature Range Validation

High temperature protection prevents sensor damage:

```cpp
if (_temperature > 65) { // 65°C threshold
    _bme_err = true;
    Serial.println("Warning: BME temperature too high, check the sensor");
}
```

### Error Flags

**Primary Error Flag:**
- `_bme_err`: Set when any environmental sensor reading fails

**I2C Communication:**
- `_grove_i2c`: Indicates Grove connector I2C communication status

## Sensor Initialization Process

### Auto-Detection Sequence

```cpp
void setupTHP() {
    // Check Grove connector
    if ((_tcaError & 0x01) == 0x01) {
        Serial.println("THP Grove not found");
    } else {
        _grove_i2c = true;
    }
    
    // Try BME280 first
    if (bme280.begin(0x76)) {
        Serial.println("Found BME280");
        _is_bme280_on = true;
        return;
    }
    
    // Try BME680 if BME280 not found
    bme680.begin(0x76, Wire);
    if (bme680.bme68xStatus == BME68X_OK) {
        Serial.println("Found BME680");
        // Configure BSEC library
        bme680.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
        _is_bme680_on = true;
    } else {
        Serial.println("No environmental sensor found");
    }
}
```

## Data Output Format

### Standard Measurements
- **Temperature**: Degrees Celsius (°C) with offset compensation
- **Humidity**: Relative Humidity percentage (%RH)
- **Pressure**: Hectopascals (hPa)

### BME680 Additional Outputs
- **AQI**: Air Quality Index (0-500 scale)
- **Gas Resistance**: Ohms (raw sensor resistance)
- **CO2 Equivalent**: Estimated CO2 concentration (ppm)
- **VOC Equivalent**: Volatile Organic Compound equivalent

### Diagnostic Outputs
- **CPU Temperature**: ESP32 internal temperature (°C)
- **Error Flags**: Communication and validation status
- **Sample Counts**: Number of accumulated samples

### Serial Output Format

**BME280 Output:**
```
BME280: 23.45°C, 1013.25hPA, 45.67RH, 0ERR, 2.5OFFSET°C
```

**BME680 Output:**
```
BME680: 1013.25hPA, 123456OHM, 25.3AQI - 3, 23.45°C, 45.67RH, 407CO2, 0ERR, 2.5OFFSET°C
```

**CPU Temperature:**
```
CPU Temp: 45.2°C
```

The environmental sensor system provides comprehensive atmospheric monitoring with automatic sensor detection, calibration support, and robust error handling for reliable environmental data collection.