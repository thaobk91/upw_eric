
#ifndef __APP_CFG_H
#define __APP_CFG_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <esp_task_wdt.h>
#include <Wire.h>
#include "esp_pm.h"

#if CONFIG_USE_SERIAL_DEBUG == true
#define CONFIG_LOG_TAG(name)            static const char *const_TAG = #name ;
#define DBG_OUT(msg,...)			    CONFIG_DEBUG_PORT.printf("---> [%s-%d]: " msg, const_TAG, __LINE__, ##__VA_ARGS__); \
										fflush(stdout)
#define DBG_OUT_RAW						CONFIG_DEBUG_PORT.printf
#define DBG_OUT_E(msg,...)			    CONFIG_DEBUG_PORT.printf("\033[0;31m---> [%s-%d]: " msg "\033[0m", const_TAG, __LINE__, ##__VA_ARGS__); \
										fflush(stdout)
#define DBG_OUT_W(msg,...)              CONFIG_DEBUG_PORT.printf("\033[0;33m---> [%s-%d]: " msg "\033[0m", const_TAG, __LINE__, ##__VA_ARGS__); \
										fflush(stdout)
#define DBG_OUT_I(msg,...)              CONFIG_DEBUG_PORT.printf("\033[0;32m---> [%s-%d]: " msg "\033[0m", const_TAG, __LINE__, ##__VA_ARGS__); \
										fflush(stdout)
#define DBG_OUT_H(msg,...)              CONFIG_DEBUG_PORT.printf("\033[0;36m---> [%s-%d]: " msg "\033[0m", const_TAG, __LINE__, ##__VA_ARGS__); \
										fflush(stdout)
#define DBG_OUT_RAW_I(msg,...)          CONFIG_DEBUG_PORT.printf("\033[0;32m" msg "\033[0m", ##__VA_ARGS__); \
										fflush(stdout)
#else
#define CONFIG_LOG_TAG(name)
#define DBG_OUT(...)
#define DBG_OUT_RAW(...)
#define DBG_OUT_E(...)
#define DBG_OUT_W(...)
#define DBG_OUT_I(...)
#define DBG_OUT_H(...)
#define DBG_OUT_RAW_I(...)
#endif // #if CONFIG_USE_SERIAL_DEBUG == true

#define FUNC_GET_TICK_MS()              (esp_timer_get_time() / 1000) 	// int64_t
#define FUNC_GET_TICK_US()              esp_timer_get_time() 			// int64_t
#define FUNC_DELAY_MS(ms)               vTaskDelay(ms / portTICK_PERIOD_MS)

/* task handle */
typedef struct {
    TaskHandle_t handle;
    bool enabled;
} task_handle_t;

/* task index */
enum {
    ENUM_TASK_APP_INDEX = 0,
	ENUM_TASK_NET_INDEX,
#if CONFIG_CLI_ENABLED == 1
    ENUM_TASK_CLI_INDEX,
#endif // #if CONFIG_CLI_ENABLED == 1
    ENUM_TASK_MAX_INDEX,
};

/* Battery Charger */
typedef enum {
	ENUM_BATTERY_CHARGER_NOT_CHARGING = 0,
	ENUM_BATTERY_CHARGER_CHARGING,
	ENUM_BATTERY_CHARGER_FULL,
} battery_charger_e;

/* Location */
typedef struct {
	char device_id[32] = {0};
	char fw_version[16] = CONFIG_FW_VERSION;
	uint8_t debug_level = 1;

	struct {
		char date[16] = {0};
		char time[16] = {0};
		int quality = 0;
		float latitude = 0.0f;
		float longitude = 0.0f;
		float altitude = 0.0f;
		int speed = 0;
		int heading = 0;
		float hdop = 0.0f;
		uint8_t sats = 0;
		char label[64] = {0};
		char group[64] = {0};
	} location;

	char owner[32] = {0};
} device_cfg_t;

/* Hardware Configuration */
typedef struct {
	uint8_t weather_shield_version = 14; /* Hardware version (10=VF2, 11=VF3, 12=VG, 13=VH, 14=VH2 CAN) */
	uint32_t panel_watts = 0;
	struct {
		float low_power_voltage = 3.4f; /* Battery protection threshold (default: 3.4V) */
		float battery_ah = 30.0f; 		/* Battery capacity in amp-hours */
		uint32_t low_power_nap_s = 0; 	/* Sleep duration when battery is low (second) */
	} battery;

	struct {
		char comment[64] = {0};
		char cal_date_time[64] = {0};
		uint32_t cal_time_stamp = 0;
		float cal_temp = 20.0f; 		/* Calibration reference temperature (default: 20.0°C) */
		float temp_offset = -3.2f; 		/* Temperature calibration offset */
		float temp_dx = 0.0f;			/* Temperature differential calculation */
		bool pm_enable = false;			/* Enable/disable PM sensor */
		uint32_t pm25_restime_min = 0;	/* Rest time between measurements */
		bool voc_mox_enable = false;
		bool voc_enable = true;
		uint32_t voc_offset = 0;
		float voc_gain = 1.0f;
		uint32_t voc_trig = 60;
		float voc_tcomp = 0.0;
		float voc_exp_C = -3.183948105f;
		float voc_exp_b = 0.000618f;
		float voc_exp_A = 1.999186069f;
		uint32_t voc_lin_m = 0;
		uint32_t voc_lin_b = 0;
		bool wind_enable = true;
		uint32_t wind_interval_sec = 30;
		bool pid_enable = true;
		float pid_offset_volt = 0.06f;
		uint32_t pid_gain = 40;
		uint32_t pid_range_ppm = 40;
		float pid_trig = 0.0005f;
		float pid_tcomp = 0.0f;
		bool c1_enable = false;
		float c1_offset_volt = 0.1f;
		uint32_t c1_gain = 1;
		uint32_t c1_range_ppm = 50000;
		float c1_trig = 0.001f;
		float c1_tcomp = 0.0f;
		bool co2_enable = false;
		float co2_offset_volt = 0.1f;
		uint32_t co2_gain = 1;
		uint32_t co2_range_ppm = 300000;
		float co2_trig = 0.001f;
		float co2_tcomp = 0.0f;
		bool h2s_enable = false;
		float h2s_offset_volt = 0.410f;
		float h2s_gain = 39.3f;
		float h2s_trig = 0.0005f;
		float h2s_tcomp = 0.0f;
		bool o3_enable = false;
		float o3_offset_volt = 1.024f;
		float o3_gain = -33.4f;
		float o3_trig = 0.0005f;
		float o3_tcomp = 0.0;
		bool so2_enable = false;
		float so2_offset_volt = 1.024f;
		float so2_gain = 80.2f;
		float so2_trig = 0.0005f;
		float so2_tcomp = 0.0f;
		bool no2_enable = false;
		float no2_offset_volt = 1.024f;
		float no2_gain = -66.8f;
		float no2_trig = 0.0005f;
		float no2_tcomp = 0.0f;
		bool nh3_enable = false;
		float nh3_offset_volt = 1.024f;
		float nh3_gain = -29.26f;
		float nh3_trig = 0.0005f;
		float nh3_tcomp = 0.0f;
		uint32_t red_r0 = 100000;
		uint32_t ox_r0 = 800;
		uint32_t nh3_r0 = 10000;
		bool mps_enable = false;
		uint32_t pid_lin_b = 0;
		uint32_t pid_lin_m = 0;
	} sensor;
} hw_cfg_t;

/* Cloud */
typedef struct {
	struct {
		char endpoint[128] = {0};	/* MQTT broker hostname */
		uint16_t port = 8883;		/* MQTT broker port (typically 8883 for SSL) */
		char label[32] = {0};
		char *ca = NULL;			/* Certificate Authority certificate */		
		char *cc = NULL;			/* Client certificate */
		char *ck = NULL;			/* Client private key */
	} mqtt;

	struct {
		char endpoint[128] = {0};
		bool automatic_updates = false;
		uint32_t check_interval_s = 24 * 60 * 60 * 1000; /* second */
	} update;

	struct {
		char endpoint[128] = {0};
	} error_log;
} server_info_t;

/* Network */
typedef struct {
	uint32_t no_coms_nap = 0;

	struct {
		bool ap_always_on = false;
		char ap_password[64] = {0};
		bool enabled = false;
		bool antenna = true;
		char sta_ssid[64] = "TerraSLS";
		char sta_pwd[64] = "weslowifi";
	} wifi;

	struct {
		char chip[64] = "Quectel BG95-M3 Revision: BG95M1LAR02A04";
		char carrier[32] = "Teal";
		char apn[32] = "teal";
		bool enabled = true;
		uint32_t attach_timeout_s = 180; /* Second */
		char imei[32] = {0};
		char imsi[32] = {0};
		char iccid[32] = {0};
		int rssi = 0;
		bool gps_enabled = false;
		bool lock_gps = false;
	} lte;

	bool offline_mode = false;
} network_info_t;

typedef struct {
	uint32_t pumping_time_s = 0;		/* Sampling pump duration (0 = always on) */
	uint32_t sampling_interval_sec = 2; /* Time between sensor readings (minimum 2 seconds) */
	uint32_t report_interval_count = 15;/* Number of samples before reporting */
	uint32_t sleep_time_sec = 0;		/* Deep sleep duration between reports, second */
	bool engineering = false;			/* Enable engineering data format */
} sampling_cfg_t;

typedef struct {
    char wifiMac_str[32] = {0};

	battery_charger_e battery_charger = ENUM_BATTERY_CHARGER_NOT_CHARGING;
	uint16_t battery_mV = 0;
    uint8_t battery_percent = 0;

	device_cfg_t device_cfg;
	hw_cfg_t hw_cfg;
	server_info_t server_info;
	network_info_t network_info;
	sampling_cfg_t sampling_cfg;

} device_data_t;

extern device_data_t g_device_data;
extern task_handle_t g_task_handle[ENUM_TASK_MAX_INDEX];

#endif // __APP_CFG_H