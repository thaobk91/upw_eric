
#include "app_task.h"

CONFIG_LOG_TAG(APP_TASK)
static int64_t g_dbg_last_ms = -30000;


/**
 * @brief APP Task. the task handles application data
 * 
 * @param pArg input argurment of task
 * 
 * @return None
 */
void app_task(void *pArg)
{
    DBG_OUT_I("APP Task started\r\n");

    while (1) {
        FUNC_DELAY_MS(200);

		/* Debug */
		if ((FUNC_GET_TICK_MS() - g_dbg_last_ms) >= 30000) {
			DBG_OUT("free Heap size           : %ld\r\n", esp_get_free_heap_size());
			DBG_OUT("free MALLOC_CAP_INTERNAL : %d\r\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
			g_dbg_last_ms = FUNC_GET_TICK_MS();
		}
    }
}