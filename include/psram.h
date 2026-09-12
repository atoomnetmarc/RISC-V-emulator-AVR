/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdint.h>

#ifndef PSRAM_H_
#define PSRAM_H_

#define PSRAM_READ         0x03
#define PSRAM_WRITE        0x02
#define PSRAM_READ_ID      0x9F
#define PSRAM_RESET_ENABLE 0x66
#define PSRAM_RESET        0x99

typedef union {
    struct __attribute__((packed)) {
        uint32_t address : 24;
        uint8_t operation;
    };

    struct __attribute__((packed)) {
        uint8_t array[4];
    };

} psram_command_u;

typedef union {
    struct __attribute__((packed)) {
        // Electronic ID (48 bits, 6 bytes)
        uint8_t eid[6];

        // Known Good Die
        uint8_t kgd;

        // Manufacturer ID
        uint8_t mfid;
    };

    struct __attribute__((packed)) {
        uint8_t array[8];
    };
} psram_eid_u;

void psram_init(void);
void psram_reset(void);
void psram_read_eid(psram_eid_u *eid);
void psram_read(uint32_t address, const void *destination, uint8_t length);
void psram_write(uint32_t address, const void *source, uint8_t length);

#endif
