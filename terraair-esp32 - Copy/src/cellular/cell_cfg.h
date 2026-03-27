
#ifndef __CELL_CFG_H
#define __CELL_CFG_H

#include "app_cfg.h"

#define TINY_GSM_MODEM_BG96
#define TINY_GSM_USE_GPRS 		true  // Internet Access Through GPRS

#include <TinyGsmClient.h>

#define CONFIG_CELL_BUF_MAX_SIZE				4096

extern xSemaphoreHandle g_cell_lock;

#define FUNC_CELL_LOCK()			{													\
										if (g_cell_lock != NULL) {						\
											xSemaphoreTake(g_cell_lock, portMAX_DELAY );\
										}												\
									}
#define FUNC_CELL_UNLOCK()			{													\
										if (g_cell_lock != NULL) {						\
											xSemaphoreGive(g_cell_lock);				\
										}												\
									}

/* date time */
typedef struct {
    int     year;
    int     month;
    int     day;
    int     hour;
    int     minute;
    int     second;
    int     dayofweek;
    float   timezone;
} cell_date_time_t;

#endif // __CELL_CFG_H