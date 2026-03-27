# Hardware Interfaces and I2C Communication Analysis

## I2C Device Mapping and Addressing

### TCA9548 I2C Multiplexer (0x70)
The system uses a TCA9548 I2C multiplexer to manage multiple I2C devices that may have address conflicts. The multiplexer is located at address **0x70** and provides 8 channels (0-7).

#### TCA Channel Assignments:
- **Channel 0**: Grove sensors (BME280/BME680 at 0x76)
- **Channel 1**: SPEC electrochemical sensors (LMP91000 at 0x6B)
- **Channel 2**: VOC sensors (SGP40 at 0x59, SGP30 at 0x58)
- **Channel 3**: Weather Shield onboard temperature sensor (BME280/BME680 at 0x76)
- **Channel 4**: Particulate Matter sensor (SPS30 at 0x69)
- **Channel 5**: MPS (Micro Particle Sensor) at 0x40
- **Channel 6**: Infrared sensors (ADC at 0x6C)
- **Channel 7**: NH3 sensor (ADC at 0x6A, disabled for WS version 14)

### Primary I2C Device Addresses

#### Core System Devices:
- **0x70**: TCA9548 I2C Multiplexer
- **0x50**: Weather Shield EEPROM (IC113)
- **0x52**: FeatherBase EEPROM (LTE module)
- **0x4B**: Battery pack monitoring (Grove 4-Channel 16-bit ADC ADS1115)

#### GPIO Expanders:
- **0x39**: PCF8574 FeatherBase IC8 (User interface, system control)
- **0x3A**: PCF8574 Weather Shield IC113 (IR power control, sampling pump)
- **0x3B**: PCF8574 Weather Shield IC102 (NH3 power control, SPEC sensor selection)
- **0x38**: PCF8574 FeatherBase IC7 (Wind speed sensor)

#### Sensor Devices (via TCA channels):
- **0x76**: BME280/BME680 Temperature, Humidity, Pressure sensors
- **0x6B**: LMP91000 Electrochemical sensor ADC (H2S, O3, SO2, NO2)
- **0x59**: SGP40 VOC sensor
- **0x58**: SGP30 VOC sensor (alternative to SGP40)
- **0x69**: SPS30 Particulate Matter sensor
- **0x40**: ATTiny MPS (Micro Particle Sensor)
- **0x6C**: Infrared sensor ADC (CO2, CH4, PID)
- **0x6A**: NH3 sensor ADC (disabled for WS version 14)

#### Wind Sensors:
- **0x15**: Calypso Ultrasonic Wind sensor (FeatherBase VF3)
- **0x38**: PCF8574 Wind speed sensor (FeatherBase VF7 IC7)

#### SUMMA Canister Control:
- **Configurable address**: PCF8574 for relay control (default varies by configuration)

### GPIO Pin Assignments

#### ESP32 Feather Pins:
- **Pin 13**: MODEM_PWR_ON_PIN (LTE modem power enable)
- **Pin 15**: TCA Reset (I2C multiplexer reset)
- **Pin 34**: WINDIR_PIN (Wind direction analog input A2_I34)
- **Pin 35**: BATTERY_PIN (Battery voltage monitoring, backup method)

#### PCF8574 GPIO Expander Pins:

**FeatherBase IC8 (0x39):**
- **P0**: User button input
- **P1**: User LED output (inverted logic)
- **P4**: PCF4 (general purpose)
- **P5**: ECOOL (cooling control)
- **P6**: EMODEM (modem enable, inverted logic)
- **P7**: RST TCA (TCA multiplexer reset)

**Weather Shield IC113 (0x3A) - Power Control:**
- **P0**: IR1 CO2 sensor power (U2 Ch4, active low)
- **P1**: IR2 CH4 sensor power (U3 Ch1, active low)
- **P2**: IR3 PID sensor power (U4 Ch3, active low)
- **P3**: MPS sensor power (IR4/MPS, active low)
- **P4**: Sampling pump control (active high)
- **P5**: PM sensor power control (inverted logic)
- **P6**: 3VP supply control (active low)

**Weather Shield IC102 (0x3B) - SPEC Control:**
- **P0**: H2S sensor enable (En_H2S, active low)
- **P1**: NO2 sensor enable (En_NO2, active low)
- **P2**: O3 sensor enable (En_O3, active low)
- **P3**: SO2 sensor enable (En_SO2, active low)
- **P4**: NH3 sensor power/enable (active low, WS version dependent)

**FeatherBase IC7 (0x38) - Wind Speed:**
- **P0-P7**: Wind speed pulse counting inputs (all configured as inputs)

## Power Management and Control Systems

### Power Sequencing

#### System Startup Sequence:
1. **Core Power**: ESP32 and basic I2C infrastructure
2. **GPIO Expanders**: Initialize PCF8574 devices for control
3. **3VP Supply**: Enable 3.3V power rail for sensors (`set3VP_ON()`)
4. **Sensor Power**: Individual sensor power control via GPIO expanders
5. **TCA Multiplexer**: Reset and configure I2C multiplexer
6. **Sensor Initialization**: Configure each sensor through appropriate TCA channels

#### Sensor Power Control Functions:

**Infrared Sensors:**
```cpp
void setIR_ON()   // Powers CO2, CH4, PID sensors sequentially
void setIR_OFF()  // Disables all IR sensors
```

**Electrochemical Sensors:**
```cpp
void setSPEC_ONLine(byte CSBit)  // Enable specific SPEC sensor (0-4)
void setSPEC_OFFLine()           // Disable all SPEC sensors
```

**Other Sensors:**
```cpp
void setMPS_ON() / setMPS_OFF()   // MPS sensor control
void setPM_ON() / setPM_OFF()     // PM sensor control  
void setNH3_ON() / setNH3_OFF()   // NH3 sensor control
void setSampling_ON() / setSampling_OFF()  // Sampling pump control
```

### Sleep Modes and Power Saving

#### Deep Sleep Implementation:
```cpp
void goSleep(String output, uint64_t sleepDelay)
```

**Sleep Preparation Sequence:**
1. Stop data collection (`pauseCollect()`)
2. Turn off sampling pump (`setSampling_OFF()`)
3. Put sensors in sleep mode:
   - `sleepPM()` - Particulate matter sensor
   - `sleepMPS()` - Micro particle sensor  
   - `sleepVOC()` - VOC sensor heater off
4. Disable GPS/GNSS (`modem_gnss_off()`)
5. Turn off all sensor power (`shieldOFF()`)
6. Configure ESP32 deep sleep timer
7. Enter deep sleep (`esp_deep_sleep_start()`)

#### Power Saving Features:
- **Sensor Sleep Modes**: Individual sensors can be put into low-power states
- **Sampling Pump Control**: Pump only runs during sampling periods
- **Modem Power Management**: LTE modem can be powered down between transmissions
- **VOC Sensor Optimization**: SGP40 heater control for power efficiency

### Battery Monitoring and Protection

#### Battery Voltage Monitoring:
- **Primary**: Grove 4-Channel ADC (ADS1115) at 0x4B
- **Backup**: ESP32 internal ADC on pin 35
- **Voltage Calculation**: `BATTERY_K = (2) * (3300.0) / 4096.0`

#### Low Battery Protection:
```cpp
bool checkBatLow()  // Returns true if battery below threshold
```

**Protection Actions:**
- **Voltage Check**: Continuous monitoring against `_batt_low_voltage` threshold
- **Sleep on Low Battery**: System enters 1-hour sleep if voltage < 3.4V
- **Graceful Shutdown**: Saves system state before sleep

#### Battery Status Variables:
- `_vbat`: Current battery voltage
- `_state_off_charge`: Charge state
- `_bat_change_rate`: Battery voltage change rate
- `_batt_low_voltage`: Configurable low voltage threshold

### System Protection and Recovery

#### Temperature Protection:
- **CPU Temperature Monitoring**: `_cpu_temp` variable tracking
- **High Temperature Protection**: System reboot if CPU > 70°C
- **Sensor Temperature Limits**: EC sensors protected at high temperatures

#### Error Handling and Recovery:
- **Sensor Error Counting**: `_err_loop` tracks consecutive sensor failures
- **Automatic Reboot**: System reboots after 10 consecutive sensor errors
- **Error Persistence**: `_ctxerr_count` limits consecutive reboots (max 3)
- **Watchdog Timer**: 90-second timeout with panic on failure

#### Power Control Error Recovery:
- **I2C Communication Failures**: Automatic retry and error reporting
- **TCA Multiplexer Reset**: Hardware reset capability via GPIO
- **Sensor Power Cycling**: Individual sensor reboot functions
- **Modem Power Management**: Complete power cycle capability

### Hardware Version Dependencies

#### Weather Shield Versions:
- **WS Version 14**: CAN-based sensors, different NH3 handling
- **Earlier Versions**: Standard I2C sensor configuration

#### FeatherBase Versions:
- **VF3**: Uses GPIO pin 15 for TCA reset
- **VF**: Uses PCF8574 pin P7 for TCA reset
- **Compatibility**: Code supports both versions with conditional compilation

This hardware interface analysis provides the foundation for understanding how the air monitoring system manages its complex sensor array and power requirements through I2C communication and GPIO control.