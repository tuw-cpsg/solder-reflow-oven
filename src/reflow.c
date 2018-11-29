/**
 * @file reflow.c
 * @author Michael Platzer
 * @date 2018-11-29
 *
 * @brief Manage the process of reflow soldering a circuit board
 */
#include "reflow.h"

#include "pid.h"

#ifdef USE_LCD_DISP
#include "lcd.h"
#else
#include "uart.h"
#endif

static int phase, t;

void reflow_start()
{
    phase = 0;
    t = 0;
}

#define TIMEOUT         (12 * 60)

#define OUT_100_PERCENT (15 * 1024)

// Temperature constants (multiplied by 4):
#define TEMP_SOAK_MIN   (100 * 4)
#define TEMP_SOAK_SET   (120 * 4)
#define TEMP_SOAK_MAX   (150 * 4)
#define TEMP_LIQUIDUS   (183 * 4)
#define TEMP_PEAK       (235 * 4)
#define TEMP_OFF        (TEMP_PEAK - 10)

int reflow_update(int temp)
{
    // recorded time-stamps:
    static int t_soak, // start of soak phase (time when min soak temp reached)
               t_ramp, // start of ramp (time when max soak temp exceeded)
               t_liqu, // start of liquidus phase (time when liquidus temp exceeded)
               t_off,  // time when the heater is turned off previous to peak
               t_peak, // start of peak phase (time when peak temp exceeded)
               t_chill, // end of peak phase (time when falling below peak temp)
               t_cool; // end of critical phase (time when falling below liquidus temp)

    static pid_state_t pid_state;

#ifdef USE_LCD_DISP
    int lcd_blink = ((t & 1)) ? LCD_BACKLIGHT_MAX : 0;
    lcd_clear();
    lcd_printf("REFLOW MODE  %4d'", t++);
    lcd_printf("Temp: %3d.%02d degC", temp >> 2, (temp & 3) * 25);
#else
    fprintf(&uartout, "%4d\t%3d.%02d\t", t++, temp >> 2, (temp & 3) * 25);
#endif

    // turn heater off upon timeout
    if (t >= TIMEOUT) {
#ifdef USE_LCD_DISP
        lcd_printf("TIMEOUT!");
        lcd_backlight(lcd_blink, 0, 0);
#else
        fprintf(&uartout, " TIMEOUT\n");
#endif
        return 0;
    }

    switch (phase) {
        case 0:
            if (temp < TEMP_SOAK_MIN) {
#ifdef USE_LCD_DISP
                lcd_printf("PREHEAT");
                lcd_backlight(LCD_BACKLIGHT_MAX, 0, LCD_BACKLIGHT_MAX);
#else
                fprintf(&uartout, " PREHEAT\n");
#endif
                return OUT_100_PERCENT;
            }
            phase = 1;
            t_soak = t;
            pid_state = pid_init(160, 1, 0);

        case 1:
            if (t < t_soak + 120) {
                int out = pid_update(&pid_state, TEMP_SOAK_SET - temp);

                // limit out to [0,8191]
                out += 4096;
                if (out < 0)
                    out = 0;
                if (out > 8191)
                    out = 8191;

                // reduce out to 16 values
                out >>= 9;

#ifdef USE_LCD_DISP
                lcd_printf("SOAK PHASE, D: %4d", pid_state.last_diff);
                lcd_printf("I: %4d => O: %2d", pid_state.integ, out);
                lcd_backlight(LCD_BACKLIGHT_MAX, 0, LCD_BACKLIGHT_MAX);
#else
                fprintf(&uartout, "diff: %d, integ: %d => %d\n", pid_state.last_diff, pid_state.integ, out);
#endif
                return out * 1024;
            }
            phase = 2;

        case 2:
            if (temp < TEMP_SOAK_MAX) {
#ifdef USE_LCD_DISP
                lcd_printf("RAMPING UP");
                lcd_backlight(LCD_BACKLIGHT_MAX, LCD_BACKLIGHT_MAX / 2, 0);
#else
                fprintf(&uartout, " RAMPING UP\n");
#endif
                return OUT_100_PERCENT;
            }
            phase = 3;
            t_ramp = t;

#ifdef USE_LCD_DISP
#else
            fprintf(&uartout, " total soak time: %d s;", t_ramp - t_soak);
#endif

        case 3:
            if (temp < TEMP_LIQUIDUS) {
#ifdef USE_LCD_DISP
                lcd_printf("RAMPING UP");
                lcd_printf("LIQUIDUS TEMP");
                lcd_backlight(LCD_BACKLIGHT_MAX, LCD_BACKLIGHT_MAX / 2, 0);
#else
                fprintf(&uartout, " RAMPING UP\n");
#endif
                return OUT_100_PERCENT;
            }
            phase = 4;
            t_liqu = t;

        case 4:
            if (temp < TEMP_OFF) {
#ifdef USE_LCD_DISP
                lcd_printf("LIQUIDUS PHASE");
                lcd_backlight(LCD_BACKLIGHT_MAX, LCD_BACKLIGHT_MAX / 2, 0);
#else
                fprintf(&uartout, " LIQUIDUS PHASE\n");
#endif
                return OUT_100_PERCENT;
            }
            phase = 5;
            t_off = t;

        case 5:
            if (temp < TEMP_PEAK && t < t_off + 5) {
#ifdef USE_LCD_DISP
                lcd_printf("LIQUIDUS PHASE");
                lcd_printf("HEATER OFF");
                lcd_backlight(lcd_blink, lcd_blink, lcd_blink);
#else
                fprintf(&uartout, " LIQUIDUS PHASE, HEATER OFF\n");
#endif
                return 0;
            }
            phase = 6;
            t_peak = t;

        case 6:
            if (temp >= TEMP_PEAK) {
#ifdef USE_LCD_DISP
                lcd_printf("PEAK");
                lcd_printf("HEATER OFF");
                lcd_backlight(lcd_blink, lcd_blink, lcd_blink);
#else
                fprintf(&uartout, " PEAK\n");
#endif
                return 0;
            }
            phase = 7;
            t_chill = t;

        case 7:
            if (temp >= TEMP_LIQUIDUS) {
#ifdef USE_LCD_DISP
                lcd_printf("CHILLING");
                lcd_backlight(0, LCD_BACKLIGHT_MAX, LCD_BACKLIGHT_MAX);
#else
                fprintf(&uartout, " CHILLING\n");
#endif
                return 0;
            }
            phase = 8;
            t_chill = t;

#ifdef USE_LCD_DISP
#else
            fprintf(&uartout, " total liquidus time: %d s;", t_chill - t_liqu);
#endif

        case 8:
#ifdef USE_LCD_DISP
            lcd_printf("COOL DOWN");
            lcd_printf("Liquidus time: %3d'", t_chill - t_liqu);
            lcd_backlight(0, 0, LCD_BACKLIGHT_MAX);
#else
            fprintf(&uartout, " COOL DOWN\n");
#endif
            return 0;
    }
    return 0;
}
