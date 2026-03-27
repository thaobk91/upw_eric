
#ifndef CLI_COMMAND_CLI_OUTPUT_H_
#define CLI_COMMAND_CLI_OUTPUT_H_

#include "cli_config.h"
#include "cli_typedef.h"

#if (CONFIG_CLI_ENABLED == 1)

/**
 * @brief initialize output parameters
 * @param None
 * @return None
*/
void cli_output_init(void);

/**
 * @brief handle and parse input command
 * @param None
 * @return
 * 		true: command is OK
 * 		false: failure
*/
bool cli_output_checkcommand(cli_cmd_t **cmd_list,
		uint8_t cmd_count, char *buff, uint16_t buff_len);

/**
 * @brief Get a integer number from argument
 * @param arg_index: argument index
 * @return integer number
*/
int32_t cli_output_getArg_integer(uint8_t arg_index);

/**
 * @brief Get a float number from argument
 * @param arg_index: argument index
 * @return float number
*/
float cli_output_getArg_float(uint8_t arg_index);

/**
 * @brief Get a string from argument
 * 
 * @param arg_index: argument index
 * 
 * @return string
*/
char *cli_output_getArg_string(uint8_t arg_index);

#endif // CONFIG_CLI_ENABLED

#endif /* CLI_COMMAND_CLI_OUTPUT_H_ */
