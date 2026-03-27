
#ifndef __CELL_CMD_H
#define __CELL_CMD_H

#include "cell_cfg.h"

/**
 * @brief Send AT Command and get response
 * 
 * @param atCmd AT Command
 * @param respOK response of successful
 * @param timeout_ms timeout in millisecond
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_cmd_sendAT(const char *atCmd, const char *respOK, uint32_t timeout_ms);

/**
 * @brief Get RX buffer
 * 
 * @param None
 * 
 * @return RX buffer pointer
 */
uint8_t *cell_cmd_get_rx_buffer(void);

/**
 * @brief Get RX buffer length
 * 
 * @param None
 * 
 * @return RX buffer length
 */
int cell_cmd_get_rx_buffer_len(void);

#endif // __CELL_CMD_H