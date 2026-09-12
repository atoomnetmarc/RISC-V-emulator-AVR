# What is this?

This is a project that uses my [RISC-V emulator library](https://github.com/atoomnetmarc/RISC-V-emulator) and runs on my [AT90S8535 8-bit AVR demo board](https://github.com/atoomnetmarc/AVR-demo-board?tab=readme-ov-file#at90s8535).

I developed and debugged the code on a [pin-compatible](https://github.com/atoomnetmarc/AVR-demo-board/blob/main/AVR overview.md) ATmega1284P using Microchip Studio and the ATMEL-ICE programmer-debugger. The demo board has a JTAG header (J2/EXP) for debugging.

# How fast is the emulated RV32I cpu?

It is really slow. CPU speed is heavily influenced on how the SPI flash, RAM, TF and SD-card is accessed. Expect at most 10 kHz. But hey! It works. And for a CPU this slow having 8MiB of RAM, 8MiB ROM and a huge SD-card it could run forever processing unique program code and data.

# Shopping list

My [AT90S8535 demoboard](https://github.com/atoomnetmarc/AVR-demo-board/tree/main/at90s8535/Kicad). You can use a vintage AT90S8535 or a [more recent pin-compatible one](https://github.com/atoomnetmarc/AVR-demo-board/blob/main/AVR%20overview.md).

Connect APS6404L 8MiB SPI RAM on PMOD connector J5 (using my [PMOD-SPI-Memory](https://github.com/atoomnetmarc/PMOD-SPI-Memory) or a compatible board).

Connect a W25Q64 8MiB SPI FLASH to PMOD connector J9.

Connect a 5V USB UART to PMOD connector J4, like my [PMOD-USB-UART](https://github.com/atoomnetmarc/PMOD-USB-UART) board.

Connect my [PMOD-extended-SPI-TFT](https://github.com/atoomnetmarc/PMOD-extended-SPI-TFT) with a ST7735S 128x160 TFT display and FAT32 formatted SD-card to Extended SPI PMOD connector J10.

# How to make RV32I programs?

Compile a RV32I program using gcc.

| Setting | Value  |
| ------- | ------ |
| mabi    | ilp32  |
| march   | rv32i  |
| mcmodel | medlow |

Memory layout:

| Name   | Address    | Length | Note                             |
| ------ | ---------- | ------ | -------------------------------- |
| AVR IO | 0x02000000 | 0xFF   | Direct access to host registers. |
| UART   | 0x10000000 |        |
| ROM    | 0x20000000 | 8MiB   |
| RAM    | 0x80000000 | 8MiB   |

# How does to make it work?

Compile this code into firmware using Platformio (on Visual Studio Code). Choose the env (from `platformio.ini`) that matches your AVR microcontroller type and speed.

Program the firmware onto the microcontroller with ISP using some compatible AVR programmer. See [AVRDUDE](https://github.com/avrdudes/avrdude) for inspiration. I use [AVRDUDESS](https://github.com/ZakKemble/AVRDUDESS) with [Arduino-as-ISP-shield](https://github.com/atoomnetmarc/Arduino-as-ISP-shield) for the AT90S8535 and Microchip Studio Device Programming and an ATMEL-ICE to program the ATmega1284P.

Reset the FUSE settings to factory default (see datasheet). Then configure these fuses if available:

- Disable `CKDIV8`.
- Set `BODLEVEL` to 4.3V.
- Set `CKSEL` to `Full Swing Crystal Oscillator`.

Use https://www.engbedded.com/fusecalc/ to calculate the fuse bytes.

Format a SD-card FAT32.

Put compiled RV32I program on SD-card root. Filename: `firmware.bin`.

Power-on the demo board.

`firmware.bin` will be programmed into the SPI FLASH.

The emulator starts executing code at address 0x0 in SPI FLASH (ROM) at virtual address 0x20000000.

# UART

The UART is initialized at 38400 8N1. Both the firmware and the emulated code will use the same UART for output.

# ACT mode

The `[env:ATmega1284P_ACT]` environment in [platformio.ini](platformio.ini) builds the emulator for the ATmega1284P with ACT (Architecture Compliance Test) support enabled (`RVE_AVR_E_ACT=1`), together with the full set of RISC-V extensions (M, A, C, B, Zicsr, etc.). This build has no instruction disassembly, so it runs as fast as possible in the simavr backend.

For debugging failing tests, `[env:ATmega1284P_ACT_DISASM]` builds the same firmware with built-in instruction disassembly (`RVE_E_DISASM=1`) and verbose messages enabled, printing everything over the UART. This is significantly slower.

In this mode the emulator can run the official RISC-V ACT test suites: the RISC-V binary is loaded into the VPI-backed RAM, which is accessed via `act_load()`/`act_store()` ([include/act.h](include/act.h)), and a store to the halt region signals pass or fail (`RVMODEL_HALT_PASS`/`RVMODEL_HALT_FAIL`). This environment is used for compliance testing.

# License

I license my code Apache License, Version 2.0.

Individual files can be licensed differently. For example: [Petit-FatFs library](lib/Petit-FatFs/).

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
