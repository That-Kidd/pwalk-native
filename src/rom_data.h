#pragma once
#include <stdint.h>

/* The firmware's data area (fonts, strings, tables) after the code. It is
   not distributed with this project: pw_rom_load() copies it from the
   user's own ROM dump at startup. */
#define PW_ROM_DATA_START 0xBAC6
#define PW_ROM_DATA_END   0xBFFF

extern uint8_t pw_rom_data[PW_ROM_DATA_END - PW_ROM_DATA_START + 1];

static inline uint8_t rom_rd8(uint16_t addr) {
    if (addr >= PW_ROM_DATA_START && addr <= PW_ROM_DATA_END) {
        return pw_rom_data[addr - PW_ROM_DATA_START];
    }
    return 0xFF;
}
