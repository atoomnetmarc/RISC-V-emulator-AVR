/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdint.h>

#ifndef SPI_H_
#define SPI_H_

#if (defined(SPI2X0))
// Some models like the ATMega164PA, ATMega324A, ATMega324P and ATMega324PA have 0 appended for an unknown reason. Lets define aliases.
#define SPCR SPCR0
#define SPR0 SPR00
#define SPR1 SPR10
#define CPHA CPHA0
#define CPOL CPOL0
#define MSTR MSTR0
#define DORD DORD0
#define SPE  SPE0
#define SPIE SPIE0

#define SPSR  SPSR0
#define SPI2X SPI2X0
#define WCOL  WCOL0
#define SPIF  SPIF0

#define SPDR SPDR0
#endif

void spi_init(void);
uint8_t spi1_receive(void);
void spi1_transmit(uint8_t byte);
void spi1_cs_none(void);
void spi1_cs_sd(void);
void spi1_cs_flash(void);
void spi1_cs_tft(void);

void spi2_transfer(uint8_t *byte);
void spi2_cs_none(void);
void spi2_cs_ram(void);

#endif
