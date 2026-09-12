/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "defines.h"

#include <stdio.h>
#include <string.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include <RiscvEmulator.h>

#include "diskio.h"
#include "pff.h"

#include "flash.h"
#include "led.h"
#include "psram.h"
#include "spi.h"
#include "tft.h"
#include "uart.h"

RiscvEmulatorState_t RiscvEmulatorState;

__attribute__((OS_main)) int main(void) {
#if (RVE_AVR_E_ACT == 0)
    led_init();
#endif

    sei();
    uart_init();
#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("\n\nHello!\n"));
#endif
#if (RVE_AVR_E_ACT == 1)
    act_init();
#endif

#if (RVE_AVR_E_ACT == 0)

#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("Initialize SPI\n"));
#endif
    spi_init();

#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("Initialize PSRAM\n"));
#endif
    psram_init();

#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("Initialize FLASH\n"));
#endif
    flash_init();

#if RVE_AVR_E_TFT == 1
#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("Initialize TFT\n"));
#endif
    tft_init();
#endif

#if (RVE_AVR_E_FATFS == 1)
    BYTE buff[512]; // Must be power of 2 and maximal 1 flash sector size(4096)
    UINT br;
    BYTE res;
#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("Initialize SD-card\n"));
#endif

    res = mmc_disk_initialize();
    if (res != RES_OK) {
#if (RVE_AVR_E_VERBOSE == 1)
        printf_P(PSTR("SD-card initialize fail. DRESULT: 0x%02X. Stop.\n"), res);
#endif
        for (;;) {
        }
    }

#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("Mount SD-card\n"));
#endif
    FATFS fs;
    res = pf_mount(&fs);
    if (res != FR_OK) {
#if (RVE_AVR_E_VERBOSE == 1)
        printf_P(PSTR("SD-card mount fail. FRESULT: 0x%02X. Stop.\n"), res);
#endif
        for (;;) {
        }
    }

#if (PF_USE_DIR == 1 && \
     RVE_AVR_E_VERBOSE == 1)
    DIR dir;
    FILINFO fno;
    char *path = "";
    printf_P(PSTR("Files on SD-card root:\n"));
    res = pf_opendir(&dir, path);
    if (res == FR_OK) {
        for (;;) {
            res = pf_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0) {
                break;
            }
            if (!(fno.fattrib & AM_DIR)) {
                printf("%s/%s\n", path, fno.fname);
            }
        }
    } else {
        printf_P(PSTR("SD-card pf_opendir fail. FRESULT: 0x%02X. Stop.\n"), res);
    }
#endif

#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("Read MOTD.TXT\n"));
#endif
    res = pf_open("MOTD.TXT");
    if (res != FR_OK) {
#if (RVE_AVR_E_VERBOSE == 1)
        printf_P(PSTR("Read MOTD.TXT fail. FRESULT: 0x%02X.\n"), res);
#endif
    } else {
        do {
            res = pf_read(buff, sizeof(buff), &br);
            for (uint8_t i = 0; i < br; i++) {
                uart_putc(buff[i], NULL);
            }
        } while (br == sizeof(buff));
    }

#if (1)
#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("Read FIRMWARE.BIN\n"));
#endif
    res = pf_open("FIRMWARE.BIN");
    if (res != FR_OK) {
#if (RVE_AVR_E_VERBOSE == 1)
        printf_P(PSTR("Read FIRMWARE.BIN fail. FRESULT: 0x%02X.\n"), res);
#endif
    } else {
        uint32_t bytecounter = 0;
        do {
            if ((bytecounter % 0x1000) == 0) {
#if (RVE_AVR_E_VERBOSE == 1)
                printf_P(PSTR("Flash erase sector 0x%08X\n"), bytecounter);
#endif
                flash_sector_erase(bytecounter);
            }

            res = pf_read(buff, sizeof(buff), &br);
#if (RVE_AVR_E_VERBOSE == 1)
            printf_P(PSTR("Flash program 0x%02X bytes at 0x%08X\n"), sizeof(buff), bytecounter);
#endif
            flash_write(bytecounter, &buff, br);

            bytecounter += br;

        } while (br == sizeof(buff));
    }
#endif
#endif

#endif

#if (RVE_AVR_E_VERBOSE == 1)
    printf_P(PSTR("RiscvEmulatorInit\n"));
#endif
    RiscvEmulatorInit(&RiscvEmulatorState, RVE_AVR_E_RAM_LENGTH);

#if (RVE_AVR_E_ACT == 1)
    // The test binary lives in the VPI-backed RAM, not in ROM.
    RiscvEmulatorState.programcounter = RAM_ORIGIN;
    RiscvEmulatorState.programcounternext = RAM_ORIGIN;
#endif

    for (;;) {
#if (RVE_AVR_E_ACT == 0)
        PORTA ^= _BV(PA0);
#endif
        RiscvEmulatorLoop(&RiscvEmulatorState);

#if (RVE_AVR_E_ACT == 1)
        act_count_instruction();
#endif
    }
}
