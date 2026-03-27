
#include "cell_cmd.h"
#include "common/appUtils.h"

static uint8_t g_cell_rx_buf[CONFIG_CELL_BUF_MAX_SIZE] = {0};
static int g_cell_rx_buf_len = 0;


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
bool cell_cmd_sendAT(const char *atCmd, const char *respOK, uint32_t timeout_ms)
{
    bool rc = false;
    int64_t ms = FUNC_GET_TICK_MS();
    memset(g_cell_rx_buf, 0, sizeof(g_cell_rx_buf));
	g_cell_rx_buf_len = 0;

	CONFIG_CELL_UART_PORT.print(atCmd);
    do {   
        FUNC_DELAY_MS(10);
        if (CONFIG_CELL_UART_PORT.available() > 0) {
            size_t rx_len = CONFIG_CELL_UART_PORT.readBytes(&g_cell_rx_buf[g_cell_rx_buf_len], sizeof(g_cell_rx_buf) - g_cell_rx_buf_len);
            if (rx_len > 0) {
                DBG_OUT_RAW((char *)&g_cell_rx_buf[g_cell_rx_buf_len]);
				g_cell_rx_buf_len += rx_len;
            }

            if (appUtils_find_string(g_cell_rx_buf, g_cell_rx_buf_len, (char *)"ERROR") != NULL) {
                break;
            } else if (appUtils_find_string(g_cell_rx_buf, g_cell_rx_buf_len, (char *)respOK) != NULL) {
                rc = true;
                break;
            }
        }
    } while ((FUNC_GET_TICK_MS() - ms) < timeout_ms);

	FUNC_DELAY_MS(1);
    if (CONFIG_CELL_UART_PORT.available() > 0) {
		FUNC_DELAY_MS(10);
    	size_t rx_len = CONFIG_CELL_UART_PORT.readBytes(&g_cell_rx_buf[g_cell_rx_buf_len], sizeof(g_cell_rx_buf) - g_cell_rx_buf_len);
    	if (rx_len > 0) {
			DBG_OUT_RAW((char *)&g_cell_rx_buf[g_cell_rx_buf_len]);
			g_cell_rx_buf_len += rx_len;
		}
	}
    return rc;
}


/**
 * @brief Get RX buffer
 * 
 * @param None
 * 
 * @return RX buffer pointer
 */
uint8_t *cell_cmd_get_rx_buffer(void)
{
	return &g_cell_rx_buf[0];
}


/**
 * @brief Get RX buffer length
 * 
 * @param None
 * 
 * @return RX buffer length
 */
int cell_cmd_get_rx_buffer_len(void)
{
	return g_cell_rx_buf_len;
}