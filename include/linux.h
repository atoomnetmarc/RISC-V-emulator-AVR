/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef LINUX_H_
#define LINUX_H_

#include <stdbool.h>
#include <stdint.h>

#include <RiscvEmulatorType.h>

#include "vpi.h"

// Host hooks for the device library. Define them before including the device
// headers so the library uses the VPI-backed implementations.
#define RVE_DEV_UART16550_TX(byte)                      vpi_uart_tx(byte)
#define RVE_DEV_VIRTIO_CONSOLE_TX(buffer, length)       vpi_console_tx(buffer, length)
#define RVE_DEV_VIRTIO_BLK_READ(sector, buffer, count)  vpi_disk_read(sector, buffer, count)
#define RVE_DEV_VIRTIO_BLK_WRITE(sector, buffer, count) vpi_disk_write(sector, buffer, count)
#define RVE_DEV_VIRTIO_BLK_FLUSH()                      vpi_disk_flush()
#define RVE_DEV_VIRTIO_NET_TX(buffer, length)           vpi_net_tx(buffer, length)
#define RVE_DEV_VIRTIO_RNG_FILL(buffer, length)         vpi_rng_fill(buffer, length)
#define RVE_DEV_GOLDFISH_RTC_REALTIME_NS()              vpi_rtc_realtime_ns()
#define RVE_DEV_GOLDFISH_RTC_MONOTONIC_NS()             vpi_rtc_monotonic_ns()
#define RVE_DEV_SYSCON_POWEROFF(pass)                   vpi_exit((pass) ? VPI_EXIT_PASS : VPI_EXIT_FAIL)
#define RVE_DEV_SYSCON_REBOOT()                         vpi_exit(VPI_EXIT_REBOOT)

// Guest RAM lives in the simavr host, so the virtio transport must use VPI
// transfers instead of memcpy on a local pointer.
#define RVE_DEV_GUEST_READ(dev, address, destination, length) \
    vpi_load((address) - (dev)->memory_base, (destination), (uint16_t)(length))
#define RVE_DEV_GUEST_WRITE(dev, address, source, length) \
    vpi_store((address) - (dev)->memory_base, (source), (uint16_t)(length))

#include <RiscvEmulatorDevices.h>

// Device memory ranges. These match the DTS in the Native project.
#define SYSCON_ORIGIN         0x00100000u
#define SYSCON_SIZE           0x1000u
#define CLINT_ORIGIN          0x02000000u
#define CLINT_SIZE            0x10000u
#define UART_SIZE             0x100u
#define PLIC_ORIGIN           0x0C000000u
#define PLIC_SIZE             0x400000u
#define VIRTIO_BLK_ORIGIN     0x10001000u
#define VIRTIO_BLK_SIZE       0x1000u
#define VIRTIO_CONSOLE_ORIGIN 0x10002000u
#define VIRTIO_CONSOLE_SIZE   0x1000u
#define VIRTIO_NET_ORIGIN     0x10003000u
#define VIRTIO_NET_SIZE       0x1000u
#define VIRTIO_RNG_ORIGIN     0x10004000u
#define VIRTIO_RNG_SIZE       0x1000u
#define GOLDFISH_RTC_ORIGIN   0x10005000u
#define GOLDFISH_RTC_SIZE     0x1000u

extern syscon_t syscon;
extern clint_t clint;
extern uart16550_t uart;
extern plic_t plic;
extern virtio_blk_t virtio_blk;
extern virtio_console_t virtio_console;
extern virtio_net_t virtio_net;
extern virtio_rng_t virtio_rng;
extern goldfish_rtc_t goldfish_rtc;

/**
 * Initializes the platform devices and the guest disk capacity.
 */
void linux_init(void);

/**
 * Loads bytes from a platform device. Returns false when the address is not
 * mapped to a device.
 */
bool linux_load(uint32_t address, void *destination, uint8_t length);

/**
 * Stores bytes to a platform device. Returns false when the address is not
 * mapped to a device.
 */
bool linux_store(uint32_t address, const void *source, uint8_t length);

/**
 * Advances the platform devices and updates the pending interrupt bits.
 */
void linux_tick(RiscvEmulatorState_t *state);

/**
 * Returns the CLINT time source in microseconds.
 */
uint64_t linux_time_source(void);

/**
 * Starts the 10 ms Timer0 compare interrupt used to wake the AVR from idle.
 */
void linux_idle_init(void);

/**
 * Parks the AVR in idle sleep until an interrupt wakes it.
 */
void linux_idle(void);

#endif /* LINUX_H_ */
