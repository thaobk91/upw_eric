
#include "config_parse.h"
#include "common/core_json.h"
#include "common/dev_mem.h"
#include "common/appUtils.h"

CONFIG_LOG_TAG(CONFIG_PARSE)

#define CONFIG_DATA_JSON_FIELD_NUM			138
static const char *g_data_json_field[CONFIG_DATA_JSON_FIELD_NUM] = {
	"device",
		"device.id",
		"device.firmware",
		"device.verbose_level",
		"device.location",
			"device.location.date",
			"device.location.time",
			"device.location.quality",
			"device.location.latitude",
			"device.location.longitude",
			"device.location.altitude",
			"device.location.speed",
			"device.location.heading",
			"device.location.HDOP",
			"device.location.Sats",
			"device.location.label",
			"device.location.group",
		"device.owner",
	"hardware",
		"hardware.weather_shield_version",
		"hardware.panel_watts",
		"hardware.battery",
			"hardware.battery.low_power_voltage",
			"hardware.battery.battery_ah",
			"hardware.battery.low_power_nap",
		"hardware.sensors",
			"hardware.sensors.comment",
			"hardware.sensors.cal_date_time",
			"hardware.sensors.cal_time_stamp",
			"hardware.sensors.cal_temp",
			"hardware.sensors.temp_offset",
			"hardware.sensors.temp_dx",
			"hardware.sensors.pm_enable",
			"hardware.sensors.pm25_restime_min",
			"hardware.sensors.voc_mox_enable",
			"hardware.sensors.voc_enable",
			"hardware.sensors.voc_offset",
			"hardware.sensors.voc_gain",
			"hardware.sensors.voc_trig",
			"hardware.sensors.voc_tcomp",
			"hardware.sensors.voc_exp_C",
			"hardware.sensors.voc_exp_b",
			"hardware.sensors.voc_exp_A",
			"hardware.sensors.voc_lin_m",
			"hardware.sensors.voc_lin_b",
			"hardware.sensors.wind_enable",
			"hardware.sensors.wind_interval_sec",
			"hardware.sensors.pid_enable",
			"hardware.sensors.pid_offset_volt",
			"hardware.sensors.pid_gain",
			"hardware.sensors.pid_range_ppm",
			"hardware.sensors.pid_trig",
			"hardware.sensors.pid_tcomp",
			"hardware.sensors.c1_enable",
			"hardware.sensors.c1_offset_volt",
			"hardware.sensors.c1_gain",
			"hardware.sensors.c1_range_ppm",
			"hardware.sensors.c1_trig",
			"hardware.sensors.c1_tcomp",
			"hardware.sensors.co2_enable",
			"hardware.sensors.co2_offset_volt",
			"hardware.sensors.co2_gain",
			"hardware.sensors.co2_range_ppm",
			"hardware.sensors.co2_trig",
			"hardware.sensors.co2_tcomp",
			"hardware.sensors.h2s_enable",
			"hardware.sensors.h2s_offset_volt",
			"hardware.sensors.h2s_gain",
			"hardware.sensors.h2s_trig",
			"hardware.sensors.h2s_tcomp",
			"hardware.sensors.o3_enable",
			"hardware.sensors.o3_offset_volt",
			"hardware.sensors.o3_gain",
			"hardware.sensors.o3_trig",
			"hardware.sensors.o3_tcomp",
			"hardware.sensors.so2_enable",
			"hardware.sensors.so2_offset_volt",
			"hardware.sensors.so2_gain",
			"hardware.sensors.so2_trig",
			"hardware.sensors.so2_tcomp",
			"hardware.sensors.no2_enable",
			"hardware.sensors.no2_offset_volt",
			"hardware.sensors.no2_gain",
			"hardware.sensors.no2_trig",
			"hardware.sensors.no2_tcomp",
			"hardware.sensors.nh3_enable",
			"hardware.sensors.nh3_offset_volt",
			"hardware.sensors.nh3_gain",
			"hardware.sensors.nh3_trig",
			"hardware.sensors.nh3_tcomp",
			"hardware.sensors.red_r0",
			"hardware.sensors.ox_r0",
			"hardware.sensors.nh3_r0",
			"hardware.sensors.mps_enable",
			"hardware.sensors.pid_lin_b",
			"hardware.sensors.pid_lin_m",
	"server",
		"server.mqtt",
			"server.mqtt.host",
			"server.mqtt.port",
			"server.mqtt.ssl",
			"server.mqtt.ssl.label",
			"server.mqtt.ssl.ca",
			"server.mqtt.ssl.cert",
			"server.mqtt.ssl.key",
		"server.update",
			"server.update.endpoint",
			"server.update.automatic_updates",
			"server.update.check_interval",
		"server.error_log",
			"server.error_log.endpoint",
	"network",
		"network.no_coms_nap",
		"network.wifi",
			"network.wifi.ap_always_on",
			"network.wifi.ap_password",
			"network.wifi.enabled",
			"network.wifi.antenna",
			"network.wifi.ssid",
			"network.wifi.password"
		"network.lte",
			"network.lte.chip",
			"network.lte.carrier",
			"network.lte.apn",
			"network.lte.enabled",
			"network.lte.attach_timeout",
			"network.lte.IMEI",
			"network.lte.IMSI",
			"network.lte.CCID",
			"network.lte.RSSI",
			"network.lte.gps_enabled",
			"network.lte.lock_gps",
		"network.offline_mode",
	"sampling",
		"sampling.pumping_time",
		"sampling.sampling_interval_sec",
		"sampling.report_interval_count",
		"sampling.sleep_time_sec",
		"sampling.engineering"
};


/*** STATIC FUNCTION ***/
static int config_parse_get_field_index(char *field_name);
static void config_parse_get_string(char *data, uint8_t data_size, const char *field_name,
	const char *json_data[CONFIG_DATA_JSON_FIELD_NUM], size_t json_data_len[CONFIG_DATA_JSON_FIELD_NUM]);
static void config_parse_get_string_allocate(char **data, const char *field_name,
	const char *json_data[CONFIG_DATA_JSON_FIELD_NUM], size_t json_data_len[CONFIG_DATA_JSON_FIELD_NUM]);
static void config_parse_get_int(int *data,
	const char *field_name, const char *json_data[CONFIG_DATA_JSON_FIELD_NUM]);
static void config_parse_get_u8(uint8_t *data,
	const char *field_name, const char *json_data[CONFIG_DATA_JSON_FIELD_NUM]);
static void config_parse_get_u16(uint16_t *data,
	const char *field_name, const char *json_data[CONFIG_DATA_JSON_FIELD_NUM]);
static void config_parse_get_u32(uint32_t *data,
	const char *field_name, const char *json_data[CONFIG_DATA_JSON_FIELD_NUM]);
static void config_parse_get_float(float *data,
	const char *field_name, const char *json_data[CONFIG_DATA_JSON_FIELD_NUM]);
static void config_parse_get_bool(bool *data,
	const char *field_name, const char *json_data[CONFIG_DATA_JSON_FIELD_NUM]);
static void config_parse_get_time_second(uint32_t *out_second, const char *field_name,
	const char *json_data[CONFIG_DATA_JSON_FIELD_NUM], size_t json_data_len[CONFIG_DATA_JSON_FIELD_NUM]);


/*** PUBLIC FUNCTION ***/

/**
 * @brief parse configuration from JSON data
 * 
 * @param data json data
 * @param data_len length of data
 * 
 * @retval true if success
 * @retval false if false
 */
bool config_parse_json_file(char *data, uint16_t data_len)
{
	if (JSON_Validate((const char *)data, data_len) != JSONSuccess) {
		DBG_OUT_E("Json not found\r\n");
		return false;
	}

	const char *out[CONFIG_DATA_JSON_FIELD_NUM] = {NULL};
	size_t out_len[CONFIG_DATA_JSON_FIELD_NUM] = {0};
	for (int i = 0; i < CONFIG_DATA_JSON_FIELD_NUM; i++) {
		JSON_SearchConst((const char *)data, data_len,
				g_data_json_field[i], strlen(g_data_json_field[i]), &out[i], &out_len[i], NULL);
	}

	int field_index = 0;

	///////// DEVICE /////////
	/* Device ID */
	config_parse_get_string(g_device_data.device_cfg.device_id,
			sizeof(g_device_data.device_cfg.device_id), "device.id", out, out_len);
	/* Firmware */
	config_parse_get_string(g_device_data.device_cfg.fw_version,
			sizeof(g_device_data.device_cfg.fw_version), "device.firmware", out, out_len);
	/* Log level */
	config_parse_get_int((int *)&g_device_data.device_cfg.debug_level, "device.verbose_level", out);
	/* Location date */
	config_parse_get_string(g_device_data.device_cfg.location.date,
			sizeof(g_device_data.device_cfg.location.date), "device.location.date", out, out_len);
	/* Location time */
	config_parse_get_string(g_device_data.device_cfg.location.time,
			sizeof(g_device_data.device_cfg.location.time), "device.location.time", out, out_len);
	/* Location Quality */
	config_parse_get_int(&g_device_data.device_cfg.location.quality, "device.location.quality", out);
	/* Location Latitude */
	config_parse_get_float(&g_device_data.device_cfg.location.latitude, "device.location.latitude", out);
	/* Location Longitude */
	config_parse_get_float(&g_device_data.device_cfg.location.longitude, "device.location.longitude", out);
	/* Location altitude */
	config_parse_get_float(&g_device_data.device_cfg.location.altitude, "device.location.altitude", out);
	/* Location speed */
	config_parse_get_int(&g_device_data.device_cfg.location.speed, "device.location.speed", out);
	/* Location heading */
	config_parse_get_int(&g_device_data.device_cfg.location.heading, "device.location.heading", out);
	/* Location HDOP */
	config_parse_get_float(&g_device_data.device_cfg.location.hdop, "device.location.HDOP", out);
	/* Location Sats */
	config_parse_get_u8(&g_device_data.device_cfg.location.sats, "device.location.Sats", out);
	/* Location Label */
	config_parse_get_string(g_device_data.device_cfg.location.label,
			sizeof(g_device_data.device_cfg.location.label), "device.location.label", out, out_len);
	/* Location group */
	config_parse_get_string(g_device_data.device_cfg.location.group,
			sizeof(g_device_data.device_cfg.location.group), "device.location.group", out, out_len);
	/* Device owner */
	config_parse_get_string(g_device_data.device_cfg.owner,
			sizeof(g_device_data.device_cfg.owner), "device.owner", out, out_len);

	///////// HARDWARE /////////
	/* Hardware weather_shield_version */
	config_parse_get_u8(&g_device_data.hw_cfg.weather_shield_version, "hardware.weather_shield_version", out);
	/* Hardware panel_watts */
	config_parse_get_u32(&g_device_data.hw_cfg.panel_watts, "hardware.panel_watts", out);
	/* Hardware Battery low_power_voltage */
	config_parse_get_float(&g_device_data.hw_cfg.battery.low_power_voltage, "hardware.battery.low_power_voltage", out);
	/* Hardware Battery battery_ah */
	config_parse_get_float(&g_device_data.hw_cfg.battery.battery_ah, "hardware.battery.battery_ah", out);
	/* Hardware low_power_nap */
	config_parse_get_u32(&g_device_data.hw_cfg.battery.low_power_nap_s, "hardware.battery.low_power_nap", out);
	/* Hardware Sensor Comment */
	config_parse_get_string(g_device_data.hw_cfg.sensor.comment,
			sizeof(g_device_data.hw_cfg.sensor.comment), "hardware.sensors.comment", out, out_len);
	/* Hardware Sensor cal_date_time */
	config_parse_get_string(g_device_data.hw_cfg.sensor.cal_date_time,
			sizeof(g_device_data.hw_cfg.sensor.cal_date_time), "hardware.sensors.cal_date_time", out, out_len);
	/* Hardware Sensor cal_time_stamp */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.cal_time_stamp, "hardware.sensors.cal_time_stamp", out);
	/* Hardware Sensor cal_temp */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.cal_temp, "hardware.sensors.cal_temp", out);
	/* Hardware Sensor temp_offset */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.temp_offset, "hardware.sensors.temp_offset", out);
	/* Hardware Sensor temp_dx */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.temp_dx, "hardware.sensors.temp_dx", out);
	/* Hardware Sensor pm_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.pm_enable, "hardware.sensors.pm_enable", out);
	/* Hardware Sensor pm25_restime_min */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.pm25_restime_min, "hardware.sensors.pm25_restime_min", out);
	/* Hardware Sensor voc_mox_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.voc_mox_enable, "hardware.sensors.voc_mox_enable", out);
	/* Hardware Sensor voc_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.voc_enable, "hardware.sensors.voc_enable", out);
	/* Hardware Sensor voc_offset */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.voc_offset, "hardware.sensors.voc_offset", out);
	/* Hardware Sensor voc_gain */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.voc_gain, "hardware.sensors.voc_gain", out);
	/* Hardware Sensor voc_trig */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.voc_trig, "hardware.sensors.voc_trig", out);
	/* Hardware Sensor voc_tcomp */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.voc_tcomp, "hardware.sensors.voc_tcomp", out);
	/* Hardware Sensor voc_exp_C */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.voc_exp_C, "hardware.sensors.voc_exp_C", out);
	/* Hardware Sensor voc_exp_b */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.voc_exp_b, "hardware.sensors.voc_exp_b", out);
	/* Hardware Sensor voc_exp_A */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.voc_exp_A, "hardware.sensors.voc_exp_A", out);
	/* Hardware Sensor voc_lin_m */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.voc_lin_m, "hardware.sensors.voc_lin_m", out);
	/* Hardware Sensor voc_lin_b */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.voc_lin_b, "hardware.sensors.voc_lin_b", out);
	/* Hardware Sensor wind_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.wind_enable, "hardware.sensors.wind_enable", out);
	/* Hardware Sensor wind_interval_sec */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.wind_interval_sec, "hardware.sensors.wind_interval_sec", out);
	/* Hardware Sensor pid_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.pid_enable, "hardware.sensors.pid_enable", out);
	/* Hardware Sensor pid_offset_volt */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.pid_offset_volt, "hardware.sensors.pid_offset_volt", out);
	/* Hardware Sensor pid_gain */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.pid_gain, "hardware.sensors.pid_gain", out);
	/* Hardware Sensor pid_range_ppm */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.pid_range_ppm, "hardware.sensors.pid_range_ppm", out);
	/* Hardware Sensor pid_trig */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.pid_trig, "hardware.sensors.pid_trig", out);
	/* Hardware Sensor pid_tcomp */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.pid_tcomp, "hardware.sensors.pid_tcomp", out);
	/* Hardware Sensor c1_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.c1_enable, "hardware.sensors.c1_enable", out);
	/* Hardware Sensor c1_offset_volt */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.c1_offset_volt, "hardware.sensors.c1_offset_volt", out);
	/* Hardware Sensor c1_gain */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.c1_gain, "hardware.sensors.c1_gain", out);
	/* Hardware Sensor c1_range_ppm */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.c1_range_ppm, "hardware.sensors.c1_range_ppm", out);
	/* Hardware Sensor c1_trig */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.c1_trig, "hardware.sensors.c1_trig", out);
	/* Hardware Sensor c1_tcomp */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.c1_tcomp, "hardware.sensors.c1_tcomp", out);
	/* Hardware Sensor co2_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.co2_enable, "hardware.sensors.co2_enable", out);
	/* Hardware Sensor co2_offset_volt */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.co2_offset_volt, "hardware.sensors.co2_offset_volt", out);
	/* Hardware Sensor co2_gain */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.co2_gain, "hardware.sensors.co2_gain", out);
	/* Hardware Sensor co2_range_ppm */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.co2_range_ppm, "hardware.sensors.co2_range_ppm", out);
	/* Hardware Sensor co2_trig */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.co2_trig, "hardware.sensors.co2_trig", out);
	/* Hardware Sensor co2_tcomp */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.co2_tcomp, "hardware.sensors.co2_tcomp", out);
	/* Hardware Sensor h2s_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.h2s_enable, "hardware.sensors.h2s_enable", out);
	/* Hardware Sensor h2s_offset_volt */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.h2s_offset_volt, "hardware.sensors.h2s_offset_volt", out);
	/* Hardware Sensor h2s_gain */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.h2s_gain, "hardware.sensors.h2s_gain", out);
	/* Hardware Sensor h2s_trig */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.h2s_trig, "hardware.sensors.h2s_trig", out);
	/* Hardware Sensor h2s_tcomp */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.h2s_tcomp, "hardware.sensors.h2s_tcomp", out);
	/* Hardware Sensor o3_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.o3_enable, "hardware.sensors.o3_enable", out);
	/* Hardware Sensor o3_offset_volt */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.o3_offset_volt, "hardware.sensors.o3_offset_volt", out);
	/* Hardware Sensor o3_gain */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.o3_gain, "hardware.sensors.o3_gain", out);
	/* Hardware Sensor o3_trig */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.o3_trig, "hardware.sensors.o3_trig", out);
	/* Hardware Sensor o3_tcomp */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.o3_tcomp, "hardware.sensors.o3_tcomp", out);
	/* Hardware Sensor so2_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.so2_enable, "hardware.sensors.so2_enable", out);
	/* Hardware Sensor so2_offset_volt */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.so2_offset_volt, "hardware.sensors.so2_offset_volt", out);
	/* Hardware Sensor so2_gain */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.so2_gain, "hardware.sensors.so2_gain", out);
	/* Hardware Sensor so2_trig */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.so2_trig, "hardware.sensors.so2_trig", out);
	/* Hardware Sensor so2_tcomp */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.so2_tcomp, "hardware.sensors.so2_tcomp", out);
	/* Hardware Sensor no2_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.no2_enable, "hardware.sensors.no2_enable", out);
	/* Hardware Sensor no2_offset_volt */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.no2_offset_volt, "hardware.sensors.no2_offset_volt", out);
	/* Hardware Sensor no2_gain */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.no2_gain, "hardware.sensors.no2_gain", out);
	/* Hardware Sensor no2_trig */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.no2_trig, "hardware.sensors.no2_trig", out);
	/* Hardware Sensor no2_tcomp */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.no2_tcomp, "hardware.sensors.no2_tcomp", out);
	/* Hardware Sensor nh3_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.nh3_enable, "hardware.sensors.nh3_enable", out);
	/* Hardware Sensor nh3_offset_volt */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.nh3_offset_volt, "hardware.sensors.nh3_offset_volt", out);
	/* Hardware Sensor nh3_gain */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.nh3_gain, "hardware.sensors.nh3_gain", out);
	/* Hardware Sensor nh3_trig */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.nh3_trig, "hardware.sensors.nh3_trig", out);
	/* Hardware Sensor nh3_tcomp */
	config_parse_get_float(&g_device_data.hw_cfg.sensor.nh3_tcomp, "hardware.sensors.nh3_tcomp", out);
	/* Hardware Sensor red_r0 */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.red_r0, "hardware.sensors.red_r0", out);
	/* Hardware Sensor ox_r0 */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.ox_r0, "hardware.sensors.ox_r0", out);
	/* Hardware Sensor nh3_r0 */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.nh3_r0, "hardware.sensors.nh3_r0", out);
	/* Hardware Sensor mps_enable */
	config_parse_get_bool(&g_device_data.hw_cfg.sensor.mps_enable, "hardware.sensors.mps_enable", out);
	/* Hardware Sensor pid_lin_b */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.pid_lin_b, "hardware.sensors.pid_lin_b", out);
	/* Hardware Sensor pid_lin_m */
	config_parse_get_u32(&g_device_data.hw_cfg.sensor.pid_lin_m, "hardware.sensors.pid_lin_m", out);

	///////// SERVER /////////
	/* Server MQTT Host */
	config_parse_get_string(g_device_data.server_info.mqtt.endpoint,
			sizeof(g_device_data.server_info.mqtt.endpoint), "server.mqtt.host", out, out_len);
	/* Server MQTT Port */
	config_parse_get_u16(&g_device_data.server_info.mqtt.port, "server.mqtt.port", out);
	/* Server MQTT label */
	config_parse_get_string(g_device_data.server_info.mqtt.label,
			sizeof(g_device_data.server_info.mqtt.label), "server.mqtt.ssl.label", out, out_len);
	/* Server MQTT ca */
	config_parse_get_string_allocate(&g_device_data.server_info.mqtt.ca, "server.mqtt.ssl.ca", out, out_len);
	/* Server MQTT cert */
	config_parse_get_string_allocate(&g_device_data.server_info.mqtt.cc, "server.mqtt.ssl.cert", out, out_len);
	/* Server MQTT key */
	config_parse_get_string_allocate(&g_device_data.server_info.mqtt.ck, "server.mqtt.ssl.key", out, out_len);
	/* Server Update endpoint */
	config_parse_get_string(g_device_data.server_info.update.endpoint,
			sizeof(g_device_data.server_info.update.endpoint), "server.update.endpoint", out, out_len);
	/* Server Update endpoint */
	config_parse_get_string(g_device_data.server_info.update.endpoint,
			sizeof(g_device_data.server_info.update.endpoint), "server.update.endpoint", out, out_len);
	/* Server Update automatic_updates */
	config_parse_get_bool(&g_device_data.server_info.update.automatic_updates, "server.update.automatic_updates", out);
	/* Server Update check_interval */
	config_parse_get_time_second(&g_device_data.server_info.update.check_interval_s,
			"server.update.check_interval", out, out_len);
	/* Server error_log */
	config_parse_get_string(g_device_data.server_info.error_log.endpoint,
			sizeof(g_device_data.server_info.error_log.endpoint), "server.error_log.endpoint", out, out_len);

	///////// NETWORK /////////
	/* Network no_coms_nap */
	config_parse_get_time_second(&g_device_data.network_info.no_coms_nap, "network.no_coms_nap", out, out_len);
	/* Network wifi ap_always_on */
	config_parse_get_bool(&g_device_data.network_info.wifi.ap_always_on, "network.wifi.ap_always_on", out);
	/* Network wifi ap_password */
	config_parse_get_string(g_device_data.network_info.wifi.ap_password,
			sizeof(g_device_data.network_info.wifi.ap_password), "network.wifi.ap_password", out, out_len);
	/* Network wifi enabled */
	config_parse_get_bool(&g_device_data.network_info.wifi.enabled, "network.wifi.enabled", out);
	/* Network wifi antenna */
	config_parse_get_bool(&g_device_data.network_info.wifi.antenna, "network.wifi.antenna", out);
	/* Network wifi ssid */
	config_parse_get_string(g_device_data.network_info.wifi.sta_ssid,
			sizeof(g_device_data.network_info.wifi.sta_ssid), "network.wifi.ssid", out, out_len);
	/* Network wifi password */
	config_parse_get_string(g_device_data.network_info.wifi.sta_pwd,
			sizeof(g_device_data.network_info.wifi.sta_pwd), "network.wifi.password", out, out_len);

	///////// LTE /////////
	/* Network lte chip */
	config_parse_get_string(g_device_data.network_info.lte.chip,
			sizeof(g_device_data.network_info.lte.chip), "network.lte.chip", out, out_len);
	/* Network lte carrier */
	config_parse_get_string(g_device_data.network_info.lte.carrier,
			sizeof(g_device_data.network_info.lte.carrier), "network.lte.carrier", out, out_len);
	/* Network lte apn */
	config_parse_get_string(g_device_data.network_info.lte.apn,
			sizeof(g_device_data.network_info.lte.apn), "network.lte.apn", out, out_len);
	/* Network lte enabled */
	config_parse_get_bool(&g_device_data.network_info.lte.enabled, "network.lte.enabled", out);
	/* Network lte attach_timeout */
	config_parse_get_time_second(&g_device_data.network_info.lte.attach_timeout_s, "network.lte.attach_timeout", out, out_len);
	/* Network lte IMEI */
	config_parse_get_string(g_device_data.network_info.lte.imei,
			sizeof(g_device_data.network_info.lte.imei), "network.lte.IMEI", out, out_len);
	/* Network lte IMSI */
	config_parse_get_string(g_device_data.network_info.lte.imsi,
			sizeof(g_device_data.network_info.lte.imsi), "network.lte.IMSI", out, out_len);
	/* Network lte CCID */
	config_parse_get_string(g_device_data.network_info.lte.iccid,
			sizeof(g_device_data.network_info.lte.iccid), "network.lte.CCID", out, out_len);
	/* Network lte RSSI */
	config_parse_get_int(&g_device_data.network_info.lte.rssi, "network.lte.RSSI", out);
	/* Network lte gps_enabled */
	config_parse_get_bool(&g_device_data.network_info.lte.gps_enabled, "network.lte.gps_enabled", out);
	/* Network lte lock_gps */
	config_parse_get_bool(&g_device_data.network_info.lte.lock_gps, "network.lte.lock_gps", out);
	/* Network offline_mode */
	config_parse_get_bool(&g_device_data.network_info.offline_mode, "network.offline_mode", out);

	///////// SAMPLING /////////
	/* Sampling pumping_time */
	config_parse_get_time_second(&g_device_data.sampling_cfg.pumping_time_s, "sampling.pumping_time", out, out_len);
	/* Sampling sampling_interval_sec */
	config_parse_get_time_second(&g_device_data.sampling_cfg.sampling_interval_sec, "sampling.sampling_interval_sec", out, out_len);
	/* Sampling report_interval_count */
	config_parse_get_u32(&g_device_data.sampling_cfg.report_interval_count, "sampling.report_interval_count", out);
	/* Sampling sleep_time_sec */
	config_parse_get_time_second(&g_device_data.sampling_cfg.sleep_time_sec, "sampling.sleep_time_sec", out, out_len);
	/* Sampling engineering */
	config_parse_get_bool(&g_device_data.sampling_cfg.engineering, "sampling.engineering", out);

	return true;
}


/**
 * @brief print all configurations
 * 
 * @param None
 * 
 * @return None
 */
void config_parse_print(void)
{
	DBG_OUT_I("DEVICE:\r\n");
	DBG_OUT_RAW("     ID           : %s\r\n", g_device_data.device_cfg.device_id);
	DBG_OUT_RAW("     FW           : %s\r\n", g_device_data.device_cfg.fw_version);
	DBG_OUT_RAW("     Verbose Level: %d\r\n", g_device_data.device_cfg.debug_level);
	DBG_OUT_RAW("     Location     : [%s-%s], %d, [%.6f, %.6f, %.6f], %d, %d, %.2f, %d, %s, %s\r\n",
			g_device_data.device_cfg.location.date, g_device_data.device_cfg.location.time,
			g_device_data.device_cfg.location.quality, g_device_data.device_cfg.location.latitude,
			g_device_data.device_cfg.location.longitude, g_device_data.device_cfg.location.altitude,
			g_device_data.device_cfg.location.speed, g_device_data.device_cfg.location.heading,
			g_device_data.device_cfg.location.hdop, g_device_data.device_cfg.location.sats,
			g_device_data.device_cfg.location.label, g_device_data.device_cfg.location.group
			);
	DBG_OUT_RAW("     Owner        : %s\r\n", g_device_data.device_cfg.owner);

	DBG_OUT_I("HARDWARE:\r\n");
	DBG_OUT_RAW("     Weather Shield Version : %d\r\n", g_device_data.hw_cfg.weather_shield_version);
	DBG_OUT_RAW("     Panel Watts            : %s [W]\r\n", g_device_data.hw_cfg.panel_watts);
	DBG_OUT_RAW("     BATT: Low Power voltage: %.2f [V]\r\n", g_device_data.hw_cfg.battery.low_power_voltage);
	DBG_OUT_RAW("     BATT: Ah               : %.2f [Ah]\r\n", g_device_data.hw_cfg.battery.battery_ah);
	DBG_OUT_RAW("     BATT: Low Power NAP    : %d [s]\r\n", g_device_data.hw_cfg.battery.low_power_nap_s);
	DBG_OUT_RAW("     SS: Comment            : %s\r\n", g_device_data.hw_cfg.sensor.comment);
	DBG_OUT_RAW("     SS: Cal Date Time      : %s\r\n", g_device_data.hw_cfg.sensor.cal_date_time);
	DBG_OUT_RAW("     SS: cal_time_stamp     : %d\r\n", g_device_data.hw_cfg.sensor.cal_time_stamp);
	DBG_OUT_RAW("     SS: cal_temp           : %.1f [C]\r\n", g_device_data.hw_cfg.sensor.cal_temp);
	DBG_OUT_RAW("     SS: temp_offset        : %.1f\r\n", g_device_data.hw_cfg.sensor.temp_offset);
	DBG_OUT_RAW("     SS: temp_dx            : %.1f\r\n", g_device_data.hw_cfg.sensor.temp_dx);
	DBG_OUT_RAW("     SS: pm_enable          : %s\r\n", (g_device_data.hw_cfg.sensor.pm_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: pm25_restime_min   : %d\r\n", g_device_data.hw_cfg.sensor.pm25_restime_min);
	DBG_OUT_RAW("     SS: voc_mox_enable     : %s\r\n", (g_device_data.hw_cfg.sensor.voc_mox_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: voc_enable         : %s\r\n", (g_device_data.hw_cfg.sensor.voc_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: voc_offset         : %d\r\n", g_device_data.hw_cfg.sensor.voc_offset);
	DBG_OUT_RAW("     SS: voc_gain           : %.1f\r\n", g_device_data.hw_cfg.sensor.voc_gain);
	DBG_OUT_RAW("     SS: voc_trig           : %d\r\n", g_device_data.hw_cfg.sensor.voc_trig);
	DBG_OUT_RAW("     SS: voc_tcomp          : %.1f\r\n", g_device_data.hw_cfg.sensor.voc_tcomp);
	DBG_OUT_RAW("     SS: voc_exp_C          : %f\r\n", g_device_data.hw_cfg.sensor.voc_exp_C);
	DBG_OUT_RAW("     SS: voc_exp_b          : %f\r\n", g_device_data.hw_cfg.sensor.voc_exp_b);
	DBG_OUT_RAW("     SS: voc_exp_A          : %f\r\n", g_device_data.hw_cfg.sensor.voc_exp_A);
	DBG_OUT_RAW("     SS: voc_lin_m          : %d\r\n", g_device_data.hw_cfg.sensor.voc_lin_m);
	DBG_OUT_RAW("     SS: voc_lin_b          : %d\r\n", g_device_data.hw_cfg.sensor.voc_lin_b);
	DBG_OUT_RAW("     SS: wind_enable        : %s\r\n", (g_device_data.hw_cfg.sensor.wind_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: wind_interval_sec  : %d [s]\r\n", g_device_data.hw_cfg.sensor.wind_interval_sec);
	DBG_OUT_RAW("     SS: pid_enable         : %s\r\n", (g_device_data.hw_cfg.sensor.pid_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: pid_offset_volt    : %.2f\r\n", g_device_data.hw_cfg.sensor.pid_offset_volt);
	DBG_OUT_RAW("     SS: pid_gain           : %d\r\n", g_device_data.hw_cfg.sensor.pid_gain);
	DBG_OUT_RAW("     SS: pid_range_ppm      : %d\r\n", g_device_data.hw_cfg.sensor.pid_range_ppm);
	DBG_OUT_RAW("     SS: pid_trig           : %f\r\n", g_device_data.hw_cfg.sensor.pid_trig);
	DBG_OUT_RAW("     SS: pid_tcomp          : %f\r\n", g_device_data.hw_cfg.sensor.pid_tcomp);
	DBG_OUT_RAW("     SS: c1_enable          : %s\r\n", (g_device_data.hw_cfg.sensor.c1_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: c1_offset_volt     : %f\r\n", g_device_data.hw_cfg.sensor.c1_offset_volt);
	DBG_OUT_RAW("     SS: c1_gain            : %d\r\n", g_device_data.hw_cfg.sensor.c1_gain);
	DBG_OUT_RAW("     SS: c1_range_ppm       : %d\r\n", g_device_data.hw_cfg.sensor.c1_range_ppm);
	DBG_OUT_RAW("     SS: c1_trig            : %f\r\n", g_device_data.hw_cfg.sensor.c1_trig);
	DBG_OUT_RAW("     SS: c1_tcomp           : %f\r\n", g_device_data.hw_cfg.sensor.c1_tcomp);
	DBG_OUT_RAW("     SS: co2_enable         : %s\r\n", (g_device_data.hw_cfg.sensor.co2_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: co2_offset_volt    : %f\r\n", g_device_data.hw_cfg.sensor.co2_offset_volt);
	DBG_OUT_RAW("     SS: co2_gain           : %d\r\n", g_device_data.hw_cfg.sensor.co2_gain);
	DBG_OUT_RAW("     SS: co2_range_ppm      : %d\r\n", g_device_data.hw_cfg.sensor.co2_range_ppm);
	DBG_OUT_RAW("     SS: co2_trig           : %f\r\n", g_device_data.hw_cfg.sensor.co2_trig);
	DBG_OUT_RAW("     SS: co2_tcomp          : %f\r\n", g_device_data.hw_cfg.sensor.co2_tcomp);
	DBG_OUT_RAW("     SS: h2s_enable         : %s\r\n", (g_device_data.hw_cfg.sensor.h2s_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: h2s_offset_volt    : %f\r\n", g_device_data.hw_cfg.sensor.h2s_offset_volt);
	DBG_OUT_RAW("     SS: h2s_gain           : %f\r\n", g_device_data.hw_cfg.sensor.h2s_gain);
	DBG_OUT_RAW("     SS: h2s_trig           : %f\r\n", g_device_data.hw_cfg.sensor.h2s_trig);
	DBG_OUT_RAW("     SS: h2s_tcomp          : %f\r\n", g_device_data.hw_cfg.sensor.h2s_tcomp);
	DBG_OUT_RAW("     SS: o3_enable          : %s\r\n", (g_device_data.hw_cfg.sensor.o3_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: o3_offset_volt     : %f\r\n", g_device_data.hw_cfg.sensor.o3_offset_volt);
	DBG_OUT_RAW("     SS: o3_gain            : %f\r\n", g_device_data.hw_cfg.sensor.o3_gain);
	DBG_OUT_RAW("     SS: o3_trig            : %f\r\n", g_device_data.hw_cfg.sensor.o3_trig);
	DBG_OUT_RAW("     SS: o3_tcomp           : %f\r\n", g_device_data.hw_cfg.sensor.o3_tcomp);
	DBG_OUT_RAW("     SS: so2_enable         : %s\r\n", (g_device_data.hw_cfg.sensor.so2_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: so2_offset_volt    : %f\r\n", g_device_data.hw_cfg.sensor.so2_offset_volt);
	DBG_OUT_RAW("     SS: so2_gain           : %f\r\n", g_device_data.hw_cfg.sensor.so2_gain);
	DBG_OUT_RAW("     SS: so2_trig           : %f\r\n", g_device_data.hw_cfg.sensor.so2_trig);
	DBG_OUT_RAW("     SS: so2_tcomp          : %f\r\n", g_device_data.hw_cfg.sensor.so2_tcomp);
	DBG_OUT_RAW("     SS: no2_enable         : %s\r\n", (g_device_data.hw_cfg.sensor.no2_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: no2_offset_volt    : %f\r\n", g_device_data.hw_cfg.sensor.no2_offset_volt);
	DBG_OUT_RAW("     SS: no2_gain           : %f\r\n", g_device_data.hw_cfg.sensor.no2_gain);
	DBG_OUT_RAW("     SS: no2_trig           : %f\r\n", g_device_data.hw_cfg.sensor.no2_trig);
	DBG_OUT_RAW("     SS: no2_tcomp          : %f\r\n", g_device_data.hw_cfg.sensor.no2_tcomp);
	DBG_OUT_RAW("     SS: nh3_enable         : %s\r\n", (g_device_data.hw_cfg.sensor.nh3_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: nh3_offset_volt    : %f\r\n", g_device_data.hw_cfg.sensor.nh3_offset_volt);
	DBG_OUT_RAW("     SS: nh3_gain           : %f\r\n", g_device_data.hw_cfg.sensor.nh3_gain);
	DBG_OUT_RAW("     SS: nh3_trig           : %f\r\n", g_device_data.hw_cfg.sensor.nh3_trig);
	DBG_OUT_RAW("     SS: nh3_tcomp          : %f\r\n", g_device_data.hw_cfg.sensor.nh3_tcomp);
	DBG_OUT_RAW("     SS: red_r0             : %d\r\n", g_device_data.hw_cfg.sensor.red_r0);
	DBG_OUT_RAW("     SS: ox_r0              : %d\r\n", g_device_data.hw_cfg.sensor.ox_r0);
	DBG_OUT_RAW("     SS: nh3_r0             : %d\r\n", g_device_data.hw_cfg.sensor.nh3_r0);
	DBG_OUT_RAW("     SS: mps_enable         : %s\r\n", (g_device_data.hw_cfg.sensor.mps_enable == true) ? "true" : "false");
	DBG_OUT_RAW("     SS: pid_lin_b          : %d\r\n", g_device_data.hw_cfg.sensor.pid_lin_b);
	DBG_OUT_RAW("     SS: pid_lin_b          : %d\r\n", g_device_data.hw_cfg.sensor.pid_lin_b);

	DBG_OUT_I("SERVER:\r\n");
	DBG_OUT_RAW("     MQTT: host             : %s\r\n", g_device_data.server_info.mqtt.endpoint);
	DBG_OUT_RAW("     MQTT: port             : %d\r\n", g_device_data.server_info.mqtt.port);
	DBG_OUT_RAW("     MQTT: label            : %s\r\n", g_device_data.server_info.mqtt.label);
	DBG_OUT_RAW("     MQTT: ca               : %s\r\n", g_device_data.server_info.mqtt.ca);
	DBG_OUT_RAW("     MQTT: cc               : %s\r\n", g_device_data.server_info.mqtt.cc);
	DBG_OUT_RAW("     MQTT: ck               : %s\r\n", g_device_data.server_info.mqtt.ck);
	DBG_OUT_RAW("     UPDATE: endpoint       : %s\r\n", g_device_data.server_info.update.endpoint);
	DBG_OUT_RAW("     UPDATE: automatic_updates: %s\r\n", (g_device_data.server_info.update.automatic_updates == true) ? "true" : "false");
	DBG_OUT_RAW("     UPDATE: check_interval : %02d:%02d\r\n",
			g_device_data.server_info.update.check_interval_s / 3600,
			(g_device_data.server_info.update.check_interval_s % 3600) / 60);
	DBG_OUT_RAW("     ERROR LOG: endpoint    : %s\r\n", g_device_data.server_info.error_log.endpoint);

	DBG_OUT_I("NETWORK:\r\n");
	DBG_OUT_RAW("     no_coms_nap            : %02d:%02d\r\n",
			g_device_data.network_info.no_coms_nap / 3600,
			(g_device_data.network_info.no_coms_nap % 3600) / 60);
	DBG_OUT_RAW("     WIFI: ap_always_on     : %s\r\n", (g_device_data.network_info.wifi.ap_always_on == true) ? "true" : "false");
	DBG_OUT_RAW("     WIFI: ap_password      : %s\r\n", g_device_data.network_info.wifi.ap_password);
	DBG_OUT_RAW("     WIFI: enabled          : %s\r\n", (g_device_data.network_info.wifi.enabled == true) ? "true" : "false");
	DBG_OUT_RAW("     WIFI: antenna          : %s\r\n", (g_device_data.network_info.wifi.antenna == true) ? "true" : "false");
	DBG_OUT_RAW("     WIFI: ssid             : %s\r\n", g_device_data.network_info.wifi.sta_ssid);
	DBG_OUT_RAW("     WIFI: password         : %s\r\n", g_device_data.network_info.wifi.sta_pwd);
	DBG_OUT_RAW("     LTE: chip              : %s\r\n", g_device_data.network_info.lte.chip);
	DBG_OUT_RAW("     LTE: carrier           : %s\r\n", g_device_data.network_info.lte.carrier);
	DBG_OUT_RAW("     LTE: apn               : %s\r\n", g_device_data.network_info.lte.apn);
	DBG_OUT_RAW("     LTE: enabled           : %s\r\n", (g_device_data.network_info.lte.enabled == true) ? "true" : "false");
	DBG_OUT_RAW("     LTE: attach_timeout    : %02d:%02d\r\n",
			g_device_data.network_info.lte.attach_timeout_s / 3600,
			(g_device_data.network_info.lte.attach_timeout_s % 3600) / 60);
	DBG_OUT_RAW("     LTE: IMEI              : %s\r\n", g_device_data.network_info.lte.imei);
	DBG_OUT_RAW("     LTE: IMSI              : %s\r\n", g_device_data.network_info.lte.imsi);
	DBG_OUT_RAW("     LTE: CCID              : %s\r\n", g_device_data.network_info.lte.iccid);
	DBG_OUT_RAW("     LTE: RSSI              : %d\r\n", g_device_data.network_info.lte.rssi);
	DBG_OUT_RAW("     LTE: gps_enabled       : %s\r\n", (g_device_data.network_info.lte.gps_enabled == true) ? "true" : "false");
	DBG_OUT_RAW("     LTE: lock_gps          : %s\r\n", (g_device_data.network_info.lte.lock_gps == true) ? "true" : "false");
	DBG_OUT_RAW("     offline_mode           : %s\r\n", (g_device_data.network_info.offline_mode == true) ? "true" : "false");

	DBG_OUT_I("SAMPLING:\r\n");
	DBG_OUT_RAW("     pumping_time           : %d\r\n", g_device_data.sampling_cfg.pumping_time_s);
	DBG_OUT_RAW("     sampling_interval_sec  : %d\r\n", g_device_data.sampling_cfg.sampling_interval_sec);
	DBG_OUT_RAW("     report_interval_count  : %d\r\n", g_device_data.sampling_cfg.report_interval_count);
	DBG_OUT_RAW("     sleep_time_sec         : %d\r\n", g_device_data.sampling_cfg.sleep_time_sec);
	DBG_OUT_RAW("     engineering            : %s\r\n", (g_device_data.sampling_cfg.engineering == true) ? "true" : "false");
}


/*** STATIC FUNCTION ***/

static int config_parse_get_field_index(char *field_name)
{
	for (int i = 0; i < CONFIG_DATA_JSON_FIELD_NUM; i++) {
		if (strcmp(g_data_json_field[i], field_name) == 0) {
			return i;
		}
	}
	return -1;
}


static void config_parse_get_string(char *data, uint8_t data_size, const char *field_name,
	const char *json_data[CONFIG_DATA_JSON_FIELD_NUM], size_t json_data_len[CONFIG_DATA_JSON_FIELD_NUM])
{
	int index = config_parse_get_field_index((char *)field_name);
	if ((index >= 0) && (json_data[index] != NULL)) {
		memset(data, 0, data_size);
		int len = json_data_len[index];
		if (len >= data_size) {
			len = data_size - 1;
		}
		strncpy(data, json_data[index], len);
	}
}


static void config_parse_get_string_allocate(char **data, const char *field_name,
	const char *json_data[CONFIG_DATA_JSON_FIELD_NUM], size_t json_data_len[CONFIG_DATA_JSON_FIELD_NUM])
{
	int index = config_parse_get_field_index((char *)field_name);
	if ((index >= 0) && (json_data[index] != NULL)) {
		if (*data != NULL) {
			dev_mem_free((void **)data);
		}
		*data = (char *)dev_mem_malloc_selectRam(json_data_len[index] + 1);
		if (data != NULL) {
			strncpy(*data, json_data[index], json_data_len[index]);
			*data[json_data_len[index]] = 0;
		}
	}
}


static void config_parse_get_int(int *data,
	const char *field_name, const char *json_data[CONFIG_DATA_JSON_FIELD_NUM])
{
	int index = config_parse_get_field_index((char *)field_name);
	if ((index >= 0) && (json_data[index] != NULL)) {
		*data = atoi(json_data[index]);
	}
}


static void config_parse_get_u8(uint8_t *data,
	const char *field_name, const char *json_data[CONFIG_DATA_JSON_FIELD_NUM])
{
	int index = config_parse_get_field_index((char *)field_name);
	if ((index >= 0) && (json_data[index] != NULL)) {
		*data = (uint8_t)atoi(json_data[index]);
	}
}


static void config_parse_get_u16(uint16_t *data,
	const char *field_name, const char *json_data[CONFIG_DATA_JSON_FIELD_NUM])
{
	int index = config_parse_get_field_index((char *)field_name);
	if ((index >= 0) && (json_data[index] != NULL)) {
		*data = (uint16_t)atoi(json_data[index]);
	}
}


static void config_parse_get_u32(uint32_t *data,
	const char *field_name, const char *json_data[CONFIG_DATA_JSON_FIELD_NUM])
{
	int index = config_parse_get_field_index((char *)field_name);
	if ((index >= 0) && (json_data[index] != NULL)) {
		*data = (uint32_t)atoi(json_data[index]);
	}
}


static void config_parse_get_float(float *data,
	const char *field_name, const char *json_data[CONFIG_DATA_JSON_FIELD_NUM])
{
	int index = config_parse_get_field_index((char *)field_name);
	if ((index >= 0) && (json_data[index] != NULL)) {
		*data = (float)atof(json_data[index]);
	}
}


static void config_parse_get_bool(bool *data,
	const char *field_name, const char *json_data[CONFIG_DATA_JSON_FIELD_NUM])
{
	int index = config_parse_get_field_index((char *)field_name);
	if ((index >= 0) && (json_data[index] != NULL)) {
		*data = (bool)atof(json_data[index]);
	}
}


static void config_parse_get_time_second(uint32_t *out_second, const char *field_name,
	const char *json_data[CONFIG_DATA_JSON_FIELD_NUM], size_t json_data_len[CONFIG_DATA_JSON_FIELD_NUM])
{
	char buf[16] = {0};
	config_parse_get_string(buf, sizeof(buf), field_name, json_data, json_data_len);
	if (strlen(buf) <= 0) {
		return;
	}

	int hour = atoi(buf);
	char *p = appUtils_getString_afterChar(buf, ':');
	if (p == NULL) {
		return;
	}

	int minute = atoi(p);

	*out_second = hour * 3600 + minute * 60;
}