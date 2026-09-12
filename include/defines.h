/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <avr/io.h>

#ifndef DEFINES_H_
#define DEFINES_H_

// Offsets of the 16550-style UART registers within the UART region.
#define UART_RECEIVE_BUFFER_OFFSET 0
#define UART_LINE_STATUS_OFFSET    5

#if (RVE_AVR_E_ACT == 1)
// ACT mode runs under simavr with a VPI-backed RAM; no SPI peripherals.
// The test binary is loaded at RAM_ORIGIN by the simavr wrapper.
#define RVE_AVR_E_START_ADDRESS RAM_ORIGIN
#define RVE_AVR_E_RAM_LENGTH    0x1000000 // 16 MiB, same as the VPI buffer
#ifndef RVE_AVR_E_VERBOSE
#define RVE_AVR_E_VERBOSE 1
#endif

#ifndef RVE_AVR_E_FATFS
#define RVE_AVR_E_FATFS 0
#endif

#ifndef RVE_AVR_E_TFT
#define RVE_AVR_E_TFT 0
#endif
#endif

// Set features based on flash size
#if FLASHEND > 8192
#ifndef RVE_AVR_E_VERBOSE
#define RVE_AVR_E_VERBOSE 1
#endif

#ifndef RVE_AVR_E_FATFS
#define RVE_AVR_E_FATFS 1
#endif

#ifndef RVE_AVR_E_TFT
#define RVE_AVR_E_TFT 1
#endif
#endif

// Feature defaults
#ifndef RVE_AVR_E_START_ADDRESS
#define RVE_AVR_E_START_ADDRESS 0x800000
#endif

#ifndef RVE_AVR_E_RAM_LENGTH
#define RVE_AVR_E_RAM_LENGTH 0x800000 // 8 MiB PSRAM
#endif
#ifndef RVE_AVR_E_VERBOSE
#define RVE_AVR_E_VERBOSE 0
#endif

#ifndef RVE_AVR_E_FATFS
#define RVE_AVR_E_FATFS 0
#endif

#ifndef RVE_AVR_E_TFT
#define RVE_AVR_E_TFT 0
#endif

#endif
