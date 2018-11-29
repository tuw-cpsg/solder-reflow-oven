/**
 * @file oven.c
 * @author Michael Platzer
 * @date 2018-11-29
 *
 * @brief User interface and state control
 */

#include "temp.h"
#include "reflow.h"
#include "pid.h"

#ifdef USE_LCD_DISP
#include "lcd.h"
#else
#include "uart.h"
#endif

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/**
 * @brief Initialize the PWM output to steer the heater elements.
 */
void pwm_init()
{
    PORTE.DIRSET = 8;

    // set period to 15360 (2 MHz, prescaler: 256 => period: 1.966 s)
    TCE0.PERL = (15 * 1024) & 0xff;
    TCE0.PERH = (15 * 1024) >> 8;

    // initiate duty cycle to 0
    TCE0.CCDL = 0;
    TCE0.CCDH = 0;

    TCE0.CTRLB = 0x83; // enable CCD, activate single-slope PWM waveform generation
    TCE0.CTRLA = 6; // prescaler: 256
}

/**
 * @brief Initialize the update timer.
 */
void timer_init()
{
    // set period to 1 s (2 MHz, prescaler: 64 => period: 31250)
    TCC0.PERL = 31250 & 0xff;
    TCC0.PERH = 31250 >> 8;

    TCC0.INTCTRLA = 2; // enable overflow interrupt with level 2
    TCC0.CTRLA = 5; // prescaler: 64
}

/**
 * @brief Restart the update timer.
 */
void timer_restart()
{
    TCC0.CTRLFSET = 8; // write restart command to F register
    TCC0.CTRLFCLR = 0xc; // clear command bits in F register

    TCC0.INTFLAGS = 1; // clear overflow interrupt (in case one is pending)
}

// Temperature constants for the cold junction (IC temperature, multiply by 16):
#define IC_OVERHEAT (40 * 16)   // temperature at which the IC is overheated (40 deg C)

// Temperature constants for the hot junction (oven temperature, multiply by 4):
#define OVEN_COOL   (50 * 4)    // temperature below which it is safe to open the oven (50 deg C)
#define BAKE_TEMP   (125 * 4)

static volatile int mode = 0; // oven mode; 0: idle, 1: reflow, 2: bake, 3: cool
static pid_state_t pid_state;
static int bake_time = 0;

#ifdef USE_LCD_DISPLAY
static int lcd_blink = 0;
#endif

/**
 * @brief Update timer interrupt routine.
 */
ISR(TCC0_OVF_vect)
{
    char temp_buf[4][16];

    int ic_temp, oven_temp;

    if (temp_read(&oven_temp, &ic_temp) < 0) {
#ifdef USE_LCD_DISP
        lcd_blink = (lcd_blink == 0) ? LCD_BACKLIGHT_MAX : 0;
        lcd_clear();
        lcd_backlight(lcd_blink, 0, 0);
        lcd_printf("MAX31855 ERROR");
#else
        fprintf(&uartout, "ERROR: MAX31855 reports a fault condition\r\n");
#endif
        return;
    }

    int32_t pid_out;
    int out;

    if (ic_temp >= IC_OVERHEAT) {
#ifdef USE_LCD_DISP
        lcd_blink = (lcd_blink == 0) ? LCD_BACKLIGHT_MAX : 0;
        lcd_clear();
        lcd_backlight(lcd_blink, 0, 0);
        lcd_printf("!!! OVERHEATED !!!");
        lcd_printf("Controller to hot!");
        lcd_printf("Temp: %3d.%04d degC", ic_temp >> 4, (ic_temp & 0xf) * 625);
        lcd_printf("HEATER OFF");
#else
        fprintf(&uartout, "OVERHEATED: IC TEMP: %d.%04d\n", ic_temp >> 4, (ic_temp & 0xf) * 625);
#endif
        out = 0;
    } else
        switch (mode) {
            case 0: // idle
#ifdef USE_LCD_DISP
                lcd_clear();
                lcd_backlight(0, 0, 0);
                lcd_printf("Solder Reflow Oven");
                lcd_printf("READY");
#endif
                out = 0;
                break;

            case 1: // reflow
                out = reflow_update(oven_temp);

                if (out == 0 && oven_temp < OVEN_COOL) {
                    mode = 0;
                    PORTE.OUTCLR = 4;
                }

                break;

            case 2: // bake
                pid_out = pid_update(&pid_state, BAKE_TEMP - oven_temp);
                bake_time++;

                // limit out to [0,8191]
                pid_out += 4096L;
                if (pid_out < 0L)
                    pid_out = 0L;
                if (pid_out > 8191L)
                    pid_out = 8191L;

                // reduce out to 16 values
                out = pid_out >> 9;

                TEMP4_TO_STR(temp_buf[0], oven_temp);
                TEMP4_TO_STR(temp_buf[1], pid_state.last_diff);
                TEMP4_TO_STR(temp_buf[2], pid_state.integ);
#ifdef USE_LCD_DISP
                lcd_clear();
                lcd_backlight(LCD_BACKLIGHT_MAX, LCD_BACKLIGHT_MAX, 0);
                lcd_printf("BAKING    %2d:%02d:%02d", bake_time / 3600, (bake_time / 60) % 60, bake_time % 60);
                lcd_printf("Temp: %3d.%02d degC", temp_buf[0]);
                lcd_printf("D: %s I: %s", temp_buf[1], temp_buf[2]);
                lcd_printf("Heater: %d", out);
#else
                fprintf(&uartout, "%s\tdiff: %s\tinteg: %s\tout: %d\n", temp_buf[0], temp_buf[1], temp_buf[2], out);
#endif
                //fprintf(&uartout, "%3d.%02d\tdiff: %d.%02d\tinteg: %d.%02d\tout: %d\n", oven_temp >> 2, (oven_temp & 3) * 25,
                //        pid_state.last_diff >> 2, (pid_state.last_diff & 3) * 25, pid_state.integ >> 2, (pid_state.integ & 3) * 25, out);

                out *= 1024;
                break;

            case 3: // cooling (after baking)
                out = 0;
                PORTE.OUTTGL = 4;

#ifdef USE_LCD_DISP
                lcd_clear();
                lcd_backlight(0, 0, LCD_BACKLIGHT_MAX);
                lcd_printf("COOLING");
                lcd_printf("Temp: %d.%02d degC", oven_temp >> 2, (oven_temp & 3) * 25);
#endif

                if (oven_temp < OVEN_COOL) {
                    mode = 0;
                    PORTE.OUTCLR = 4;
                }
                break;
        }

    TCE0.CCDBUFL = out & 0xff;
    TCE0.CCDBUFH = out >> 8;
}

/**
 * @brief Set up an interrupt for the start/stop button.
 */
void button_init()
{
    PORTD.INT0MASK = 1; // add pin 0 to port interrupt 0
    PORTD.PIN0CTRL = 2; // sense falling edge

    PORTD.INTCTRL = 2; // activate port interrupt 0 with level 2
}

/**
 * @brief Start/stop button interrupt routine.
 */
ISR(PORTD_INT0_vect)
{
    if (mode == 0) {
        if (!(PORTD.IN & 2)) {
            mode = 1;
            reflow_start();
        } else {
            mode = 2;
            pid_state = pid_init(160, 1, 0);
            bake_time = 0;
        }

        PORTE.OUTSET = 4;
        timer_restart();
    }
    else if (mode == 2 && bake_time > 1) {
        mode = 3;

        PORTE.OUTCLR = 4;
        timer_restart();
    }
}

/**
 * @brief Program entry point.
 */
int main()
{
    uart_init();
#ifdef USE_LCD_DISP
    lcd_init();
#endif

    temp_init();

    PORTE.DIRSET = 4;
    PORTE.OUTCLR = 4;

    pwm_init();
    timer_init();
    button_init();

    // enable interrupts:
    PMIC.CTRL = 0x02;
    sei();

    while (1);
    return 0;
}
