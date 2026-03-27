# Peak Detection Algorithms Analysis

## Overview

The air monitoring system implements a sophisticated peak detection algorithm that is applied to multiple sensor types to identify gas concentration spikes above baseline levels. This algorithm is crucial for detecting transient gas events and pollution episodes.

## Peak Detection Function

### Function Signature
```cpp
void peakDetect(String output, float *_volt_corr, float *_volt_max, 
                float *_signal, float *_base, float *_top, float *_mem, 
                float *_tcomp, float *_dx, float *_trig, bool *_found)
```

### Core Variables and Their Roles

#### Signal Processing Variables
- **`_signal`**: Current peak signal strength (corrected voltage - baseline)
- **`_base`**: Baseline voltage level recorded when peak detection starts
- **`_top`**: Maximum signal strength reached during current peak event
- **`_mem`**: Memory of previous corrected voltage reading for derivative calculation
- **`_dx`**: First derivative (rate of change) = current_corrected_voltage - previous_memory
- **`_trig`**: Trigger threshold for peak detection start/end
- **`_found`**: Boolean flag indicating if a peak is currently being tracked

#### Temperature Compensation Variables
- **`_volt_corr`**: Temperature-compensated voltage reading
- **`_volt_max`**: Raw maximum voltage reading from sensor
- **`_tcomp`**: Temperature compensation coefficient
- **`_temperature_mem`**: Temperature at time of compensation calculation

## Algorithm Logic Flow

### 1. Temperature Compensation
```cpp
*_volt_corr = *_volt_max - ((_temperature - _temperature_mem) * *_tcomp);
if (*_volt_corr < 0) { *_volt_corr = 0; }
```
- Corrects sensor reading for temperature drift
- Uses linear temperature compensation: `corrected = raw - (temp_delta * temp_coeff)`
- Prevents negative corrected values

### 2. Peak Detection Delay
```cpp
const unsigned long _peakDetect_delay = 4;
DLoop = _loop_counter;
if (DLoop >= _peakDetect_delay) {
    // Peak detection logic executes here
}
```
- Peak detection only starts after 4 measurement cycles
- Allows system to stabilize before detecting peaks

### 3. Derivative Calculation
```cpp
*_dx = *_volt_corr - *_mem;
```
- Calculates first derivative (rate of change)
- Positive values indicate rising signal
- Used for both peak start and end detection

### 4. Peak Start Detection
```cpp
if ((*_dx >= *_trig) && (*_found == false)) {
    *_found = true;
    *_base = *_mem;
    *_top = 0;
}
```
- Peak starts when derivative exceeds trigger threshold
- Records baseline level from previous memory value
- Resets top value tracking
- Sets found flag to true

### 5. Baseline Validation
```cpp
if (*_base > *_volt_max) {
    *_base = *_volt_corr;
    *_found = false;
}
```
- Prevents baseline from being higher than current reading
- Resets peak detection if baseline becomes invalid
- Ensures signal calculation remains positive

### 6. Signal Calculation
```cpp
*_signal = *_volt_corr - *_base;
```
- Signal strength = current corrected voltage - baseline
- Represents the magnitude of the detected peak
- Always positive due to baseline validation

### 7. Peak Top Tracking
```cpp
if ((*_found == true) && (*_signal > *_top)) {
    *_top = *_signal;
}
```
- Continuously tracks maximum signal strength during peak event
- Only updates when current signal exceeds previous maximum

### 8. Peak End Detection
```cpp
if ((*_dx <= *_trig) && (*_found == true) && (*_signal <= (*_top * 0.1))) {
    *_found = false;
}
```
- Peak ends when three conditions are met:
  1. Derivative falls to or below trigger threshold (signal declining)
  2. Peak is currently being tracked
  3. Current signal drops to 10% or less of peak maximum
- Resets found flag to allow detection of next peak

### 9. Signal Reset
```cpp
if (*_found == false) {
    *_signal = 0;
}
```
- Clears signal value when no peak is being tracked
- Ensures clean baseline between peak events

### 10. Memory Update
```cpp
*_mem = *_volt_corr;
```
- Updates memory with current corrected voltage
- Used for next cycle's derivative calculation

## Sensor-Specific Applications

### Electrochemical Sensors (H2S, O3, SO2, NO2)
```cpp
peakDetect("h2s", &_h2s_corr, &_h2s_volt_max, &_h2s_signal, &_h2s_base, 
           &_h2s_top, &_h2s_mem, &_h2s_tcomp, &_h2s_dx, &_h2s_dx_trig, &_h2s_found);
```

### Infrared Sensors (CO2, C1)
```cpp
peakDetect("irco2", &_co2_corr, &_co2_volt_max, &_co2_signal, &_co2_base, 
           &_co2_top, &_co2_mem, &_co2_tcomp, &_co2_dx, &_co2_dx_trig, &_co2_found);
```

### VOC Sensors
```cpp
peakDetect("voc", &_voc_corr, &_voc_raw_max, &_voc_signal, &_voc_base, 
           &_voc_top, &_voc_mem, &S1, &_voc_dx, &_voc_dx_trig, &_voc_found);
```

### NH3 Sensors (Weather Shield v14)
```cpp
peakDetect("nh3", &_nh3_corr, &_nh3_volt_max, &_nh3_signal, &_nh3_base, 
           &_nh3_top, &_nh3_mem, &_nh3_tcomp, &_nh3_dx, &_nh3_dx_trig, &_nh3_found);
```

## Trigger Threshold Configuration

### EEPROM Storage
Trigger thresholds (`_dx_trig`) are stored in EEPROM along with other calibration parameters:
- **C1 sensor**: EEPROM page 1
- **CO2 sensor**: EEPROM page 2  
- **PID sensor**: EEPROM page 3
- **NH3 sensor**: EEPROM page 4
- **H2S sensor**: EEPROM page 5
- **NO2 sensor**: EEPROM page 6
- **SO2 sensor**: EEPROM page 7
- **O3 sensor**: EEPROM page 8
- **VOC sensor**: EEPROM page 10

### Calibration Format
```cpp
sprintf(_EEpagebuffer, "H2S %u %f %f %f %f %f", 
        _h2s_enable, fRange, _h2s_gain, _h2s_offset_volt, _h2s_tcomp, _h2s_dx_trig);
```

## Environmental Reset Conditions

### Temperature and Humidity Changes
```cpp
// Commented out in current implementation:
// if ((abs(_temperature_dx) >= 1) || (abs(_humidity_dx) >= 1)) { 
//     *_found = false; 
// }
```
- Originally designed to reset peak detection on rapid environmental changes
- Currently disabled in the implementation
- Would reset peak detection if temperature or humidity changed by ≥1 unit

## Debug and Monitoring

### Serial Output Format
```cpp
Serial.printf("\tloop\tvmax\tcorr\tsignal\tbase\ttop\tmem\ttcomp\tdx\ttrig\tfound\r\n");
```

### VOC Sensor Output (Integer Format)
```cpp
Serial.printf("\t%lu\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%.4f\t%.0f\t%.0f\t%u\r\n", 
              DLoop, *_volt_max, *_volt_corr, *_signal, *_base, *_top, *_mem, 
              *_tcomp, *_dx, *_trig, *_found);
```

### Other Sensors Output (Float Format)
```cpp
Serial.printf("\t%lu\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%u\r\n", 
              DLoop, *_volt_max, *_volt_corr, *_signal, *_base, *_top, *_mem, 
              *_tcomp, *_dx, *_trig, *_found);
```

## Key Algorithm Characteristics

### Advantages
1. **Temperature Compensation**: Automatically corrects for temperature drift
2. **Adaptive Baseline**: Dynamically adjusts baseline to prevent false peaks
3. **Hysteresis**: 10% threshold prevents oscillation at peak end
4. **Derivative-Based**: Uses rate of change for robust peak detection
5. **Configurable Sensitivity**: Trigger thresholds stored in EEPROM

### Limitations
1. **Fixed Delay**: 4-cycle startup delay may miss early peaks
2. **Single Peak**: Cannot track multiple simultaneous peaks
3. **Linear Temperature Compensation**: May not suit all sensor types
4. **No Noise Filtering**: Susceptible to electrical noise spikes

### Performance Considerations
- Algorithm runs every measurement cycle after initial delay
- Minimal computational overhead
- Memory usage: ~10 float variables per sensor
- Real-time operation suitable for embedded systems