/**
 * @file lcd.h
 * @author Michael Platzer
 * @date 2018-11-29
 *
 * @brief Pretty print output data to the SparkFun 20x4 SerLCD module
 */

#ifndef LCD_H
#define LCD_H

#include <stdint.h>

#include "uart.h"

/**
 * @brief Clear the display.
 */
static inline int lcd_clear()
{
    return fputs("|-", &uartout) != EOF ? 0 : -1;
}

#define LCD_BACKLIGHT_MAX 29

/**
 * @brief Set the backlight of the display.
 *
 * The backlight color can be specified using @p r for the red, @p g for the
 * green and @b for the blue component.
 */
static inline int lcd_backlight(uint8_t r, uint8_t g, uint8_t b)
{
    if (r > 30 || g > 30 || b > 30)
        return -1;
    char buf[7] = "|r|g|b";
    buf[1] = 128 + r;
    buf[3] = 158 + g;
    buf[5] = 188 + b;
    return fputs(buf, &uartout) != EOF ? 0 : -1;
}

/**
 * @brief Write a line to the display.
 *
 * This function writes one line to the display (maximum 20 characters).
 * Newline characters are ignored. Call this function again to write to the
 * next line of the display. Use @c lcd_clear() to clear the display.
 */
static inline int lcd_write(const char *line)
{
    char buf[21];
    int i;
    for (i = 0; i < 20 && line[i] != 0; i++)
        buf[i] = line[i];
    for (; i < 20; i++)
        buf[i] = ' ';
    buf[20] = 0;
    return fputs(buf, &uartout) != EOF ? 0 : -1;
}

/**
 * @brief Print a formatted line to the display.
 */
#define lcd_printf(...) {                                                      \
    char line[21];                                                             \
    snprintf(line, sizeof(line), __VA_ARGS__);                                 \
    lcd_write(line);                                                           \
}

#endif // LCD_H
