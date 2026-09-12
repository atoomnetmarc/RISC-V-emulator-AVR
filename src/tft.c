/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdint.h>

#include <avr/io.h>
#include <util/delay.h>

#include "defines.h"
#include "spi.h"

#if RVE_AVR_E_TFT == 1

void tft_reset(uint8_t level) {
    if (level) {
        PORTC |= _BV(PC7);
    } else {
        PORTC &= ~_BV(PC7);
    }
}

void tft_dc(uint8_t level) {
    if (level) {
        PORTC |= _BV(PC6);
    } else {
        PORTC &= ~_BV(PC6);
    }
}

void tft_bl(uint8_t level) {
    if (level) {
        PORTB |= _BV(PB3);
    } else {
        PORTB &= ~_BV(PB3);
    }
}

void tft_send_command(uint8_t cmd) {
    spi1_cs_tft();
    tft_dc(0);
    spi1_transmit(cmd);
    spi1_cs_none();
}

void tft_send_data_start() {
    spi1_cs_tft();
    tft_dc(1);
}

void tft_send_data_end() {
    spi1_cs_none();
}

void tft_send_data(uint8_t data) {
    tft_send_data_start();
    spi1_transmit(data);
    tft_send_data_end();
}

/**
 * ST7735S green tab TFT
 */
void tft_init() {

    // BL
    tft_bl(0);
    DDRB |= _BV(PB3);

    // DC, RST
    DDRC |= _BV(PC6) | _BV(PC7);

    tft_reset(0);
    _delay_ms(50);
    tft_reset(1);
    _delay_ms(150);

    tft_send_command(0x11); // Sleep off
    _delay_ms(500);

    // Frame rate control
    tft_send_command(0xB1); // Frame rate control (Normal mode)
    tft_send_data(0x01);    // DIVA=01h
    tft_send_data(0x2C);    // VPA=2Ch
    tft_send_data(0x2D);    // VPA=2Dh

    // Power control
    tft_send_command(0xC0); // Power Control 1
    tft_send_data(0xA2);    // AVDD=4.6V
    tft_send_data(0x02);    // VRHP=+2.5V
    tft_send_data(0x84);    // VRHN=-4.5V

    tft_send_command(0xC1); // Power Control 2
    tft_send_data(0xC5);    // VGH=4.5V, VGL=-4.5V

    tft_send_command(0xC2); // Power Control 3 (In normal mode/Full colors)
    tft_send_data(0x0A);    // OpAmp current small
    tft_send_data(0x00);    // Boost frequency

    // VCOM control
    tft_send_command(0xC5); // VCOM control
    tft_send_data(0x0E);    // VCOMH voltage
    tft_send_data(0x0A);    // VCOML voltage

    // Memory Data Access Control
    tft_send_command(0x36); // Memory data access control
    tft_send_data(0xC8);    // MX, MY, RGB mode

    // Color mode - set to 16-bit color (RGB565)
    tft_send_command(0x3A); // Interface pixel format
    tft_send_data(0x05);    // 16-bit/pixel RGB565

    // Gamma correction
    tft_send_command(0xE0); // Positive Gamma correction
    tft_send_data(0x0F);
    tft_send_data(0x1A);
    tft_send_data(0x0F);
    tft_send_data(0x18);
    tft_send_data(0x2F);
    tft_send_data(0x28);
    tft_send_data(0x20);
    tft_send_data(0x22);
    tft_send_data(0x1F);
    tft_send_data(0x1B);
    tft_send_data(0x23);
    tft_send_data(0x37);
    tft_send_data(0x00);
    tft_send_data(0x07);
    tft_send_data(0x02);
    tft_send_data(0x10);

    tft_send_command(0xE1); // Negative Gamma correction
    tft_send_data(0x0F);
    tft_send_data(0x1B);
    tft_send_data(0x0F);
    tft_send_data(0x17);
    tft_send_data(0x33);
    tft_send_data(0x2C);
    tft_send_data(0x29);
    tft_send_data(0x2E);
    tft_send_data(0x30);
    tft_send_data(0x30);
    tft_send_data(0x39);
    tft_send_data(0x3F);
    tft_send_data(0x00);
    tft_send_data(0x07);
    tft_send_data(0x03);
    tft_send_data(0x10);

    // Display ON
    tft_send_command(0x29);
    _delay_ms(100);

    // Backlight on
    tft_bl(1);
}

void tft_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    tft_send_command(0x2A);   // Column Address Set
    tft_send_data(x0 >> 8);   // MSB van x0
    tft_send_data(x0 & 0xFF); // LSB van x0
    tft_send_data(x1 >> 8);   // MSB van x1
    tft_send_data(x1 & 0xFF); // LSB van x1

    tft_send_command(0x2B);   // Row Address Set
    tft_send_data(y0 >> 8);   // MSB van y0
    tft_send_data(y0 & 0xFF); // LSB van y0
    tft_send_data(y1 >> 8);   // MSB van y1
    tft_send_data(y1 & 0xFF); // LSB van y1

    tft_send_command(0x2C); // Write to RAM
}

void tft_set_pixel(uint16_t x, uint16_t y, uint16_t color) {

    tft_set_address_window(x, y, x, y);

    tft_send_data(color >> 8);   // MSB (RGB565)
    tft_send_data(color & 0xFF); // LSB
}

void tft_clear(uint16_t color) {
    tft_set_address_window(0, 0, 127, 159);
    tft_send_data_start();

    for (uint16_t i = 0; i < 128 * 160; i++) {
        spi1_transmit(color >> 8);
        spi1_transmit(color & 0xFF);
    }
    tft_send_data_end();
}

#endif