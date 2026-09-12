/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdint.h>

#include "defines.h"

#ifndef TFT_H_
#define TFT_H_

#if RVE_AVR_E_TFT == 1

void tft_bl(uint8_t level);
void tft_clear(uint16_t color);
void tft_init(void);
void tft_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void tft_set_pixel(uint16_t x, uint16_t y, uint16_t color);

#endif
#endif