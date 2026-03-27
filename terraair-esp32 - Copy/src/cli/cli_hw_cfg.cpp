
/*
 * cli_hw_cfg.c
 *
 * This file declares basic functions used to restart device and print data
 * 
 */

#include "cli_config.h"

#if (CONFIG_CLI_ENABLED == 1)

/**
 * @brief this function used to reset device
 * @param None
 * @return None
 */
void cli_hw_cfg_reset_fcn(void)
{
	esp_restart();
}


/**
 * @brief this function used to print a character
 * @param c: a character to print
 * @return None
 */
void cli_hw_cfg_putChar(char c)
{
	printf("%c", c);
	fflush(stdout);
}


/**
 * @brief this function used to print a string
 * @param c: a string to print
 * @return None
 */
void cli_hw_cfg_putBuff(char *c)
{
	uint16_t len = 0;
	for (len = 0; c[len] != 0; len++);
	if (len == 0) {
		return;
	}
	printf("%s", c);
	fflush(stdout);
}

#endif // CONFIG_CLI_ENABLED












