/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#if (RVE_AVR_LINUX == 1)

#include "linux.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <string.h>

#include "defines.h"

syscon_t syscon;
clint_t clint;
uart16550_t uart;
plic_t plic;
virtio_blk_t virtio_blk;
virtio_console_t virtio_console;
virtio_net_t virtio_net;
virtio_rng_t virtio_rng;
goldfish_rtc_t goldfish_rtc;

static const uint8_t net_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

// 20 MHz / 1024 / (194 + 1) = 100.16 Hz, close enough to a 10 ms wake tick.
#define IDLE_TICK_OCR0A 194u

// Pause host input before the 16-byte guest FIFO fills, and resume with
// hysteresis so a single byte does not toggle the signal on every tick.
#define UART_FLOW_XOFF_LEVEL 12u
#define UART_FLOW_XON_LEVEL  4u

static bool uart_flow_paused;

void linux_init(void) {
    syscon_init(&syscon, SYSCON_ORIGIN, SYSCON_SIZE);
    clint_init(&clint, CLINT_ORIGIN, CLINT_SIZE);
    clint.mtimecmp = UINT64_MAX;
    uart16550_init(&uart, UART_ORIGIN, UART_SIZE);
    plic_init(&plic, PLIC_ORIGIN, PLIC_SIZE);
    virtio_blk_init(&virtio_blk, VIRTIO_BLK_ORIGIN, VIRTIO_BLK_SIZE, NULL,
                    RAM_ORIGIN, RVE_AVR_RAM_LENGTH, vpi_disk_capacity());
    virtio_console_init(&virtio_console, VIRTIO_CONSOLE_ORIGIN, VIRTIO_CONSOLE_SIZE,
                        NULL, RAM_ORIGIN, RVE_AVR_RAM_LENGTH);
    virtio_net_init(&virtio_net, VIRTIO_NET_ORIGIN, VIRTIO_NET_SIZE, NULL,
                    RAM_ORIGIN, RVE_AVR_RAM_LENGTH, net_mac);
    virtio_rng_init(&virtio_rng, VIRTIO_RNG_ORIGIN, VIRTIO_RNG_SIZE, NULL,
                    RAM_ORIGIN, RVE_AVR_RAM_LENGTH);
    goldfish_rtc_init(&goldfish_rtc, GOLDFISH_RTC_ORIGIN, GOLDFISH_RTC_SIZE);
    linux_idle_init();
}

bool linux_load(uint32_t address, void *destination, uint8_t length) {
    if (clint_read(&clint, address, destination, length)) {
        return true;
    }
    if (uart16550_read(&uart, address, destination, length)) {
        return true;
    }
    if (plic_read(&plic, address, destination, length)) {
        return true;
    }
    if (virtio_blk_read(&virtio_blk, address, destination, length)) {
        return true;
    }
    if (virtio_console_read(&virtio_console, address, destination, length)) {
        return true;
    }
    if (virtio_net_read(&virtio_net, address, destination, length)) {
        return true;
    }
    if (virtio_rng_read(&virtio_rng, address, destination, length)) {
        return true;
    }
    if (goldfish_rtc_read(&goldfish_rtc, address, destination, length)) {
        return true;
    }
    if (syscon_read(&syscon, address, destination, length)) {
        return true;
    }
    return false;
}

bool linux_store(uint32_t address, const void *source, uint8_t length) {
    if (clint_write(&clint, address, source, length)) {
        return true;
    }
    if (uart16550_write(&uart, address, source, length)) {
        return true;
    }
    if (plic_write(&plic, address, source, length)) {
        return true;
    }
    if (virtio_blk_write(&virtio_blk, address, source, length)) {
        return true;
    }
    if (virtio_console_write(&virtio_console, address, source, length)) {
        return true;
    }
    if (virtio_net_write(&virtio_net, address, source, length)) {
        return true;
    }
    if (virtio_rng_write(&virtio_rng, address, source, length)) {
        return true;
    }
    if (goldfish_rtc_write(&goldfish_rtc, address, source, length)) {
        return true;
    }
    if (syscon_write(&syscon, address, source, length)) {
        return true;
    }
    return false;
}

void linux_tick(RiscvEmulatorState_t *state) {
    while (vpi_rx_available() != 0) {
        uint8_t byte = vpi_rx_read();
        uart16550_rx_push(&uart, byte);
        virtio_console_rx_push(&virtio_console, byte);
    }

    if (!uart_flow_paused && uart.fifo_count >= UART_FLOW_XOFF_LEVEL) {
        uart_flow_paused = true;
        vpi_uart_flow(1);
    } else if (uart_flow_paused && uart.fifo_count <= UART_FLOW_XON_LEVEL) {
        uart_flow_paused = false;
        vpi_uart_flow(0);
    }

    clint_tick(&clint, vpi_time_source());

    plic_set_pending(&plic, 1, virtio_blk_irq_pending(&virtio_blk));
    plic_set_pending(&plic, 2, virtio_console_irq_pending(&virtio_console));
    plic_set_pending(&plic, 3, virtio_net_irq_pending(&virtio_net));
    plic_set_pending(&plic, 4, uart16550_irq_pending(&uart));
    plic_set_pending(&plic, 5, virtio_rng_irq_pending(&virtio_rng));

    if (clint_timer_pending(&clint)) {
        state->csr.mip_platform |= (1u << 7);
    } else {
        state->csr.mip_platform &= ~(1u << 7);
    }
    if (clint_software_pending(&clint)) {
        state->csr.mip_platform |= (1u << 3);
    } else {
        state->csr.mip_platform &= ~(1u << 3);
    }
    if (plic_irq_pending(&plic, 0)) {
        state->csr.mip_platform |= (1u << 11);
    } else {
        state->csr.mip_platform &= ~(1u << 11);
    }
    if (plic_irq_pending(&plic, 1)) {
        state->csr.mip_platform |= (1u << 9);
    } else {
        state->csr.mip_platform &= ~(1u << 9);
    }
}

uint64_t linux_time_source(void) {
    return clint.mtime;
}

void linux_idle_init(void) {
    TCCR0A = _BV(WGM01);
    OCR0A = IDLE_TICK_OCR0A;
    TCCR0B = _BV(CS02) | _BV(CS00);
    TIMSK0 |= _BV(OCIE0A);
}

ISR(TIMER0_COMPA_vect) {
}

void linux_idle(void) {
    sleep_enable();
    sleep_cpu();
    sleep_disable();
}

#endif
