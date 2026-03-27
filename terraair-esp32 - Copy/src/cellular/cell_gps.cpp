
#include "cell_gps.h"
#include "cell_cmd.h"
#include "common/appUtils.h"

extern TinyGsm modem;


static float cell_gps_convertLatLon_toFloat(char *data);


/**
 * @brief Enable GPS
 * 
 * @param None
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_gps_on(void)
{
    FUNC_CELL_LOCK();
    bool rc = modem.enableGPS();
    FUNC_CELL_UNLOCK();
    return rc;
}


/**
 * @brief Disable GPS
 * 
 * @param None
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_gps_off(void)
{
    FUNC_CELL_LOCK();
    bool rc = modem.disableGPS();
    FUNC_CELL_UNLOCK();
    return rc;
}


/**
 * @brief Enable/Disable GPS Auto Start
 * 
 * @param enable true to enable, false to disable
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_gps_set_auto_start(bool enable)
{
    FUNC_CELL_LOCK();
	char buf[32] = {0};
	snprintf(buf, sizeof(buf), "AT+QGPSCFG=\"autogps\",%d\r\n", enable);
    bool rc = (cell_cmd_sendAT(buf, "OK\r\n", 2000) == true);
    FUNC_CELL_UNLOCK();
    return rc;
}


/**
 * @brief Get location
 * 
 * @param gps_data GPS data structure
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_gps_get_location(gps_data_t *gps_data)
{
    FUNC_CELL_LOCK();
	gps_data_t gps_data_cur = {0};

	/* RMC */
	if ((cell_cmd_sendAT("AT+QGPSGNMEA=\"RMC\"\r\n", "OK\r\n", 2000) == true) == true) {
		// DBG_OUT_RAW(LOG_LVL_1, "NMEA: %s\r\n", g_gnss_stream_buf);
		/* check RMC */
		int buf_len = cell_cmd_get_rx_buffer_len();
		while (buf_len > 0) {
			uint8_t *buf = cell_cmd_get_rx_buffer();
			char *rmc = strstr((char *)buf, "RMC,");
			if (rmc != NULL) {
				DBG_OUT_RAW("NMEA: %s\r\n", buf);
				rmc += strlen("RMC,");
				/* time */
				int utc_time = atoi(rmc);
				gps_data_cur.hour = utc_time / 10000;
				gps_data_cur.min = (utc_time % 10000) / 100;
				gps_data_cur.sec = (utc_time % 10000) % 100;
				/* status */
				char *status = appUtils_getString_afterChar(rmc, ',');
				if (*status != 'A') {
					gps_data_cur.fixed_loc = false;
					break;
				}
				
				/* latitude */
				char *lat_str = appUtils_getString_afterChar(status, ',');
				if (lat_str == NULL) {
					break;
				}
				gps_data_cur.lat = cell_gps_convertLatLon_toFloat(lat_str);
				/* North/South */
				char *ns_str = appUtils_getString_afterChar(lat_str, ',');
				if ((*ns_str != 'N') && (*ns_str != 'S')) {
					break;
				}
				gps_data_cur.ns = *ns_str;
				/* longitude */
				char *lon_str = appUtils_getString_afterChar(ns_str, ',');
				if (lon_str == NULL) {
					break;
				}
				gps_data_cur.lon = cell_gps_convertLatLon_toFloat(lon_str);
				/* East/West */
				char *ew_str = appUtils_getString_afterChar(lon_str, ',');
				if ((*ew_str != 'E') && (*ew_str != 'W')) {
					break;
				}
				gps_data_cur.ew = *ew_str;

				/* Date */
				char *next = appUtils_getString_afterChar(ew_str, ',');
				next = appUtils_getString_afterChar(next, ',');
				next = appUtils_getString_afterChar(next, ',');
				int date_int = atoi(next);
				gps_data_cur.day   = date_int / 10000;
				gps_data_cur.month = (date_int % 10000) / 100;
				gps_data_cur.year  = (date_int % 10000) % 100 + 2000;

				gps_data_cur.fixed_loc 	= true;
				gps_data->fixed_loc 	= gps_data_cur.fixed_loc;
				gps_data->timestamp 	= gps_data_cur.timestamp;
				gps_data->day 			= gps_data_cur.day;
				gps_data->month 		= gps_data_cur.month;
				gps_data->year 			= gps_data_cur.year;
				gps_data->dow 			= gps_data_cur.dow;
				gps_data->hour 			= gps_data_cur.hour;
				gps_data->min 			= gps_data_cur.min;
				gps_data->sec 			= gps_data_cur.sec;
				gps_data->lat 			= gps_data_cur.lat;
				gps_data->lon 			= gps_data_cur.lon;
				gps_data->ns 			= gps_data_cur.ns;
				gps_data->ew 			= gps_data_cur.ew;
				gps_data->alt 			= gps_data_cur.alt;
				gps_data->fix 			= gps_data_cur.fix;
				break;
			}

			break;
		}
	}

    /* if fixed GPS then check hdop and satellite used */
    if (gps_data_cur.fixed_loc == true) {
        if ((cell_cmd_sendAT("AT+QGPSGNMEA=\"GGA\"\r\n", "OK\r\n", 2000) == true) == true) {
			// DBG_OUT_RAW(LOG_LVL_1, "NMEA: %s\r\n", g_gnss_stream_buf);
			/* check GGA */
			int buf_len = cell_cmd_get_rx_buffer_len();
			while (buf_len > 0) {
				uint8_t *buf = cell_cmd_get_rx_buffer();
				char *gga = strstr((char *)buf, "GGA,");
				if (gga != NULL) {
					DBG_OUT_RAW("NMEA: %s\r\n", buf);
					gga += strlen("GGA,");
					char *next = appUtils_getString_afterChar(gga, ','); // time
					next = appUtils_getString_afterChar(next, ','); // lat
					next = appUtils_getString_afterChar(next, ','); // NS
					next = appUtils_getString_afterChar(next, ','); // lon
					next = appUtils_getString_afterChar(next, ','); // EW
					next = appUtils_getString_afterChar(next, ','); // Quality
					gps_data_cur.sat = (uint8_t)appUtils_convertHexStringToU32(next, 2);
					next = appUtils_getString_afterChar(next, ',');
					gps_data_cur.hdo = atof(next);
					if (gps_data_cur.sat > 0) {
						gps_data->sat = gps_data_cur.sat;
						gps_data->hdo = gps_data_cur.hdo;
						FUNC_CELL_UNLOCK();
						return gps_data_cur.fixed_loc;
					}
				}
			}
		}
    }

    FUNC_CELL_UNLOCK();
    return false;
}


/*** STATIC FUNCTION ***/

/**
 * @brief convert latitude or longitude from NMEA message to correctly location data
 * 
 * @param data latitude or longitude string input
 * 
 * @return latitude or longitude output
 */
static float cell_gps_convertLatLon_toFloat(char *data)
{
	float value_f = 0;
	bool isNegative = false;
	if (data[0] == '-') {
		isNegative = true;
		value_f = atof(&data[1]) / 100;
	} else {
    	value_f = atof(data) / 100;
	}
    float value_decimal = (value_f - (uint32_t)value_f) * 100 / 60;
    float value = value_decimal;
    value += (uint32_t)value_f;
	if (isNegative == true) {
		value = -value;
	}
    return value;
}