/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "act.h"

#if (RVE_AVR_E_ACT == 1)

#include <stdio.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <RiscvEmulatorDefine.h>

#include "defines.h"
#include "uart.h"

// VPI peripheral registers (simavr custom IO module).
// Memory-mapped IO addresses 0x31/0x32/0x33/0x34/0x39;
// _SFR_IO8(n) takes the IO address (memory address - 0x20).
#define VPI_ADDR_LO  _SFR_IO8(0x11) // IO 0x31: bits 0-7 of the 24-bit RAM offset
#define VPI_ADDR_MID _SFR_IO8(0x12) // IO 0x32: bits 8-15
#define VPI_ADDR_HI  _SFR_IO8(0x13) // IO 0x33: bits 16-23
#define VPI_DATA     _SFR_IO8(0x14) // IO 0x34: data byte, auto-increments the offset
#define VPI_EXIT     _SFR_IO8(0x19) // IO 0x39: 0x00 = pass, 0x01 = fail, stops simavr

uint32_t RiscvEmulatorInstructionCounter;

void act_init(void) {
    // Undefined exit status: the host treats 0xFF as "no halt store happened".
    VPI_EXIT = 0xFF;

#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("ACT mode: VPI-backed RAM at 0x%08lX\n"), (unsigned long)RAM_ORIGIN);
#endif
}

static void act_set_offset(uint32_t offset) {
    VPI_ADDR_LO = offset & 0xFF;
    VPI_ADDR_MID = (offset >> 8) & 0xFF;
    VPI_ADDR_HI = (offset >> 16) & 0xFF;
}

void act_load(uint32_t offset, void *destination, uint8_t length) {
    act_set_offset(offset);
    for (uint8_t i = 0; i < length; i++) {
        *(uint8_t *)(destination + i) = VPI_DATA;
    }
}

void act_store(uint32_t offset, const void *source, uint8_t length) {
    act_set_offset(offset);
    for (uint8_t i = 0; i < length; i++) {
        VPI_DATA = *(uint8_t *)(source + i);
    }
}

bool act_halt(uint32_t address, const void *source, uint8_t length) {
    // The halt region is IO_ORIGIN (0x02000000). Additionally, any store to
    // the ROM region is treated as a halt attempt: in ACT mode nothing is
    // mapped there (all code and data live in the VPI-backed RAM), so a
    // store there can only be a test halting at a non-AVR halt address.
    if (address != RVMODEL_HALT_ADDRESS && address < ROM_ORIGIN) {
        return false;
    }

    // Halt region: the test writes its result here.
    uint32_t value = 0;
    for (uint8_t i = 0; i < length && i < 4; i++) {
        value |= (uint32_t)*(uint8_t *)(source + i) << (8 * i);
    }

#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("Halt: result 0x%08lX after %lu instructions\n"),
             (unsigned long)value,
             (unsigned long)RiscvEmulatorInstructionCounter);
#endif

    VPI_EXIT = (value == RVMODEL_HALT_PASS) ? 0x00 : 0x01;
    for (;;) {
        asm volatile("BREAK");
    }
}

void act_count_instruction(void) {
    RiscvEmulatorInstructionCounter++;
}

#endif
