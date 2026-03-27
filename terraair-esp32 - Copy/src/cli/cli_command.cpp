
/*
 * cli_command.c
 *
 * This file will handle at top of command source. and initialize basic commands
 * 
 */

#include "cli_command.h"
#include "cli_typedef.h"
#include "cli_input.h"

#if (CONFIG_CLI_ENABLED == 1)

static cli_cmd_t *g_cli_list_cmd[CONFIG_CLI_MAX_COMMAND];
static uint8_t g_cli_list_cmd_num = 0;

static cli_cmd_t g_cli_help = {
    "help", 0, "\"help\"", "List all commands", NULL
};
static cli_cmd_t g_cli_reset = {
    "reset", 0, "\"reset\"", "Restart device", NULL
};


static uint8_t cli_command_help(void);
static uint8_t cli_command_reset(void);


/**
 * @brief initialize Command line and add basic commands
 * @param None
 * @return None
*/
void cli_command_init(void)
{
	g_cli_help.cli_cb = cli_command_help;
	g_cli_reset.cli_cb = cli_command_reset;
	g_cli_list_cmd[g_cli_list_cmd_num++] = &g_cli_help;
	g_cli_list_cmd[g_cli_list_cmd_num++] = &g_cli_reset;
	cli_input_init();
	cli_output_init();
	CLI_PutBuff((char *)CONFIG_CLI_STRING_ARROW);
}


/**
 * @brief register a command to system command list
 * @param cmd: command structure needs to add
 * @return
 * 		true: success
 * 		false: failure
*/
bool cli_command_registerCmd(cli_cmd_t *cmd)
{
	if (g_cli_list_cmd_num >= CONFIG_CLI_MAX_COMMAND) {
		return false;
	}
	if ((*cmd).command == NULL) {
		return false;
	}

	g_cli_list_cmd[g_cli_list_cmd_num] = cmd;
	g_cli_list_cmd_num++;
	return true;
}


/**
 * @brief Get a character from input source
 * @param c: input character
 * @return None
*/
void cli_command_getchar(char c)
{
	cli_input_get(c);
}


/**
 * @brief check command input. should call this function each 5ms or more
 * @param None
 * @return None
*/
void cli_command_running(void)
{
	cli_input_param_t input_value = cli_input_check_keycode();

	if (input_value.inValid == false) {
		return;
	}

	if (input_value.isEnter == true) {
		cli_input_keycode_enter(g_cli_list_cmd, g_cli_list_cmd_num);
	} else if (input_value.isBackspace == true) {
		cli_input_keycode_backspace();
	} else if (input_value.isKeyup == true) {
		cli_input_keycode_up();
	} else if (input_value.isKeydown == true) {
		cli_input_keycode_down();
	} else if (input_value.isKeyright == true) {
		cli_input_keycode_right();
	} else if (input_value.isKeyleft == true) {
		cli_input_keycode_left();
	} else if (input_value.isKeydel == true) {
		cli_input_keycode_delete();
	} else if (input_value.isKeyhome == true) {
		cli_input_keycode_home();
	} else if (input_value.isKeyend == true) {
		cli_input_keycode_end();
	} else if (input_value.isAlphabet == true) {
		cli_input_add_char(input_value.keycode);
	}
}


/**
 * @brief HELP Command, it will list all commands in system command list
 * @param None
 * @return
 * 		0: failure
 * 		1: success
*/
static uint8_t cli_command_help(void)
{
	CLI_PutBuff((char *)CONFIG_CLI_STRING_ENTER);
    CLI_PutBuff((char *)"  [command]            [Example]                               [Description]\r\n");
	for (uint8_t index = 0; index < g_cli_list_cmd_num; index++) {
		if ((*g_cli_list_cmd[index]).command == NULL) {
			continue;
		}
		CLI_PutBuff((char *)"   ");
		CLI_PutBuff((char *)(*g_cli_list_cmd[index]).command);
		for (uint8_t ui = strlen((*g_cli_list_cmd[index]).command); ui < 20; ui++) {
			CLI_PutChar(' ');
		}
		CLI_PutBuff((char *)(*g_cli_list_cmd[index]).ex);
		for (uint8_t ui = strlen((*g_cli_list_cmd[index]).ex); ui < 40; ui++) {
			CLI_PutChar(' ');
		}
		CLI_PutBuff((char *)(*g_cli_list_cmd[index]).despcription);
		CLI_PutBuff((char *)CONFIG_CLI_STRING_ENTER);
	}
	return 1;
}


/**
 * @brief RESET Command used to restart device
 * @param None
 * @return
 * 		0: failure
 * 		1: success
*/
static uint8_t cli_command_reset(void)
{
	cli_hw_cfg_reset_fcn();
	return 1;
}

#endif // CONFIG_CLI_ENABLED