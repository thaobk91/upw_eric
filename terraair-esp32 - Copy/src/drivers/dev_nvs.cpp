
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "dev_nvs.h"

#define CONFIG_NVS_STORAGE_NAMESPACE 	    "nvs_part"


/**
 * @brief initialize NVS flash
 * 
 * @param None
 * 
 * @retval true if success
 * @retval false if failure
 */
esp_err_t dev_nvs_init(void)
{
    esp_err_t retVal = nvs_flash_init();
    if (retVal != ESP_OK) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        retVal = nvs_flash_init();
    }
    return (retVal == ESP_OK);
}


/**
 * @brief erase all data in NVS partition
 * 
 * @param None
 * 
 * @retval true if success
 * @retval false if failure
 */
bool dev_nvs_eraseAll(void)
{
    return (nvs_flash_erase() == ESP_OK);
}


/**
 * @brief erase data of a key
 * 
 * @param None
 * 
 * @retval true if success
 * @retval false if failure
 */
bool dev_mvs_delete_key(const char* key)
{
	nvs_handle_t handle;
	esp_err_t err = nvs_open(CONFIG_NVS_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
	if (err != ESP_OK) {
		return false;
	}
	bool rc = (nvs_erase_key(handle, key) == ESP_OK);
	nvs_close(handle);
	return rc;
}


/**
 * @brief read size of data in NVS partition
 * 
 * @param key: the key where data is stored
 * 
 * @retval integer - size of data
 */
int dev_nvs_read_size(const char *key)
{
    nvs_handle_t handle;
	esp_err_t err = nvs_open(CONFIG_NVS_STORAGE_NAMESPACE, NVS_READONLY, &handle);
	if (err != ESP_OK) {
		return 0;
	}

	size_t read_size = 0;
	err = nvs_get_str(handle, key, NULL, &read_size);
	nvs_close(handle);
	return read_size;
}


/**
 * @brief read string data in NVS partition
 * 
 * @param key: the key where data is stored 
 * @param value: buffer to store string value
 * 
 * @retval integer - size of data
 */
int dev_nvs_readString(const char *key, char *value)
{
    nvs_handle_t handle;
	esp_err_t err = nvs_open(CONFIG_NVS_STORAGE_NAMESPACE, NVS_READONLY, &handle);
	if (err != ESP_OK) {
		return 0;
	}

	size_t read_len = 0;
	nvs_get_str(handle, key, NULL, &read_len);
	nvs_get_str(handle, key, value, &read_len);
	nvs_close(handle);
	return read_len;
}


/**
 * @brief write a string data to NVS partition
 * 
 * @param key: the key where data is stored 
 * @param value: buffer to write to NVS
 * 
 * @retval true if success
 * @retval false if failure
 */
bool dev_nvs_writeString(const char *key, const char *value)
{
	if ((value == nullptr) || (strlen(value) <= 0)) {
		return false;
	}

    nvs_handle_t handle;
	esp_err_t err = nvs_open(CONFIG_NVS_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
	if (err != ESP_OK) {
		return false;
	}

	err = nvs_set_str(handle, key, value);
	if (err != ESP_OK) {
		nvs_close(handle);
		return false;
	}
	err = nvs_commit(handle);
	nvs_close(handle);
	return (err == ESP_OK);
}


/**
 * @brief read a integer data from NVS partition
 * 
 * @param key: the key where data is stored 
 * @param value: integer value output
 * 
 * @retval true if success
 * @retval false if failure
 */
bool dev_nvs_readU32(const char *key, uint32_t *value)
{
    nvs_handle_t handle;
    esp_err_t retVal = nvs_open(CONFIG_NVS_STORAGE_NAMESPACE, NVS_READONLY, &handle);
    if(retVal != ESP_OK) {
        return false;
    }

    retVal = nvs_get_u32(handle, key, value);
    nvs_close(handle);
    if (retVal != ESP_OK) {
        return false;
    }
    return true;

}


/**
 * @brief write a integer data to NVS partition
 * 
 * @param key: the key where data is stored 
 * @param value: integer value input
 * 
 * @retval true if success
 * @retval false if failure
 */
bool dev_nvs_writeU32(const char *key, const uint32_t value)
{
    nvs_handle_t handle;
    esp_err_t retVal = nvs_open(CONFIG_NVS_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (retVal != ESP_OK) {
        return false;
    }

    retVal = nvs_set_u32(handle, key, value);
    if (retVal != ESP_OK) {
        nvs_close(handle);
        return false;
    }

    retVal = nvs_commit(handle);
    nvs_close(handle);
    return (retVal == ESP_OK);
}