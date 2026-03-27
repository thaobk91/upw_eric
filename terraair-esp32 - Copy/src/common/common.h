
#ifndef __M_COMMON_H
#define __M_COMMON_H

#include "app_cfg.h"

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
} date_time_t;

/**
 * @brief show chip info
 * 
 * @param None
 * 
 * @retval None
 */
void common_show_chipInfo(void);

/**
 * @brief Get wifi MAC in string type
 * 
 * @param wifiMac: wifi mac buffer
 * @param wifiMac_size: size of wifi mac buffer
 * 
 * @retval None
 */
void common_get_wifiMac_string(char *wifiMac, int wifiMac_size);

/**
 * @brief Get ble MAC in string type
 * 
 * @param bleMac: ble mac buffer
 * @param bleMac_size: size of ble mac buffer
 * 
 * @retval None
 */
void common_get_bleMac_string(char *bleMac, int bleMac_size);

/**
 * @brief Set NTP server to sync date time
 * 
 * @param None
 * 
 * @retval None
 */
void common_sync_time(void);

/**
 * @brief Get Date time
 * 
 * @param None
 * 
 * @retval Date time structure data
 */
date_time_t common_getDateTime(void);

/**
 * @brief Get Time Stamp
 * 
 * @param None
 * 
 * @retval Time stamp
 */
uint32_t common_get_timestamp(void);

/**
 * @brief Configure Date Time
 * 
 * @param dt input date time structure data
 * 
 * @return None
 */
void common_set_time(date_time_t dt);

#endif // __M_COMMON_H