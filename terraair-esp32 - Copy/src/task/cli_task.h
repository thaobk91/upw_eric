
#ifndef __CLI_TASK_H
#define __CLI_TASK_H

#include "app_cfg.h"

#if (CONFIG_CLI_ENABLED == 1)

/**
 * @brief CLI Task. this task will handle command via serial/USB port
 * 
 * @param pArg input argurment of task
 * 
 * @return None
 */
void cli_task(void *pArg);

#endif //CONFIG_CLI_ENABLED

#endif // __CLI_TASK_H