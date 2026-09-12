/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdint.h>
#include <stdio.h>

#include <avr/pgmspace.h>

#include "defines.h"
#include "psram.h"
#include "spi.h"

inline void psram_init(void) {
    psram_reset();

#if (RVE_AVR_E_VERBOSE == 1)
    psram_eid_u eid = {0};
    psram_read_eid(&eid);

    printf_P(PSTR("psram ID: 0x"));
    for (uint8_t i = 0; i < sizeof(eid.array); i++) {
        printf_P(PSTR("%02X"), eid.array[i]);
    }
    printf_P(PSTR("\n"));

    printf_P(PSTR("psram EID mfid: 0x%02X\n"), eid.mfid);
    printf_P(PSTR("psram EID kgd: 0x%02X\n"), eid.kgd);
    printf_P(PSTR("psram EID eid: 0x"));
    for (uint8_t i = 0; i < sizeof(eid.eid); i++) {
        printf_P(PSTR("%02X"), eid.eid[i]);
    }
    printf_P(PSTR("\n"));
    printf_P(PSTR("psram EID eid density: 0x%1X\n"), (eid.eid[5] & 0b11100000) >> 5);
#endif
}

inline void psram_reset(void) {
    spi2_cs_ram();

    uint8_t reset[] = {PSRAM_RESET_ENABLE, PSRAM_RESET};

    for (int8_t i = 0; i < sizeof(reset); i++) {
        spi2_transfer(&reset[i]);
    }

    spi2_cs_none();
}

void psram_read_eid(psram_eid_u *eid) {
    spi2_cs_ram();

    psram_command_u command = {0};
    command.operation = PSRAM_READ_ID;

    for (int8_t i = sizeof(command.array) - 1; i >= 0; i--) {
        spi2_transfer(&command.array[i]);
    }

    for (int8_t i = sizeof(eid->array) - 1; i >= 0; i--) {
        uint8_t data = 0x00;
        spi2_transfer(&data);
        eid->array[i] = data;
    }

    spi2_cs_none();
}

void psram_read(uint32_t address, const void *destination, uint8_t length) {
    spi2_cs_ram();

    psram_command_u command = {0};
    command.address = address;
    command.operation = PSRAM_READ;

    for (int8_t i = sizeof(command.array) - 1; i >= 0; i--) {
        spi2_transfer(&command.array[i]);
    }

    for (uint8_t i = 0; i < length; i++) {
        uint8_t data = 0x00;
        spi2_transfer(&data);
        *(uint8_t *)(destination + i) = data;
    }

    spi2_cs_none();
}

void psram_write(uint32_t address, const void *source, uint8_t length) {
    spi2_cs_ram();

    psram_command_u command = {0};
    command.address = address;
    command.operation = PSRAM_WRITE;

    for (int8_t i = sizeof(command.array) - 1; i >= 0; i--) {
        spi2_transfer(&command.array[i]);
    }

    for (uint8_t i = 0; i < length; i++) {
        uint8_t data = *(uint8_t *)(source + i);
        spi2_transfer(&data);
    }

    spi2_cs_none();
}
