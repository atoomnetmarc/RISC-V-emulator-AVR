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
#include "flash.h"
#include "spi.h"

void flash_init(void) {
#if (RVE_AVR_E_VERBOSE == 1)
    flash_jedec_id_u flash_jedec_id = {0};
    flash_read_jedec_id(&flash_jedec_id);

    printf_P(PSTR("JEDEC ID Manufacturer ID: 0x%02X\n"), flash_jedec_id.mfid);
    printf_P(PSTR("JEDEC ID Memory Type: 0x%02X\n"), flash_jedec_id.memory_type);
    printf_P(PSTR("JEDEC ID Capacity: 0x%02X\n"), flash_jedec_id.capacity);
#endif
}

void flash_read_jedec_id(flash_jedec_id_u *jedec_id) {
    spi1_cs_flash();

    spi1_transmit(FLASH_READ_JEDEC_ID);

    for (int8_t i = 0; i < sizeof(jedec_id->array); i++) {
        jedec_id->array[i] = spi1_receive();
    }

    spi1_cs_none();
}

void flash_read(const uint32_t address, const void *destination, uint16_t length) {
    spi1_cs_flash();

    flash_command_u command = {0};
    command.operation = FLASH_READ;
    command.address = address;

    for (int8_t i = sizeof(command.array) - 1; i >= 0; i--) {
        spi1_transmit(command.array[i]);
    }

    for (uint16_t i = 0; i < length; i++) {
        *(uint8_t *)(destination + i) = spi1_receive();
    }

    spi1_cs_none();
}

/**
 * Erase 4KB.
 */
void flash_sector_erase(const uint32_t address) {
    uint32_t maskedaddress = address & 0xFFF000;

    flash_write_enable();
    spi1_cs_flash();

    flash_command_u command = {0};
    command.operation = FLASH_SECTOR_ERASE;
    command.address = maskedaddress;

    for (int8_t i = sizeof(command.array) - 1; i >= 0; i--) {
        spi1_transmit(command.array[i]);
    }

    spi1_cs_none();
    while (flash_read_status() & 0x01)
        ;
}

void flash_write_enable(void) {
    spi1_cs_flash();
    spi1_transmit(FLASH_WRITE_ENABLE);
    spi1_cs_none();
}

void flash_write_disable(void) {
    spi1_cs_flash();
    spi1_transmit(FLASH_WRITE_DISABLE);
    spi1_cs_none();
}

uint8_t flash_read_status(void) {
    spi1_cs_flash();
    spi1_transmit(FLASH_READ_STATUS);
    uint8_t status;
    status = spi1_receive();
    spi1_cs_none();
    return status;
}

void flash_write(const uint32_t address, const void *source, uint16_t length) {
    uint32_t addresscounter = address;
    uint16_t source_offset = 0;
    while (length > 0) {
        uint16_t bytes_to_write = (length > 256) ? 256 : length;

        flash_write_enable();

        spi1_cs_flash();

        flash_command_u command = {0};
        command.operation = FLASH_PAGE_PROGRAM;
        command.address = addresscounter;

        for (int8_t i = sizeof(command.array) - 1; i >= 0; i--) {
            spi1_transmit(command.array[i]);
        }

        for (uint16_t i = 0; i < bytes_to_write; i++) {
            spi1_transmit(*(uint8_t *)(source + source_offset + i));
        }

        spi1_cs_none();

        while (flash_read_status() & 0x01)
            ;

        addresscounter += bytes_to_write;
        source_offset += bytes_to_write;
        length -= bytes_to_write;
    }
}