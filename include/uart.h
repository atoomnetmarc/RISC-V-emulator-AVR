/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdint.h>
#include <stdio.h>

#ifndef UART_H_
#define UART_H_

#include <stdbool.h>

#define UART_FIFO_BUFFER_SIZE 16

typedef struct {
    uint8_t buffer[UART_FIFO_BUFFER_SIZE];
    uint8_t write_index;
    uint8_t read_index;
} uart_fifo_t;

/**
 * Union of all the ways the UART line status register can be accessed.
 *
 * The bit layout matches the 16550 LSR. The transmitter empty bits are set
 * permanently (output is instantaneous) and the data-ready bit is set
 * whenever the receive FIFO is not empty.
 */
typedef union {
    uint8_t value;
    struct {
        /**
         * Data ready: a received byte is available in the receive FIFO.
         */
        uint8_t dataReady : 1;

        /**
         * Overrun error. Never set.
         */
        uint8_t overrunError : 1;

        /**
         * Parity error. Never set.
         */
        uint8_t parityError : 1;

        /**
         * Framing error. Never set.
         */
        uint8_t framingError : 1;

        /**
         * Break interrupt. Never set.
         */
        uint8_t breakInterrupt : 1;

        /**
         * Transmitter holding register empty. Always set.
         */
        uint8_t transmitterHoldingEmpty : 1;

        /**
         * Transmitter shift register empty. Always set.
         */
        uint8_t transmitterShiftEmpty : 1;
    } bits;
} uart_line_status_register_t;

void uart_init(void);
int uart_putc(char character, FILE *stream);
int uart_getc(FILE *stream);
bool uart_rx_ready(void);
uint8_t uart_rx_char_nonblocking(void);

#endif
