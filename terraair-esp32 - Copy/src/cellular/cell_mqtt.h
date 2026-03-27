
#ifndef __CELL_MQTT_H
#define __CELL_MQTT_H

#include "cell_cfg.h"

#define CONFIG_CELL_MQTT_MAX_SUB_TOPIC				3
#define CONFIG_CELL_MQTT_BUF_SIZE					2048

typedef struct {
    char    *url = NULL;
    int     port = 80;
    char    *user = NULL;
    char    *password = NULL;
    char    *rootCA = NULL;
    char    *clientCert = NULL;
    char    *clientPkey = NULL;
    char    *clientId = NULL;
} cell_mqtt_cfg_t;

#endif // __CELL_MQTT_H