
#include "cli_task.h"
#include "cli/cli_command.h"
#include "cli/cli_app.h"

#if (CONFIG_CLI_ENABLED == 1)

CONFIG_LOG_TAG(CLI_TASK)


/**
 * @brief CLI Task. The task handles command via serial/USB port
 * 
 * @param pArg input argurment of task
 * 
 * @return None
 */
void cli_task(void *pArg)
{
    DBG_OUT_I("CLI task started\r\n");
    FUNC_DELAY_MS(100);
    cli_command_init();
    cli_app_init();

    while (1) {
        FUNC_DELAY_MS(50);

        if (CONFIG_DEBUG_PORT.available() > 0) {
            char c = 0;
            if (CONFIG_DEBUG_PORT.read(&c, 1) > 0) {
                cli_command_getchar(c);
                cli_command_running();
            }
        }
    }
}

#endif //CONFIG_CLI_ENABLED