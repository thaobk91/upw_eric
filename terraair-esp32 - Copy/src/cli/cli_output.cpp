
/*
 * cli_output.c
 *
 * This file will handle data output from CLI
 * 
 */
#include "cli_output.h"

#if (CONFIG_CLI_ENABLED == 1)

static cli_args_t cmd_agrs;


static bool cli_output_parse(char *buff, uint16_t buff_len);


/**
 * @brief initialize output parameters
 * @param None
 * @return None
*/
void cli_output_init(void)
{
	memset(&cmd_agrs, 0, sizeof(cmd_agrs));
}


/**
 * @brief handle and parse input command
 * @param None
 * @return
 * 		true: command is OK
 * 		false: failure
*/
bool cli_output_checkcommand(cli_cmd_t **cmd_list,
		uint8_t cmd_count, char *buff, uint16_t buff_len)
{
	for (uint16_t ui = 0; cmd_agrs.cmd[ui] > 0; ui++) {
		cmd_agrs.cmd[ui] = 0;
	}
	for (uint8_t index = 0; index < cmd_agrs.argc; index++) {
		for (uint16_t ui = 0; cmd_agrs.argv[index][ui] > 0; ui++) {
			cmd_agrs.argv[index][ui] = 0;
		}
	}
	cmd_agrs.argc = 0;

	if (cli_output_parse(buff, buff_len) == false) {
		return false;
	}

	for (uint8_t index = 0; index < cmd_count; index++) {
		if ((*cmd_list[index]).command == NULL) {
			return false;
		}

		if (cmd_agrs.argc != (*cmd_list[index]).arg_num) {
			continue;
		}
		if (strcmp(cmd_agrs.cmd, (*cmd_list[index]).command) != 0) {
			continue;
		}

		if ((*cmd_list[index]).cli_cb != NULL) {
			(*cmd_list[index]).cli_cb();
		}
		return true;
	}

	return false;
}


/**
 * @brief Get a integer number from argument
 * @param arg_index: argument index
 * @return integer number
*/
int32_t cli_output_getArg_integer(uint8_t arg_index)
{
	return atoi(cmd_agrs.argv[arg_index]);
}


/**
 * @brief Get a float number from argument
 * @param arg_index: argument index
 * @return float number
*/
float cli_output_getArg_float(uint8_t arg_index)
{
	return atof(cmd_agrs.argv[arg_index]);
}


/**
 * @brief Get a string from argument
 * 
 * @param arg_index: argument index
 * 
 * @return string
*/
char *cli_output_getArg_string(uint8_t arg_index)
{
	return cmd_agrs.argv[arg_index];
}


/*** PRIVATE FUNCTION ***/

/**
 * @brief parse command
 * @param
 * 		buff: command input in string type
 * 		buff_len: length of command
 * @return
 * 		true: command is ok
 * 		false: failure
*/
static bool cli_output_parse(char *buff, uint16_t buff_len)
{
	uint16_t i = 0;
	bool space_locked = false;

	// get command
	while ((*buff != 0) && (*buff != ' ')) {
		cmd_agrs.cmd[i++] = *buff++;
		if (i >= CONFIG_CLI_COMMAND_MAX_SIZE) {
			return false;
		}
	}

	while ((*buff != 0) && (*buff == ' ')) {
		buff++;
	}

	if (*buff == 0) {
		return (i > 0);
	}

	i = 0;

	// get argument
	bool isCode_0x5C = false; /* '\' */
	while (*buff != 0) {
		if ((isCode_0x5C == false) && (*buff == '\\')) {
			isCode_0x5C = true;
		} else if ((isCode_0x5C == false) && (*buff == ' ')) {
			if (space_locked == false) {
				i = 0;
				cmd_agrs.argc++;
				if (cmd_agrs.argc >= CONFIG_CLI_MAX_ARGUMENT) {
					return false;
				}
				space_locked = true;
			}
		} else {
			space_locked = false;
			isCode_0x5C = false;
			cmd_agrs.argv[cmd_agrs.argc][i++] = *buff;
			if (i >= CONFIG_CLI_ARG_MAX_SIZE) {
				return false;
			}
		}

		buff++;
	}

	if (i > 0) {
		cmd_agrs.argc++;
	}

	return true;
}

#endif // CONFIG_CLI_ENABLED 




















