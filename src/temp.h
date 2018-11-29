/**
 * @file temp.h
 * @author Michael Platzer
 * @date 2018-11-29
 *
 * @brief Interface for the MAX31855K Thermocouple-to-Digital Converter
 */

#ifndef TEMP_H
#define TEMP_H

/**
 * @brief Initialize temperature readings.
 *
 * This function sets up the connection to the MAX31855K.
 */
void temp_init();

/**
 * @brief Read the temperature of the thermocouple.
 *
 * This function reads the temperatures of the hot junction and the cold
 * junction of the thermocouple from the MAX31855K. The temperature of the
 * hot junction is copied into the variable pointed to by @p hj_temp_ptr and
 * the temperature of the cold junction is copied into the variable pointed
 * to by @p cj_temp_ptr .
 *
 * Either one of these pointers may be NULL, in which case it is not used.
 *
 * The temperatures are given as fixed point values: the value for the
 * hot junction has 2 bits after the radix point, the value for the cold
 * junction has 4 bits after the radix point. Use the macros below for
 * converting these values into a human readable string.
 *
 * The functions returns 0 on success and -1 in case of an error.
 */
int temp_read(int *hj_temp_ptr, int *cj_temp_ptr);


/**
 * @brief Write a hot junction temperature to a string buffer.
 */
#define TEMP4_TO_STR(str, temp)  {                                             \
    if ((temp) >= 0)                                                           \
        sprintf(str, "%d.%02d", (temp) >> 2, ((temp) & 3) * 25);               \
    else {                                                                     \
        int abs_temp = -(temp);                                                \
        sprintf(str, "-%d.%02d", abs_temp >> 2, (abs_temp & 3) * 25);          \
    }                                                                          \
}

/**
 * @brief Write a cold junction temperature to a string buffer.
 */
#define TEMP16_TO_STR(str, temp) {                                             \
    if ((temp) >= 0)                                                           \
        sprintf(str, "%d.%04d", (temp) >> 4, ((temp) & 0xf) * 625);            \
    else {                                                                     \
        int abs_temp = -(temp);                                                \
        sprintf(str, "-%d.%04d", abs_temp >> 4, (abs_temp & 0xf) * 625);       \
    }                                                                          \
}

#endif // TEMP_H
