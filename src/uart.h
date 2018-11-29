/**
 * @file uart.h
 * @author Michael Platzer
 * @date 2018-11-29
 *
 * @brief UART serial output
 */

#ifndef UART_H
#define UART_H

#include <stdio.h>

/**
 * @brief Initialize the serial output.
 *
 * This function initializes the serial output.
 */
void uart_init();

/**
 * @brief Serial file object.
 *
 * Use this file object to write data to the serial output.
 */
extern FILE uartout;

#endif // UART_H
