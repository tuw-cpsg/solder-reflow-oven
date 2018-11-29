/**
 * @file temp.c
 * @author Michael Platzer
 * @date 2018-11-29
 *
 * @brief Interface for the MAX31855K Thermocouple-to-Digital Converter
 */
#include "temp.h"

#include <stddef.h>
#include <avr/io.h>

void temp_init()
{
    // set pin C1 high and as output (CS pin)
    PORTC.OUTSET = 2;
    PORTC.DIRSET = 2;

    // set pin C7 low and as output (SCK pin)
    PORTC.OUTCLR = 0x80;
    PORTC.DIRSET = 0x80;

    SPIC.INTCTRL = 0; // disable interrupts
    SPIC.CTRL = 0x50; // master spi in mode 0, 500 kHz (clk / 4), msb first
}

int temp_read(int *hj_temp_ptr, int *cj_temp_ptr)
{
    uint32_t data;

    PORTC.OUTCLR = 2; // CS low
    int i;
    for (i = 0; i < 4; i++) {
        SPIC.DATA = 0x5a; // dummy byte to start transmission
        while (!(SPIC.STATUS & 0x80)); // wait for completion
        data = (data << 8) | SPIC.DATA; // read byte
    }
    PORTC.OUTSET = 2; // CS high

    // check error bit (reports a fault condition on the MAX31855):
    if ((data & (((uint32_t)1)<<16)))
        return -1;

    int16_t hj_temp = data >> 16;
    hj_temp >>= 2; // sign extend

    if (hj_temp_ptr != NULL)
        *hj_temp_ptr = hj_temp;

    int16_t cj_temp = data;
    cj_temp >>= 4;

    if (cj_temp_ptr != NULL)
        *cj_temp_ptr = cj_temp;

    return 0;
}
