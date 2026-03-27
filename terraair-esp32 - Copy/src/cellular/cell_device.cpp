
#include "cell_device.h"
#include "cell_cmd.h"
#include "common/appUtils.h"

#define FUNC_CELL_PWR_KEY_HIGH()			digitalWrite(CONFIG_CELL_POWER_KEY_PIN, LOW)
#define FUNC_CELL_PWR_KEY_LOW()				digitalWrite(CONFIG_CELL_POWER_KEY_PIN, HIGH)

#define FUNC_CELL_POWER_ON()				digitalWrite(CONFIG_CELL_INT_PWR_EN_PIN, HIGH)
#define FUNC_CELL_POWER_OFF()				digitalWrite(CONFIG_CELL_INT_PWR_EN_PIN, LOW)

// #ifdef DUMP_AT_COMMANDS
// #include <StreamDebugger.h>
// StreamDebugger debugger(CONFIG_CELL_UART_PORT, CONFIG_DEBUG_PORT);
// TinyGsm        modem(debugger);
// #else
TinyGsm        modem(CONFIG_CELL_UART_PORT);
// #endif

TinyGsmClient gsmClient(modem);

CONFIG_LOG_TAG(CELL_DEVICE)
xSemaphoreHandle g_cell_lock;
static bool g_cell_baud_configured = false;


static bool cell_device_is_powerDown(void);
static bool cell_device_checkBaudrate(void);



/**
 * @brief initialize modem
 * 
 * @param None
 * 
 * @return None
 */
void cell_device_init(void)
{
	if (g_cell_lock == NULL) {
        g_cell_lock = xSemaphoreCreateMutex();
    }

    // LTE power key
    pinMode(CONFIG_CELL_POWER_KEY_PIN, OUTPUT);
	pinMode(CONFIG_CELL_INT_PWR_EN_PIN, OUTPUT);
    FUNC_CELL_POWER_OFF();
	FUNC_CELL_PWR_KEY_HIGH();

    CONFIG_CELL_UART_PORT.setRxBufferSize(8192);
    CONFIG_CELL_UART_PORT.begin(115200, SERIAL_8N1, CONFIG_CELL_UART_RX_PIN, CONFIG_CELL_UART_TX_PIN);
    CONFIG_CELL_UART_PORT.setTimeout(20);
}


/**
 * @brief start modem and configure uart baudrate
 * 
 * @param None
 * 
 * @retval true if modem started success
 * @retval false if error
 */
bool cell_device_start(void)
{
    FUNC_CELL_LOCK();
    int64_t ms = FUNC_GET_TICK_MS();
    DBG_OUT_I("Checking state and initializing .....\r\n");
	FUNC_CELL_PWR_KEY_HIGH();
	FUNC_DELAY_MS(10);
	FUNC_CELL_POWER_ON();

    /* waiting for Cellular module is ready */
    if (g_cell_baud_configured == false) {
        cell_device_pwrKeyTrigger();
        FUNC_DELAY_MS(1000);

        /* check current baudrate */
        CONFIG_CELL_UART_PORT.updateBaudRate(CONFIG_CELL_UART_BAUDRATE);
        FUNC_DELAY_MS(10);
		if (cell_cmd_sendAT("AT\r\n", "OK\r\n", 200) == false) {
            if (cell_device_checkBaudrate() == false) {
                FUNC_CELL_UNLOCK();
                return false;
            }
        }
    } else {
        /* waiting to cellular ready */
        bool cfun = false;
        DBG_OUT_I("Waiting modem ready\r\n");
        CONFIG_CELL_UART_PORT.updateBaudRate(CONFIG_CELL_UART_BAUDRATE);
        FUNC_DELAY_MS(10);
        do {
            if (cell_cmd_sendAT("AT\r\n", "OK\r\n", 200) == true) {
                cfun = true;
                break;
            }
            FUNC_DELAY_MS(100);
        } while ((FUNC_GET_TICK_MS() - ms) < 5000);

        if (cfun == false) {
            g_cell_baud_configured = false;
            DBG_OUT_E("Cellular isn't ready\r\n");
            FUNC_CELL_UNLOCK();
            return false;
        }
        DBG_OUT_H("modem is ready in %lld [ms]\r\n", (FUNC_GET_TICK_MS() - ms));
    }

	cell_cmd_sendAT("ATE1\r\n", "OK\r\n", 200);

	String modemInfo = modem.getModemInfo();
	String name = modem.getModemName();
	String manufacturer = modem.getModemManufacturer();
	String hw_ver = modem.getModemModel();
  	String fv_ver = modem.getModemRevision();  
  	DBG_OUT("Modem Info: %s\r\n", modemInfo.c_str());
	DBG_OUT("Modem Name: %s\r\n", name.c_str());
	DBG_OUT("Modem Manufacturer: %s\r\n", manufacturer.c_str());
	DBG_OUT("Modem Firware Version: %s\r\n", fv_ver.c_str());
	DBG_OUT("Modem Hardware Version: %s\r\n", hw_ver.c_str());

    g_cell_baud_configured = true;
    FUNC_CELL_UNLOCK();
    return true;
}


/**
 * @brief power key control
 * 
 * @param None
 * 
 * @retval None
 */
void cell_device_pwrKeyTrigger(void)
{
    FUNC_CELL_PWR_KEY_HIGH();
    delay(100);
    FUNC_CELL_PWR_KEY_LOW();
    delay(500);
    FUNC_CELL_PWR_KEY_HIGH();
}


/**
 * @brief Doing modem power off by hardware
 * 
 * @param None
 * 
 * @return None
 */
void cell_device_hard_power_off(void)
{
	FUNC_CELL_POWER_OFF();
    FUNC_CELL_PWR_KEY_HIGH();
	DBG_OUT_I("Modem is OFF\r\n");
}


/**
 * @brief Doing modem power off by hardware and command
 * 
 * @param None
 * 
 * @return None
 */
void cell_device_power_off(void)
{
    FUNC_CELL_LOCK();
    if (cell_cmd_sendAT((char *)"AT+CPOWD=1\r\n", (char *)"NORMAL POWER DOWN", 100) == true) {
		FUNC_DELAY_MS(1000);
	}
    FUNC_CELL_UNLOCK();
	cell_device_hard_power_off();
    DBG_OUT_I("Modem is OFF\r\n");
}


/**
 * @brief Check network connected or not
 * 
 * @param None
 * 
 * @retval true if connected
 * @retval false if not yet connected
 */
bool cell_device_network_connected(void)
{
    FUNC_CELL_LOCK();
     bool rc = 0;
    uint8_t try_count = 0;
    do {
        FUNC_DELAY_MS(1000);
        if (modem.isNetworkConnected() == true) {
            break;
        }
        try_count++;
    } while (try_count < 30);
    rc = modem.waitForNetwork(30000);
    FUNC_CELL_UNLOCK();
    return rc;
}


/**
 * @brief Get IMEI
 * 
 * @param imei imei buffer
 * @param imei_size maximum size of imei buffer
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_device_get_imei(char *imei, int imei_size)
{
	String buf = modem.getIMEI();
	if (buf.length() <= 0) {
		return false;
	}
	for (int i = 0; i < buf.length() && i < imei_size; i++) {
		imei[i] = buf.c_str()[i];
	}
	return true;
}


/**
 * @brief Get IMSI
 * 
 * @param imsi imsi buffer
 * @param imsi maximum size of imsi buffer
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_device_get_imsi(char *imsi, int imsi_size)
{
	String buf = modem.getIMSI();
	if (buf.length() <= 0) {
		return false;
	}
	for (int i = 0; i < buf.length() && i < imsi_size; i++) {
		imsi[i] = buf.c_str()[i];
	}
	return true;
}


/**
 * @brief Get ICCID
 * 
 * @param iccid iccid buffer
 * @param iccid_size maximum size of iccid buffer
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_device_get_iccid(char *iccid, int iccid_size)
{
	String buf = modem.getSimCCID();
	if (buf.length() <= 0) {
		return false;
	}
	for (int i = 0; i < buf.length() && i < iccid_size; i++) {
		iccid[i] = buf.c_str()[i];
	}
	return true;
}


/**
 * @brief Get signal strength RSSI
 * 
 * @param None
 * 
 * @return RSSI
 */
int cell_device_get_signalStrength(void)
{
    FUNC_CELL_LOCK();
    int x = modem.getSignalQuality();
    FUNC_CELL_UNLOCK();
    return x;
}


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
bool cell_device_connectToAPN(char *apn, char *user, char *password)
{
    FUNC_CELL_LOCK();
    modem.gprsConnect(apn, user, password);
    bool rc = modem.isGprsConnected();
    FUNC_CELL_UNLOCK();
    return rc;
}


/**
 * @brief Get time from network
 * 
 * @param cur_dt date time structure data
 * @param timeout_ms timeout in millisecond
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_device_get_time(cell_date_time_t *cur_dt, uint32_t timeout_ms)
{
    FUNC_CELL_LOCK();

    int64_t ms = FUNC_GET_TICK_MS();
    do {
        if (cell_cmd_sendAT((char *)"AT+CCLK?\r\n", (char *)"+CCLK: \"", 2000) == true) {
			int buf_len = cell_cmd_get_rx_buffer_len();
			if (buf_len > 0) {
				uint8_t *buf = cell_cmd_get_rx_buffer();
				if (appUtils_find_string(buf, buf_len, (char *)"+CCLK: \"80/0") == NULL) {
					char *p = strstr((char *)buf, "+CCLK: \"");
					if (p == NULL) {
						FUNC_CELL_UNLOCK();
						return false;
					}

					p += strlen("+CCLK: \"");
					cur_dt->year    = atoi(p + 0) + 2000;
					cur_dt->month   = atoi(p + 3);
					cur_dt->day     = atoi(p + 6);
					cur_dt->hour    = atoi(p + 9);
					cur_dt->minute  = atoi(p + 12);
					cur_dt->second  = atoi(p + 15);
					int timezone = atoi(p + 18);
					if (*(p + 17) == '-') {
						timezone = -timezone;
					}
					timezone = (timezone * 15) / 60;
					cur_dt->hour += timezone;
					// DBG_OUT("sych time: %d/%02d/%02d - %02d/%02d/%02d\r\n", 
					//                     cur_dt->year, cur_dt->month, cur_dt->day,
					//                     cur_dt->hour, cur_dt->minute, cur_dt->second);
					FUNC_CELL_UNLOCK();
					return true;
				}
			}
        }

        FUNC_DELAY_MS(1000);
    } while ((FUNC_GET_TICK_MS() - ms) < timeout_ms);

    FUNC_CELL_UNLOCK();
    return false;
}


/*** STATIC FUNCTIONS ***/

static bool cell_device_is_powerDown(void)
{
	int buf_len = cell_cmd_get_rx_buffer_len();
	if (buf_len <= 0) {
		return false;
	}
	uint8_t *buf = cell_cmd_get_rx_buffer();
	return (appUtils_find_string(buf, buf_len, (char *)"POWER DOWN") == NULL);
}


static bool cell_device_checkBaudrate(void)
{
    const uint32_t baudrate_list[] = {2400, 4800, 9600, 19200, 38400, 57600, 115200};
    uint8_t baudrate_list_num = sizeof(baudrate_list) / sizeof(baudrate_list[0]);
    uint8_t try_count = 0;

    do {
		uint8_t i = 0;
		do {
            /* change uart baudrate */
			CONFIG_CELL_UART_PORT.updateBaudRate(baudrate_list[i]);
			FUNC_DELAY_MS(10);
            DBG_OUT_RAW(".");
            /* send AT to check baudrate */
			if (cell_cmd_sendAT("AT\r\n", "OK\r\n", 200) == true) {
				if (baudrate_list[i] != CONFIG_CELL_UART_BAUDRATE) {
					char buf[32] = {0};
					snprintf(buf, sizeof(buf), "AT+IPR=%d\r\n", CONFIG_CELL_UART_BAUDRATE);
					if (cell_cmd_sendAT(buf, "OK\r\n", 200) == false) {
						return false;
					}
                    CONFIG_CELL_UART_PORT.updateBaudRate(CONFIG_CELL_UART_BAUDRATE);
				}
				DBG_OUT_I("set LTE baudrate success\r\n");
				return true;
			}
            if (cell_device_is_powerDown() == true) {
                return false;
            }
			i++;
		} while (i < baudrate_list_num);

		try_count++;
	} while (try_count < 3);

    DBG_OUT_E("Modem can't start. restarting...\r\n");
    return false;
}