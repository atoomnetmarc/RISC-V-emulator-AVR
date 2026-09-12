/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdarg.h>
#include <stdio.h>

#include <avr/pgmspace.h>

#include <RiscvEmulatorDefine.h>
#include <RiscvEmulatorType.h>

#include "defines.h"

#if (RVE_AVR_E_ACT == 1)
#include "act.h"
#include "uart.h"
#else
#include "flash.h"
#include "psram.h"
#include "uart.h"
#endif

#ifndef RiscvEmulatorImplementationSpecific_H_
#define RiscvEmulatorImplementationSpecific_H_

/**
 * Store all disassembly format strings in flash (zero RAM cost). Included
 * before RiscvEmulatorDisasm.h in every translation unit, so the library's
 * plain default never takes effect.
 */
#define RVE_DISASM_FMT(s) PSTR(s)

#if (RVE_AVR_E_ACT == 1) && (RVE_E_DISASM == 1)
/**
 * ACT mode instruction tracing. The library renders each instruction via the
 * shape functions; this override sends the fragments (format strings stored
 * in flash via RVE_DISASM_FMT_PSTR) to the UART.
 */
#define RVE_DISASM_PRINTF_OVERRIDE 1
void RiscvEmulatorDisasmPrintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf_P(stdout, fmt, ap);
    va_end(ap);
}
#endif

/**
 * Loads bytes from emulator to RISC-V.
 *
 * @param address The byte address in memory.
 * @param destination The destination address to copy the data to.
 * @param length The length in bytes of the data.
 */
extern inline void RiscvEmulatorLoad(uint32_t address, void *destination, uint8_t length) {
    if (address >= RAM_ORIGIN) {
#if (RVE_AVR_E_ACT == 1)
        act_load(address - RAM_ORIGIN, destination, length);
#else
        psram_read(address - RAM_ORIGIN, destination, length);
#endif
        return;
    }

    if (address >= ROM_ORIGIN) {
#if (RVE_AVR_E_ACT == 0)
        flash_read(address - ROM_ORIGIN, destination, length);
#endif
        return;
    }

    if (address >= UART_ORIGIN) {
        uint32_t offset = address - UART_ORIGIN;
        if (offset == UART_RECEIVE_BUFFER_OFFSET && length == 1) {
            *(uint8_t *)destination = uart_rx_char_nonblocking();
        } else if (offset == UART_LINE_STATUS_OFFSET && length == 1) {
            uart_line_status_register_t lineStatus = {
                .bits = {
                    .dataReady = uart_rx_ready(),
                    .transmitterHoldingEmpty = 1,
                    .transmitterShiftEmpty = 1,
                },
            };
            *(uint8_t *)destination = lineStatus.value;
        }
        return;
    }

    if (address >= IO_ORIGIN) {
        uint32_t ioaddress = address - IO_ORIGIN;
        if (length == 1) {
            *(uint8_t *)destination = _SFR_IO8(ioaddress);
        } else if (length == 2) {
            *(uint16_t *)destination = _SFR_IO16(ioaddress);
        }
        return;
    }
}

/**
 * Stores bytes from RISC-V to emulator.
 *
 * @param address The byte address in memory.
 * @param destination The destination address to copy the data to.
 * @param length The length in bytes of the data.
 */
extern inline void RiscvEmulatorStore(uint32_t address, const void *source, uint8_t length) {
    if (address >= RAM_ORIGIN) {
#if (RVE_AVR_E_ACT == 1)
        act_store(address - RAM_ORIGIN, source, length);
#else
        psram_write(address - RAM_ORIGIN, source, length);
#endif
        return;
    }

    if (address >= ROM_ORIGIN) {
#if (RVE_AVR_E_ACT == 1)
        // Nothing is mapped in the ROM region in ACT mode; a store there is
        // a halt attempt at a non-AVR halt address.
        if (act_halt(address, source, length)) {
            return;
        }
#endif
#if (RVE_AVR_E_VERBOSE == 1)
        printf_P(PSTR("There was a write to ROM. ROM cannot be written. Stop.\n"));
#endif
        for (;;) {
            asm volatile("BREAK");
        };
        return;
    }

    if (address >= UART_ORIGIN) {
        // The UART data register is one byte wide; a wider store transmits
        // its lowest byte only.
        if (address == UART_ORIGIN) {
            uart_putc(*(uint8_t *)source, NULL);
        }
        return;
    }

    if (address >= IO_ORIGIN) {
        uint32_t ioaddress = address - IO_ORIGIN;
#if (RVE_AVR_E_ACT == 1)
        if (act_halt(address, source, length)) {
            return;
        }
#endif
        if (length == 1) {
            _SFR_IO8(ioaddress) = *(uint8_t *)source;
        } else if (length == 2) {
            _SFR_IO16(ioaddress) = *(uint16_t *)source;
        }
        return;
    }
}

/**
 * Handles a fault where the instruction is somehow not recognized.
 *
 * Something wrong happened at the current programcounter.
 * Use a listing file of the risc-v program to better understand the wrong.
 * The failed machine instruction is found in state.instruction.value.
 */
extern inline void RiscvEmulatorIllegalInstruction(RiscvEmulatorState_t *state) {
#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("Illegal instruction 0x%08" PRIX32 " at 0x%08" PRIX32 ". Stop.\n"),
             state->instruction.value,
             state->programcounter);
#endif

    for (;;) {
        asm volatile("BREAK");
    };
}

/**
 * Handles a fault where the CSR is not recognized.
 */
#if (RVE_E_ZICSR == 1)
extern inline void RiscvEmulatorUnknownCSR(RiscvEmulatorState_t *state) {
#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("Unknown CSR 0x%04" PRIX16 " at 0x%08" PRIX32 ". Stop.\n"),
             state->instruction.itypecsr.csr,
             state->programcounter);
#endif

    for (;;) {
        asm volatile("BREAK");
    };
}
#endif

/**
 * Handles an ECALL.
 */
extern inline void RiscvEmulatorHandleECALL(RiscvEmulatorState_t *state) {
#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("ECALL at 0x%08" PRIX32 ". Stop.\n"),
             state->programcounter);
#endif

    for (;;) {
        asm volatile("BREAK");
    };
}

/**
 * Handles an EBREAK.
 */
extern inline void RiscvEmulatorHandleEBREAK(RiscvEmulatorState_t *state) {
#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("BREAK at 0x%08" PRIX32 ". Stop.\n"),
             state->programcounter);
#endif

    for (;;) {
        asm volatile("BREAK");
    };
}

#endif
