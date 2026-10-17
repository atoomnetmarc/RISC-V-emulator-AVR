/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "defines.h"
#include "led.h"
#include "uart.h"

volatile uart_fifo_t tx_fifo = {0};
volatile uart_fifo_t rx_fifo = {0};

static inline void enable_udrie(void) {
#if (defined(UCR))
    UCR |= _BV(UDRIE);
#elif (defined(UCSRB))
    UCSRB |= _BV(UDRIE);
#elif (defined(UCSR0B))
    UCSR0B |= _BV(UDRIE0);
#else
#error "Unknown AVR model."
#endif
}

static inline void disable_udrie(void) {
#if (defined(UCR))
    UCR &= ~_BV(UDRIE);
#elif (defined(UCSRB))
    UCSRB &= ~_BV(UDRIE);
#elif (defined(UCSR0B))
    UCSR0B &= ~_BV(UDRIE0);
#else
#error "Unknown AVR model."
#endif
}

#if (defined(UART_UDRE_vect))
ISR(UART_UDRE_vect)
#elif (defined(USART_UDRE_vect))
ISR(USART_UDRE_vect)
#elif (defined(USART0_UDRE_vect))
ISR(USART0_UDRE_vect)
#else
#error "Unknown AVR model."
#endif
{
    if (tx_fifo.read_index != tx_fifo.write_index) {
#if (defined(UDR))
        UDR = tx_fifo.buffer[tx_fifo.read_index];
#elif (defined(UDR0))
        UDR0 = tx_fifo.buffer[tx_fifo.read_index];
#else
#error "Unknown AVR model."
#endif
        tx_fifo.read_index = (tx_fifo.read_index + 1) % sizeof(tx_fifo.buffer);
    } else {
        disable_udrie();
    }
}

int uart_putc(char character, FILE *stream) {
    uint8_t index_next = (tx_fifo.write_index + 1) % sizeof(tx_fifo.buffer);

    // Wait for room in tx buffer.
    while (index_next == tx_fifo.read_index) {
    }

    tx_fifo.buffer[tx_fifo.write_index] = character;
    tx_fifo.write_index = index_next;

    enable_udrie();

    return 0;
}

#if (defined(UART_RX_vect))
ISR(UART_RX_vect)
#elif (defined(USART_RX_vect))
ISR(USART_RX_vect)
#elif (defined(USART_RXC_vect))
ISR(USART_RXC_vect)
#elif (defined(USART0_RX_vect))
ISR(USART0_RX_vect)
#else
#error "Unknown AVR model."
#endif
{
#if (defined(UDR))
    uint8_t data = UDR;
#elif (defined(UDR0))
    uint8_t data = UDR0;
#else
#error "Unknown AVR model."
#endif
    uint8_t index_next = (rx_fifo.write_index + 1) % sizeof(rx_fifo.buffer);

    if (index_next != rx_fifo.read_index) {
        rx_fifo.buffer[rx_fifo.write_index] = data;
        rx_fifo.write_index = index_next;
    } else {
        // Buffer overflow
    }
}

int uart_getc(FILE *stream) {
    // Wait for input.
    while (!uart_rx_ready()) {
    }

    return uart_rx_char_nonblocking();
}

bool uart_rx_ready(void) {
    return rx_fifo.read_index != rx_fifo.write_index;
}

uint8_t uart_rx_char_nonblocking(void) {
    if (!uart_rx_ready()) {
        return 0;
    }

    uint8_t data = rx_fifo.buffer[rx_fifo.read_index];
    rx_fifo.read_index = (rx_fifo.read_index + 1) % sizeof(rx_fifo.buffer);

    return data;
}

FILE uart_str = FDEV_SETUP_STREAM(uart_putc, uart_getc, _FDEV_SETUP_RW);

/**
 * Initialize UART.
 */
void uart_init(void) {

#define BAUD 38400
#include <util/setbaud.h>

#if (defined(__AVR_AT90S4434__) || \
     defined(__AVR_AT90S8535__))
    UBRR = UBRR_VALUE;
    UCR |= _BV(RXCIE) | _BV(TXEN) | _BV(RXEN);
#elif (defined(__AVR_ATmega163__))
    UBRR = UBRRL_VALUE;
    UBRRHI = UBRRH_VALUE;

#if USE_2X
    UCSRA |= _BV(U2X);
#endif

    UCSRB |= _BV(RXCIE) | _BV(TXEN) | _BV(RXEN);
#elif (defined(__AVR_ATmega8535__) || \
       defined(__AVR_ATmega323__) ||  \
       defined(__AVR_ATmega16__) ||   \
       defined(__AVR_ATmega16A__) ||  \
       defined(__AVR_ATmega32__) ||   \
       defined(__AVR_ATmega32A__))
    UBRRL = UBRRL_VALUE;
    UBRRH = UBRRH_VALUE;

#if USE_2X
    UCSRA |= _BV(U2X);
#endif

    UCSRB = _BV(TXEN) | _BV(RXEN);
#elif (defined(__AVR_ATmega164__) ||   \
       defined(__AVR_ATmega164A__) ||  \
       defined(__AVR_ATmega164P__) ||  \
       defined(__AVR_ATmega164PA__) || \
       defined(__AVR_ATmega324__) ||   \
       defined(__AVR_ATmega324A__) ||  \
       defined(__AVR_ATmega324PA__) || \
       defined(__AVR_ATmega644__) ||   \
       defined(__AVR_ATmega644A__) ||  \
       defined(__AVR_ATmega644P__) ||  \
       defined(__AVR_ATmega644PA__) || \
       defined(__AVR_ATmega1284__) ||  \
       defined(__AVR_ATmega1284P__))
    UBRR0 = UBRR_VALUE;

#if USE_2X
    UCSR0A |= _BV(U2X0);
#endif

    UCSR0B |= _BV(RXCIE0) | _BV(TXEN0) | _BV(RXEN0);
#else
#error "Unknown AVR model."
#endif

    stdin = &uart_str;
    stdout = &uart_str;
}
