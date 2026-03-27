/*
 * cli_app.c
 *
 * This file includes functions for command line.
 * The Command line can be done via USB or UART. see the configuration in cli_task.c
 * 
 */

#include "cli_app.h"
#include "cli_output.h"
#include "cli_command.h"

#if CONFIG_CLI_ENABLED == 1

#define CONFIG_CLI_APP_RESP_OK              (char *)"\r\nOK\r\n"
#define CONFIG_CLI_APP_RESP_ERROR           (char *)"\033[0;31m\r\nERROR\r\n\033[0m"

#define FUNC_ADD_COMMAND(list_cmd, cmd, num, exam, desc, cb) {    \
            list_cmd.command         = cmd;                 \
            list_cmd.arg_num         = num;                 \
            list_cmd.ex              = exam;                \
            list_cmd.despcription    = desc;                \
            list_cmd.cli_cb          = cb;                  \
}

CONFIG_LOG_TAG(CLI_APP)
static cli_cmd_t g_cli_app_list_command[4];


static uint8_t cli_app_factoryReset(void);
static uint8_t cli_app_wifi_ssid_password(void);


/**
 * @brief initialize and add commands of Application
 * @param None
 * @return None
*/
void cli_app_init(void)
{
    memset(&g_cli_app_list_command, 0, sizeof(g_cli_app_list_command));
    FUNC_ADD_COMMAND(g_cli_app_list_command[0], "factory", 0, "\"factory\"",
            "Reset all parameters and configurations to default", cli_app_factoryReset);
	FUNC_ADD_COMMAND(g_cli_app_list_command[1], "wifi", 2, "\"wifi test 12345678\"",
            "Configure Wifi SSID and Password", cli_app_wifi_ssid_password);

    for (uint8_t i = 0; i < (sizeof(g_cli_app_list_command) / sizeof(g_cli_app_list_command[0])); i++) {
        cli_command_registerCmd(&g_cli_app_list_command[i]);
    }
}


/*** PRIVATE FUNCTION ***/

/**
 * @brief this function will reset all user/app data in memory and configure them to default parameters
 *  pls check cli_app::init() to see input parameters
 * @param None
 * @return
 *      0 - failure
 *      1 - success
*/
static uint8_t cli_app_factoryReset(void)
{
    cli_hw_cfg_putBuff(CONFIG_CLI_APP_RESP_OK);
    CLI_PutBuff((char *)"the Factory reset is success. restarting in 1 second.....\r\n");
    FUNC_DELAY_MS(1000);
    esp_restart();
    return 1;
}


/**
 * @brief  Configure WIFI SSID
 * 
 * @param None
 * 
 * @return
 *      0 - failure
 *      1 - success
*/
static uint8_t cli_app_wifi_ssid_password(void)
{
	char *str_1 = cli_output_getArg_string(0);
	char *str_2 = cli_output_getArg_string(1);

	if ((str_1 != nullptr) && (str_2 != nullptr)) {
		// memset(g_device_data.wifi_ssid, 0, sizeof(g_device_data.wifi_ssid));
		// memset(g_device_data.wifi_pwd, 0, sizeof(g_device_data.wifi_pwd));
		// strcpy(g_device_data.wifi_ssid, str_1);
		// strcpy(g_device_data.wifi_pwd, str_2);
		// app_storage_save_wifi_ssid(g_device_data.wifi_ssid);
		// app_storage_save_wifi_password(g_device_data.wifi_pwd);
		// CLI_PutBuff((char *)"Configured Wifi SSID and Password successfully\r\n");
		// appWifi_disconnect();
	} else {
		CLI_PutBuff((char *)CONFIG_CLI_APP_RESP_ERROR);
	}
    return 1;
}

#endif // #if CONFIG_CLI_ENABLED == 1