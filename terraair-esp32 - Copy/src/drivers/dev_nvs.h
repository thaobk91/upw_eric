
#ifndef __DEV_NVS_H
#define __DEV_NVS_H

#include "app_cfg.h"

/**
 * @brief initialize NVS flash
 * 
 * @param None
 * 
 * @retval true if success
 * @retval false if failure
 */
esp_err_t dev_nvs_init(void);

/**
 * @brief erase all data in NVS partition
 * 
 * @param None
 * 
 * @retval true if success
 * @retval false if failure
 */
bool dev_nvs_eraseAll(void);

/**
 * @brief erase data of a key
 * 
 * @param None
 * 
 * @retval true if success
 * @retval false if failure
 */
bool dev_mvs_delete_key(const char* key);

/**
 * @brief read size of data in NVS partition
 * 
 * @param key: the key where data is stored 
 * 
 * @retval integer - size of data
 */
int dev_nvs_read_size(const char *key);

/**
 * @brief read string data in NVS partition
 * 
 * @param key: the key where data is stored 
 * @param value: buffer to store string value
 * 
 * @retval integer - size of data
 */
int dev_nvs_readString(const char *key, char *value);

/**
 * @brief write a string data to NVS partition
 * 
 * @param key: the key where data is stored 
 * @param value: buffer to write to NVS
 * 
 * @retval true if success
 * @retval false if failure
 */
bool dev_nvs_writeString(const char *key, const char *value);

/**
 * @brief read a integer data from NVS partition
 * 
 * @param key: the key where data is stored 
 * @param value: integer value output
 * 
 * @retval true if success
 * @retval false if failure
 */
bool dev_nvs_readU32(const char *key, uint32_t *value);

/**
 * @brief write a integer data to NVS partition
 * 
 * @param key: the key where data is stored 
 * @param value: integer value input
 * 
 * @retval true if success
 * @retval false if failure
 */
bool dev_nvs_writeU32(const char *key, const uint32_t value);

#endif // __DEV_NVS_H