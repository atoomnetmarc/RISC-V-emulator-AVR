/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <avr/io.h>

void led_init(void) {
    DDRB |= _BV(PB0);
}

void led_on(void) {
    PORTB |= _BV(PB0);
}

void led_off(void) {
    PORTB &= ~_BV(PB0);
}