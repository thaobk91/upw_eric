
#ifndef __CELL_DEVICE_H
#define __CELL_DEVICE_H

#include "cell_cfg.h"

/**
 * @brief initialize modem
 * 
 * @param None
 * 
 * @return None
 */
void cell_device_init(void);

/**
 * @brief power key control
 * 
 * @param None
 * 
 * @retval None
 */
void cell_device_pwrKeyTrigger(void);

/**
 * @brief Doing modem power off by hardware
 * 
 * @param None
 * 
 * @return None
 */
void cell_device_hard_power_off(void);

/**
 * @brief Doing modem power off by hardware and command
 * 
 * @param None
 * 
 * @return None
 */
void cell_device_power_off(void);

/**
 * @brief Get IMEI
 * 
 * @param imei imei buffer
 * @param imei_size maximum size of imei buffer
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_device_get_imei(char *imei, int imei_size);

/**
 * @brief Get IMSI
 * 
 * @param imsi imsi buffer
 * @param imsi maximum size of imsi buffer
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_device_get_imsi(char *imsi, int imsi_size);

/**
 * @brief Get ICCID
 * 
 * @param iccid iccid buffer
 * @param iccid_size maximum size of iccid buffer
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_device_get_iccid(char *iccid, int iccid_size);

/**
 * @brief Check network connected or not
 * 
 * @param None
 * 
 * @retval true if connected
 * @retval false if not yet connected
 */
bool cell_device_network_connected(void);

/**
 * @brief Get signal strength RSSI
 * 
 * @param None
 * 
 * @return RSSI
 */
int cell_device_get_signalStrength(void);

/**
 * @brief Configure APN
 * 
 * @param apn Operator APN
 * @param user APN Username
 * @param password APN Password
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_device_connectToAPN(char *apn, char *user, char *password);

/**
 * @brief Enable GPS
 * 
 * @param None
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_device_gps_on(void);

/**
 * @brief Disable GPS
 * 
 * @param None
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_device_gps_off(void);

/**
 * @brief Get time from network
 * 
 * @param cur_dt date time structure data
 * @param timeout_ms timeout in millisecond
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_device_get_time(cell_date_time_t *cur_dt, uint32_t timeout_ms);

#endif // __CELL_DEVICE_H