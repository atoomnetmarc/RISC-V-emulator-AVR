/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ACT_H_
#define ACT_H_

#include <stdbool.h>
#include <stdint.h>

#if (RVE_AVR_E_ACT == 1)

// The RISC-V address the ACT test writes its result to.
#define RVMODEL_HALT_ADDRESS 0x02000000
#define RVMODEL_HALT_PASS    123456789u
#define RVMODEL_HALT_FAIL    1u

/**
 * Prints the ACT mode banner (verbose builds only).
 */
void act_init(void);

/**
 * Reads length bytes from the VPI-backed RAM at the given offset
 * (RISC-V address - RAM_ORIGIN).
 */
void act_load(uint32_t offset, void *destination, uint8_t length);

/**
 * Writes length bytes to the VPI-backed RAM at the given offset
 * (RISC-V address - RAM_ORIGIN).
 */
void act_store(uint32_t offset, const void *source, uint8_t length);

/**
 * Handles a store to the halt region (IO_ORIGIN) or to the ROM region
 * (nothing is mapped there in ACT mode, so such a store can only be a
 * halt attempt at a non-AVR halt address). Writes the exit status to the
 * VPI EXIT register (0x00 = pass, 0x01 = fail) and never returns.
 *
 * @return true if the store was a halt store, false otherwise.
 */
bool act_halt(uint32_t address, const void *source, uint8_t length);

/**
 * Counts one executed RISC-V instruction.
 */
void act_count_instruction(void);

#endif

#endif
