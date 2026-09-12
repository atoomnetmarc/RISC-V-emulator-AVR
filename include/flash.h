/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdint.h>

#ifndef FLASH_H_
#define FLASH_H_

#define FLASH_SECTOR_ERASE  0x20
#define FLASH_PAGE_PROGRAM  0x02
#define FLASH_READ          0x03
#define FLASH_READ_JEDEC_ID 0x9F
#define FLASH_READ_STATUS   0x05
#define FLASH_WRITE_DISABLE 0x04
#define FLASH_WRITE_ENABLE  0x06

typedef union {
    struct __attribute__((packed)) {
        uint32_t address : 24;
        uint8_t operation;
    };

    struct __attribute__((packed)) {
        uint8_t array[4];
    };

} flash_command_u;

typedef union {
    struct __attribute__((packed)) {
        // See: https://github.com/flashrom/flashrom/blob/main/include/flashchips.h
        uint8_t mfid;
        uint8_t memory_type;
        uint8_t capacity;
    };

    struct __attribute__((packed)) {
        uint8_t array[3];
    };
} flash_jedec_id_u;

void flash_init(void);
void flash_read_jedec_id(flash_jedec_id_u *jedec_id);
void flash_read(const uint32_t address, const void *destination, uint16_t length);
void flash_write(const uint32_t address, const void *source, uint16_t length);
void flash_write_enable(void);
void flash_write_disable(void);
uint8_t flash_read_status(void);
void flash_sector_erase(const uint32_t address);

#endif
