/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#if (RVE_AVR_E_LINUX == 1)

#include "vpi.h"

#include <string.h>

/**
 * Sets the 32-bit VPI address.
 */
static void vpi_set_address(uint32_t address) {
    VPI_ADDR_LO = (uint8_t)address;
    VPI_ADDR_MID = (uint8_t)(address >> 8u);
    VPI_ADDR_HI = (uint8_t)(address >> 16u);
    VPI_ADDR_X = (uint8_t)(address >> 24u);
}

/**
 * Appends one argument byte to the current host call.
 */
static void vpi_arg_u8(uint8_t value) {
    VPI_ARGRES = value;
}

/**
 * Appends a little-endian 16-bit argument to the current host call.
 */
static void vpi_arg_u16(uint16_t value) {
    vpi_arg_u8((uint8_t)value);
    vpi_arg_u8((uint8_t)(value >> 8u));
}

/**
 * Appends a little-endian 64-bit argument to the current host call.
 */
static void vpi_arg_u64(uint64_t value) {
    for (uint8_t i = 0; i < 8; i++) {
        vpi_arg_u8((uint8_t)(value >> (8u * i)));
    }
}

/**
 * Reads one result byte from the completed host call.
 */
static uint8_t vpi_result_u8(void) {
    return VPI_ARGRES;
}

/**
 * Reads a little-endian 64-bit result from the completed host call.
 */
static uint64_t vpi_result_u64(void) {
    uint64_t value = 0;
    for (uint8_t i = 0; i < 8; i++) {
        value |= (uint64_t)vpi_result_u8() << (8u * i);
    }
    return value;
}

/**
 * Copies a buffer into the host scratch space.
 */
static void vpi_scratch_write(const void *buffer, uint16_t length) {
    const uint8_t *bytes = (const uint8_t *)buffer;
    VPI_SPACE = VPI_SPACE_SCRATCH;
    vpi_set_address(0);
    for (uint16_t i = 0; i < length; i++) {
        VPI_DATA = bytes[i];
    }
    VPI_SPACE = VPI_SPACE_RAM;
}

/**
 * Copies a buffer out of the host scratch space.
 */
static void vpi_scratch_read(void *buffer, uint16_t length) {
    uint8_t *bytes = (uint8_t *)buffer;
    VPI_SPACE = VPI_SPACE_SCRATCH;
    vpi_set_address(0);
    for (uint16_t i = 0; i < length; i++) {
        bytes[i] = VPI_DATA;
    }
    VPI_SPACE = VPI_SPACE_RAM;
}

uint8_t vpi_snapshot_request(void) {
    return VPI_SNAPSHOT;
}

void vpi_snapshot_acknowledge(void) {
    VPI_SNAPSHOT = 0;
}

void vpi_state_saved(void) {
    VPI_CALL = VPI_CALL_STATE_SAVE;
}

void vpi_load(uint32_t offset, void *destination, uint16_t length) {
    uint8_t *bytes = (uint8_t *)destination;
    vpi_set_address(offset);
    for (uint16_t i = 0; i < length; i++) {
        bytes[i] = VPI_DATA;
    }
}

void vpi_store(uint32_t offset, const void *source, uint16_t length) {
    const uint8_t *bytes = (const uint8_t *)source;
    vpi_set_address(offset);
    for (uint16_t i = 0; i < length; i++) {
        VPI_DATA = bytes[i];
    }
}

void vpi_uart_tx(uint8_t byte) {
    vpi_arg_u8(byte);
    VPI_CALL = VPI_CALL_UART_TX;
}

void vpi_console_tx(const void *buffer, uint16_t length) {
    vpi_scratch_write(buffer, length);
    vpi_arg_u16(length);
    VPI_CALL = VPI_CALL_CONSOLE_TX;
}

bool vpi_disk_read(uint64_t sector, void *buffer, uint8_t count) {
    vpi_arg_u64(sector);
    vpi_arg_u8(count);
    VPI_CALL = VPI_CALL_DISK_READ;
    if (VPI_STATUS != VPI_STATUS_DONE || vpi_result_u8() == 0) {
        return false;
    }
    vpi_scratch_read(buffer, (uint16_t)count * 512u);
    return true;
}

bool vpi_disk_write(uint64_t sector, const void *buffer, uint8_t count) {
    vpi_scratch_write(buffer, (uint16_t)count * 512u);
    vpi_arg_u64(sector);
    vpi_arg_u8(count);
    VPI_CALL = VPI_CALL_DISK_WRITE;
    return VPI_STATUS == VPI_STATUS_DONE && vpi_result_u8() != 0;
}

bool vpi_disk_flush(void) {
    VPI_CALL = VPI_CALL_DISK_FLUSH;
    return VPI_STATUS == VPI_STATUS_DONE && vpi_result_u8() != 0;
}

void vpi_net_tx(const void *buffer, uint16_t length) {
    vpi_scratch_write(buffer, length);
    vpi_arg_u16(length);
    VPI_CALL = VPI_CALL_NET_TX;
}

void vpi_rng_fill(void *buffer, uint16_t length) {
    vpi_arg_u16(length);
    VPI_CALL = VPI_CALL_RNG_FILL;
    vpi_scratch_read(buffer, length);
}

uint64_t vpi_rtc_realtime_ns(void) {
    VPI_CALL = VPI_CALL_RTC_REALTIME;
    return vpi_result_u64();
}

uint64_t vpi_rtc_monotonic_ns(void) {
    VPI_CALL = VPI_CALL_RTC_MONOTONIC;
    return vpi_result_u64();
}

// The AVR executes roughly 2000 RISC-V instructions per second, three orders
// of magnitude slower than Native. Linux envs slow CLINT time to match so the
// guest kernel does not spend all its instructions servicing timer ticks.
#ifndef RVE_AVR_TIME_SCALE
#define RVE_AVR_TIME_SCALE 1u
#endif

uint64_t vpi_time_source(void) {
    VPI_CALL = VPI_CALL_TIME_SOURCE;
    return vpi_result_u64() / RVE_AVR_TIME_SCALE;
}

uint64_t vpi_disk_capacity(void) {
    VPI_CALL = VPI_CALL_DISK_CAPACITY;
    return vpi_result_u64();
}

void vpi_exit(uint8_t status) {
    VPI_EXIT = status;
}

uint8_t vpi_rx_available(void) {
    return VPI_RX_LEN;
}

uint8_t vpi_rx_read(void) {
    return VPI_RX_DATA;
}

#endif
