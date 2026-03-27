
#ifndef CLI_COMMAND_CLI_CONFIG_H_
#define CLI_COMMAND_CLI_CONFIG_H_

#include "app_cfg.h"

#if (CONFIG_CLI_ENABLED == 1)

// configuration
#define CONFIG_CLI_MAX_COMMAND		    (8)
#define CONFIG_CLI_MAX_ARGUMENT			(2)
#define CONFIG_CLI_COMMAND_MAX_SIZE		(32)
#define CONFIG_CLI_ARG_MAX_SIZE			(64)
#define CONFIG_CLI_LOG_SIZE				(3)
#define CONFIG_CLI_STRING_ENTER			"\r\n"
#define CONFIG_CLI_STRING_ARROW			"\033[0;32mSCB>> \033[0m"
#define CONFIG_CLI_QUEUE_SIZE			(64)

#define CONFIG_CLI_INPUT_MAX_SIZE		(CONFIG_CLI_COMMAND_MAX_SIZE + CONFIG_CLI_MAX_ARGUMENT * \
                                        (CONFIG_CLI_ARG_MAX_SIZE + 1) + 1)

#define CLI_PRINT_OUTPUT				(1)
#define CLI_LOG_ENABLE					(1)
#define CLI_ECHO_EN						(1)


// ************************* IO Terminal Settings ***************************

#if (CLI_PRINT_OUTPUT == 1)
void cli_hw_cfg_reset_fcn(void);
void cli_hw_cfg_putChar(char c);
void cli_hw_cfg_print(const char* str);
void cli_hw_cfg_putBuff(char *c);

#if (CLI_ECHO_EN == 1)
#define CLI_PutChar						cli_hw_cfg_putChar
#define CLI_PutBuff						cli_hw_cfg_putBuff
#else	// CLI_ECHO_EN != 1
#define CLI_PutChar
#define CLI_PutBuff
#endif	// CLI_ECHO_EN == 1

#else	// CLI_PRINT_OUTPUT != 1
#define CLI_Printf
#define CLI_PutChar
#endif	// CLI_PRINT_OUTPUT == 1

#endif //CONFIG_CLI_ENABLED

#endif /* CLI_COMMAND_CLI_CONFIG_H_ */