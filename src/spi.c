/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <avr/io.h>

#include "spi.h"

/**
 * Initialize 2 SPI ports.
 */
inline void spi_init(void) {
    // SPI1
    // Chipselect: SD, flash, TFT
    spi1_cs_none();
    DDRA |= _BV(PA5) | _BV(PA6) | _BV(PA7);

    // MOSI, SCK
    DDRB |= _BV(PB5) | _BV(PB7);
    PORTB &= ~_BV(PB7);

    // MISO
    DDRB &= ~_BV(PB6);

    SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR0);

    // SPI2
    // Chipselect: RAM
    spi2_cs_none();
    DDRB |= _BV(PB1);

    // MOSI, SCK
    DDRD |= _BV(PD3) | _BV(PD4);
    PORTD &= ~_BV(PD4);

    // MISO
    DDRD &= ~_BV(PD2);

    // Enable buffers
    DDRB |= _BV(PB4);
    PORTB &= ~_BV(PB4);
}

inline void spi1_cs_none(void) {
    PORTA |= _BV(PA5) | _BV(PA6) | _BV(PA7);
}

inline void spi1_cs_sd(void) {
    PORTA &= ~_BV(PA5);
}

inline void spi1_cs_flash(void) {
    PORTA &= ~_BV(PA6);
}

inline void spi1_cs_tft(void) {
    PORTA &= ~_BV(PA7);
}

inline uint8_t spi1_receive(void) {
    // Must be 0xFF for sd-card.
    SPDR = 0xFF;
    loop_until_bit_is_set(SPSR, SPIF);
    return SPDR;
}

inline void spi1_transmit(uint8_t byte) {
    SPDR = byte;
    loop_until_bit_is_set(SPSR, SPIF);
}

inline void spi2_cs_none() {
    PORTB |= _BV(PB1);
}

inline void spi2_cs_ram() {
    PORTB &= ~_BV(PB1);
}

// SPI2 transfer 1 byte MODE0
void spi2_transfer(uint8_t *byte) {
    for (int8_t bit = 7; bit >= 0; bit--) {
        // MOSI
        if (*byte & _BV(bit)) {
            PORTD |= _BV(PD3);
        } else {
            PORTD &= ~_BV(PD3);
        }

        // SCK=1
        PORTD |= _BV(PD4);

        // MISO
        if (PIND & _BV(PD2)) {
            *byte |= _BV(bit);
        } else {
            *byte &= ~_BV(bit);
        }

        // SCK=0
        PORTD &= ~_BV(PD4);
    }
}