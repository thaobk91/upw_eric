
#ifndef __CELL_GPS_H
#define __CELL_GPS_H

#include "cell_cmd.h"

typedef struct {
    bool fixed_loc;
    uint32_t timestamp;
    uint8_t day;    /* Date */
    uint8_t month;
    uint16_t year;
    uint8_t dow;    /* day of week */
    uint8_t hour;   /* Time */
    uint8_t min;
    uint8_t sec;
    float lat;      /* latitude */
    float lon;      /* longitude */
    char ns;        /* North/South */
    char ew;        /* East/West */
    float alt; /* altitude */
    uint8_t fix;    /* type of fix, 2-2D, 3-3D */
    uint8_t sat;    /* number of satellite in view */
    float hdo;      /* HDOP - Horizontal dilution of precision */
} gps_data_t;

/**
 * @brief Enable GPS
 * 
 * @param None
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_gps_on(void);

/**
 * @brief Disable GPS
 * 
 * @param None
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_gps_off(void);

/**
 * @brief Enable/Disable GPS Auto Start
 * 
 * @param enable true to enable, false to disable
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_gps_set_auto_start(bool enable);

/**
 * @brief Get location
 * 
 * @param gps_data GPS data structure
 * 
 * @retval true if success
 * @retval false if failure
 */
bool cell_gps_get_location(gps_data_t *gps_data);

#endif // __CELL_GPS_H