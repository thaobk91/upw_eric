
#include "app_cfg.h"
#include "common/common.h"

#include "drivers/dev_nvs.h"

#include "task/app_task.h"
#include "task/net_task.h"
#include "task/cli_task.h"

CONFIG_LOG_TAG(MAIN)
device_data_t g_device_data;

/* Task handle data */
task_handle_t g_task_handle[ENUM_TASK_MAX_INDEX] = {
    [ENUM_TASK_APP_INDEX	] 	= {.enabled = true},
	[ENUM_TASK_NET_INDEX	] 	= {.enabled = true},
#if CONFIG_CLI_ENABLED == true
	[ENUM_TASK_CLI_INDEX	] 	= {.enabled = true},
#endif // CONFIG_CLI_ENABLED
};


void setup() {
    CONFIG_DEBUG_PORT.begin(115200);

    /* Init NVS */
    dev_nvs_init();

    DBG_OUT_RAW_I("\r\n********* Scoreboard v%s. Released on %d *********\r\n",
					CONFIG_FW_VERSION, CONFIG_FW_RELEASE_DATETIME);
    common_show_chipInfo();

    /* Get Mac */
    common_get_wifiMac_string(g_device_data.wifiMac_str, sizeof(g_device_data.wifiMac_str));
    DBG_OUT_I("Wifi Mac: %s\r\n", g_device_data.wifiMac_str);

	/* Load Data in Memory */
	// app_storage_load_data();
	DBG_OUT("Wifi Credentials: %s - %s\r\n", g_device_data.network_info.wifi.sta_ssid, g_device_data.network_info.wifi.sta_pwd);

    /* Output */
	// pinMode(CONFIG_LED_PWR_CTRL_PIN, OUTPUT);

	/* Input */
	// pinMode(CONFIG_DIRECTION_SENSOR_PIN, INPUT);

    /* create tasks */
    xTaskCreatePinnedToCore(app_task	, "app_task"	, (8 * 1024), NULL, 5, &g_task_handle[ENUM_TASK_APP_INDEX].handle	, 1	);
	xTaskCreatePinnedToCore(net_task	, "net_task"	, (16* 1024), NULL, 5, &g_task_handle[ENUM_TASK_NET_INDEX].handle	, 1	);
#if CONFIG_CLI_ENABLED == true
    xTaskCreatePinnedToCore(cli_task	, "cli_task"	, (4 * 1024), NULL, 4, &g_task_handle[ENUM_TASK_CLI_INDEX].handle	, 0	);
#endif // CONFIG_CLI_ENABLED
}


void loop() {
	DBG_OUT("Total heap: %d\r\n", ESP.getHeapSize());
    DBG_OUT("Free heap: %d\r\n", ESP.getFreeHeap());
    DBG_OUT("Total PSRAM: %d\r\n", ESP.getPsramSize());
    DBG_OUT("Free PSRAM: %d\r\n", ESP.getFreePsram());

	vTaskDelete(NULL);
	FUNC_DELAY_MS(5000);
}