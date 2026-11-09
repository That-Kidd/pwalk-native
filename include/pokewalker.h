#pragma once
#include <stddef.h>
#include <stdint.h>
#include "pw_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PW_RAM_START 0xF780
#define PW_RAM_SIZE  0x0800

#define PW_LCD_PAGES 16
typedef struct pw_lcd {
    uint8_t ram[PW_LCD_PAGES][256];
    uint8_t page;
    uint8_t column;
    uint8_t start_line;
    uint8_t contrast;
    uint8_t display_on;
    uint8_t power_save;
    uint8_t cmd[2];
    uint8_t cmd_len;
} pw_lcd_t;

typedef struct pw_hw {
    uint64_t now;

    uint8_t rtc_sec, rtc_min, rtc_hour;
    uint8_t rtc_quarter;
    uint64_t rtc_next;
    uint8_t rtcflg, rtccr2;

    uint8_t tb_on;
    uint64_t tb_next;

    uint8_t tw_clock, tmrw, tw_irq;
    uint16_t gra, grb, grc;
    uint64_t tw_start;

    uint8_t irq0_flag;
    uint8_t keys_last;

    uint8_t masked;
    uint64_t ir_base;

    uint32_t beep_freq;
    uint8_t beep_vol;
} pw_hw_t;

typedef struct pw_state {
    uint8_t ram[PW_RAM_SIZE];

    pw_lcd_t lcd;
    pw_hw_t hw;

    const pw_hal_t *hal;
} pw_state_t;

#define PW_ROM_SIZE 0xC000

enum { PW_ROM_OK = 0, PW_ROM_BAD_SIZE = -1, PW_ROM_BAD_HASH = -2 };

/* Loads the firmware's data (fonts, strings, tables) from the user's own
   48 KB ROM dump, checked by SHA-1. Call once before pw_init or pw_load;
   the data is shared by every walker in the process. */
int pw_rom_load(const void *rom, size_t size);

int pw_rom_loaded(void);

void pw_init(pw_state_t *s, const pw_hal_t *hal);

void pw_render(const pw_state_t *s, uint8_t pixels[PW_SCREEN_HEIGHT][PW_SCREEN_WIDTH]);

void pw_update(pw_state_t *s, uint32_t ms);

void pw_set_hal(pw_state_t *s, const pw_hal_t *hal);

size_t pw_save_size(void);

size_t pw_save(const pw_state_t *s, void *buf, size_t size);

int pw_load(pw_state_t *s, const void *buf, size_t size, const pw_hal_t *hal);

#ifdef __cplusplus
}
#endif
