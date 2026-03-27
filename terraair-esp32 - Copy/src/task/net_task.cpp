
#include "net_task.h"

CONFIG_LOG_TAG(NET_TASK)


/**
 * @brief Network Task. the task handles network connection
 * 
 * @param pArg input argurment of task
 * 
 * @return None
 */
void net_task(void *pArg)
{
    DBG_OUT_I("NET Task started\r\n");

    while (1) {
        FUNC_DELAY_MS(200);
    }
}