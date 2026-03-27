
#ifndef __CONFIG_PARSE_H
#define __CONFIG_PARSE_H

#include "app_cfg.h"

/**
 * @brief parse configuration from JSON data
 * 
 * @param data json data
 * @param data_len length of data
 * 
 * @retval true if success
 * @retval false if false
 */
bool config_parse_json_file(char *data, uint16_t data_len);

/**
 * @brief print all configurations
 * 
 * @param None
 * 
 * @return None
 */
void config_parse_print(void);

#endif // __CONFIG_PARSE_H