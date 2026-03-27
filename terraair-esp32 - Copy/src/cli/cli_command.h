
#ifndef CLI_COMMAND_CLI_COMMAND_H_
#define CLI_COMMAND_CLI_COMMAND_H_

#include "cli_output.h"

#if (CONFIG_CLI_ENABLED == 1)

/**
 * @brief initialize Command line and add basic commands
 * @param None
 * @return None
*/
void cli_command_init(void);

/**
 * @brief register a command to system command list
 * @param cmd: command structure needs to add
 * @return
 * 		true: success
 * 		false: failure
*/
bool cli_command_registerCmd(cli_cmd_t *cmd);

/**
 * @brief Get a character from input source
 * @param c: input character
 * @return None
*/
void cli_command_getchar(char c);

/**
 * @brief check command input. should call this function each 5ms or more
 * @param None
 * @return None
*/
void cli_command_running(void);

#endif // CONFIG_CLI_ENABLED

#endif /* CLI_COMMAND_CLI_COMMAND_H_ */
