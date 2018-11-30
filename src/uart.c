/**
 * @file uart.c
 * @author Michael Platzer
 * @date 2018-11-29
 *
 * @brief UART serial output
 */
#include "uart.h"

#include <avr/io.h>

void uart_init()
{
    // set pin D7 high and as output (TX pin)
    PORTD.OUTSET = 0x80;
    PORTD.DIRSET = 0x80;

    // target baud rate: 9600, 2 MHz clock => BSCALE = 0, BSEL = 12
    USARTD1.BAUDCTRLB = 0;
    USARTD1.BAUDCTRLA = 13;

    USARTD1.CTRLA = 0; // disable interrupts
    USARTD1.CTRLC = 3; // async, no parity, 8 bit data, 1 stop bit

    USARTD1.CTRLB = 8; // enable transmitter
}

static int uart_putc(char c, FILE *f)
{
    while (!(USARTD1.STATUS & 0x20)); // wait for data register empty flag
    USARTD1.DATA = c; // write data
    return 0;
}

FILE uartout = FDEV_SETUP_STREAM(uart_putc, NULL, _FDEV_SETUP_WRITE);
