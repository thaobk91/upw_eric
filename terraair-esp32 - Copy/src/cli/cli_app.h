
#ifndef __CLI_APP_H
#define __CLI_APP_H

#include "app_cfg.h"
#include "cli_typedef.h"

#if CONFIG_CLI_ENABLED == 1

/**
 * @brief initialize and add commands of Application
 * @param None
 * @return None
*/
void cli_app_init(void);

#endif // #if CONFIG_CLI_ENABLED == 1

#endif /* __CLI_APP_H */