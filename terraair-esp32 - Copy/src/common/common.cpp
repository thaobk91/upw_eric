
/*
 * m_common.c
 *
 * This file implements basic function of device
 * 
 */

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "dev_mem.h"
#include "common.h"

CONFIG_LOG_TAG(COMMON)
static char g_hex_str[] = "123456789ABCDEF";


/**
 * @brief show chip info
 * 
 * @param None
 * 
 * @retval None
 */
void common_show_chipInfo(void)
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    DBG_OUT_I("This is %s chip with %d CPU core(s), %s%s%s%s, silicon revision v%d.%d\r\n",
                CONFIG_IDF_TARGET,
                chip_info.cores,
                (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
                (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
                (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
                (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "",
                chip_info.revision / 100,
                chip_info.revision % 100);

    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        DBG_OUT_I("Get flash size failed\r\n");
        return;
    }
    DBG_OUT_I("%" PRIu32 "MB %s flash\r\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    DBG_OUT_I("Minimum free heap size: %" PRIu32 " bytes\r\n", esp_get_minimum_free_heap_size());
}


/**
 * @brief Get wifi MAC in string type
 * 
 * @param wifiMac: wifi mac buffer
 * @param wifiMac_size: size of wifi mac buffer
 * 
 * @retval None
 */
void common_get_wifiMac_string(char *wifiMac, int wifiMac_size)
{
    if (wifiMac_size <= 0) {
        return;
    }
	uint8_t mac_addr[6] = {0};
    /* getting wifi mac */
	esp_read_mac(mac_addr, ESP_MAC_WIFI_STA);
    int mac_len = 0;
    /* convert mac hex to hex string */
    for (int i = 0; i < 6 && mac_len < (wifiMac_size - 1); i++) {
        wifiMac[mac_len++] = g_hex_str[mac_addr[i] / 16];
        wifiMac[mac_len++] = g_hex_str[mac_addr[i] % 16];
    }
}


/**
 * @brief Get ble MAC in string type
 * 
 * @param bleMac: ble mac buffer
 * @param bleMac_size: size of ble mac buffer
 * 
 * @retval None
 */
void common_get_bleMac_string(char *bleMac, int bleMac_size)
{
    if (bleMac_size <= 0) {
        return;
    }
	uint8_t mac_addr[6] = {0};
    /* getting ble mac */
	esp_read_mac(mac_addr, ESP_MAC_BT);
    int mac_len = 0;
    /* convert mac hex to hex string */
    for (int i = 0; i < 6 && mac_len < (bleMac_size - 1); i++) {
        bleMac[mac_len++] = g_hex_str[mac_addr[i] / 16];
        bleMac[mac_len++] = g_hex_str[mac_addr[i] % 16];
    }
}


/**
 * @brief Set NTP server to sync date time
 * 
 * @param None
 * 
 * @retval None
 */
void common_sync_time(void)
{
    const char* ntpServer = "pool.ntp.org";
    const long  gmtOffset_sec = 0;
    const int   daylightOffset_sec = 3600;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}


/**
 * @brief Get Date time
 * 
 * @param None
 * 
 * @retval Date time structure data
 */
date_time_t common_getDateTime(void)
{
    struct tm timeinfo;
    getLocalTime(&timeinfo);

    date_time_t datetime = {0};
    datetime.year       = timeinfo.tm_year + 1900;
    datetime.month      = timeinfo.tm_mon + 1;
    datetime.day        = timeinfo.tm_mday;
    datetime.hour       = timeinfo.tm_hour;
    datetime.minute     = timeinfo.tm_min;
    datetime.second     = timeinfo.tm_sec;
    DBG_OUT("Get DateTime: %d/%02d/%02d - %02d:%02d:%02d\r\n", 
                    datetime.year, datetime.month, datetime.day, 
                    datetime.hour, datetime.minute, datetime.second);
    return datetime;
}


/**
 * @brief Get Time Stamp
 * 
 * @param None
 * 
 * @retval Time stamp
 */
uint32_t common_get_timestamp(void)
{
    struct tm timeinfo;
    getLocalTime(&timeinfo);
	time_t t_of_day;
	t_of_day = mktime(&timeinfo);
	// DBG_OUT("epoc time: %ld\r\n", (long)t_of_day);
	return (uint32_t)t_of_day;
}


/**
 * @brief Configure Date Time
 * 
 * @param dt input date time structure data
 * 
 * @return None
 */
void common_set_time(date_time_t dt)
{
    struct tm tm;
    tm.tm_year = dt.year - 1900;
    tm.tm_mon = dt.month - 1;
    tm.tm_mday = dt.day;
    tm.tm_hour = dt.hour;
    tm.tm_min = dt.minute;
    tm.tm_sec = dt.second;
    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);
}