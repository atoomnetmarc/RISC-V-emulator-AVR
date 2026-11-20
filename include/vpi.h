/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef VPI_H_
#define VPI_H_

#include <stdbool.h>
#include <stdint.h>

#include <avr/io.h>

// VPI IO registers. _SFR_IO8 takes the IO address; the simavr host uses the
// memory address, which is the IO address plus 0x20.
#define VPI_ADDR_LO  _SFR_IO8(0x11)  // 0x31
#define VPI_ADDR_MID _SFR_IO8(0x12)  // 0x32
#define VPI_ADDR_HI  _SFR_IO8(0x13)  // 0x33
#define VPI_DATA     _SFR_IO8(0x14)  // 0x34
#define VPI_ADDR_X   _SFR_IO8(0x16)  // 0x36
#define VPI_CALL     _SFR_IO8(0x18)  // 0x38
#define VPI_ARGRES   _SFR_IO8(0x1A)  // 0x3A
#define VPI_STATUS   _SFR_IO8(0x1C)  // 0x3C
#define VPI_SPACE    _SFR_IO8(0x1D)  // 0x3D
#define VPI_RX_LEN   _SFR_IO8(0x1E)  // 0x3E
#define VPI_RX_DATA  _SFR_IO8(0x1F)  // 0x3F
#define VPI_EXIT     _SFR_IO8(0x19)  // 0x39

#define VPI_EXIT_PASS   0
#define VPI_EXIT_FAIL   1
#define VPI_EXIT_REBOOT 2

#define VPI_STATUS_IDLE  0
#define VPI_STATUS_DONE  1
#define VPI_STATUS_ERROR 2

#define VPI_SPACE_RAM     0
#define VPI_SPACE_SCRATCH 1

#define VPI_CALL_UART_TX       1
#define VPI_CALL_CONSOLE_TX    2
#define VPI_CALL_DISK_READ     3
#define VPI_CALL_DISK_WRITE    4
#define VPI_CALL_DISK_FLUSH    5
#define VPI_CALL_NET_TX        6
#define VPI_CALL_RNG_FILL      7
#define VPI_CALL_RTC_REALTIME  8
#define VPI_CALL_RTC_MONOTONIC 9
#define VPI_CALL_TIME_SOURCE   10
#define VPI_CALL_DISK_CAPACITY 11

#define VPI_SCRATCH_SIZE 4096

/**
 * Reads length bytes from the VPI-backed guest RAM at the given offset.
 */
void vpi_load(uint32_t offset, void *destination, uint16_t length);

/**
 * Writes length bytes to the VPI-backed guest RAM at the given offset.
 */
void vpi_store(uint32_t offset, const void *source, uint16_t length);

/**
 * Transmits one byte on the host console.
 */
void vpi_uart_tx(uint8_t byte);

/**
 * Transmits a buffer on the host console.
 */
void vpi_console_tx(const void *buffer, uint16_t length);

/**
 * Reads count sectors from the host disk into buffer. Returns true on success.
 */
bool vpi_disk_read(uint64_t sector, void *buffer, uint8_t count);

/**
 * Writes count sectors from buffer to the host disk. Returns true on success.
 */
bool vpi_disk_write(uint64_t sector, const void *buffer, uint8_t count);

/**
 * Flushes the host disk. Returns true on success.
 */
bool vpi_disk_flush(void);

/**
 * Transmits one Ethernet frame to the host TAP interface.
 */
void vpi_net_tx(const void *buffer, uint16_t length);

/**
 * Fills a buffer with host entropy.
 */
void vpi_rng_fill(void *buffer, uint16_t length);

/**
 * Returns the host realtime clock in nanoseconds.
 */
uint64_t vpi_rtc_realtime_ns(void);

/**
 * Returns the host monotonic clock in nanoseconds.
 */
uint64_t vpi_rtc_monotonic_ns(void);

/**
 * Returns the CLINT time source in microseconds.
 */
uint64_t vpi_time_source(void);

/**
 * Returns the host disk capacity in 512-byte sectors.
 */
uint64_t vpi_disk_capacity(void);

/**
 * Stops the host simulation with the given exit status.
 */
void vpi_exit(uint8_t status);

/**
 * Returns the number of bytes available in the VPI receive buffer.
 */
uint8_t vpi_rx_available(void);

/**
 * Reads one byte from the VPI receive buffer.
 */
uint8_t vpi_rx_read(void);

#endif /* VPI_H_ */
