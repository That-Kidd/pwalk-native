#include "pw_internal.h"

#define ROM_TINY_CHARS 0xBCF4

#define EEPROM_DIGIT_GLYPHS 0x0280
#define DIGIT_GLYPHS_SIZE   0x0140

static void draw_tiny_char(pw_state_t *s, uint8_t c) {
    if (c == ' ') {
        for (int i = 0; i < 8; i++) {
            lcdc_data(s, 0);
        }
        return;
    }
    uint8_t glyph = c > '9' ? (uint8_t)(c - ('A' - 10)) : (uint8_t)(c - '0');
    for (int col = 0; col < 3; col++) {
        uint8_t bits = rom_rd8(ROM_TINY_CHARS + glyph * 3 + col);
        lcdc_data(s, bits);
        lcdc_data(s, bits);
    }
    lcdc_data(s, 0);
    lcdc_data(s, 0);
}

void drawTinyChars(pw_state_t *s, uint8_t x, uint8_t y, uint16_t str) {
    lcd_set_address(s, x, y >> 3);
    for (uint8_t c; (c = mem_rd8(s, str)) != 0; str++) {
        draw_tiny_char(s, c);
    }
}

void draw_tiny_string(pw_state_t *s, uint8_t x, uint8_t y, const char *str) {
    lcd_set_address(s, x, y >> 3);
    for (; *str; str++) {
        draw_tiny_char(s, (uint8_t)*str);
    }
}

void drawNumberOnscreen(pw_state_t *s, uint8_t x, int8_t y, uint32_t number, uint8_t overline) {
    setHeapPointer(s);
    uint16_t glyphs = static_malloc(s, DIGIT_GLYPHS_SIZE);
    eepromReadToRamAlso(s, EEPROM_DIGIT_GLYPHS, glyphs, DIGIT_GLYPHS_SIZE);
    if (overline) {
        for (int digit = 0; digit < 10; digit++) {
            for (int col = 0; col < 8; col++) {
                uint16_t p = glyphs + digit * 32 + col * 2;
                wr16(s, p, rd16(s, p) | 0x0101);
            }
        }
    }
    if (number == 0) {
        drawImageToScreen(s, x, y, glyphs, 8, 16);
        return;
    }
    while (number != 0) {
        uint32_t digit;
        number = divmod32(number, 10, &digit);
        drawImageToScreen(s, x, y, glyphs + digit * 32, 8, 16);
        x -= 8;
    }
}

#define EEPROM_WATT_SYMBOL 0x0420
#define WATT_SYMBOL_SIZE   0x40

void drawNumberAndWattsymbolOnscreen(pw_state_t *s, uint8_t x, int8_t y, uint16_t watts) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, DIGIT_GLYPHS_SIZE);
    drawBoxOutlineRows32To47(s);
    lcdFillRectangle(s, 1, 40, 94, 8, 0);
    drawNumberOnscreen(s, x + 8, y, watts, 1);

    eepromReadToRamAlso(s, EEPROM_WATT_SYMBOL, buf, WATT_SYMBOL_SIZE);
    for (int col = 0; col < 16; col++) {
        uint16_t p = buf + col * 2;
        wr16(s, p, rd16(s, p) | 0x0101);
    }
    drawImageToScreen(s, x + 16, y, buf, 16, 16);
}

#define EEPROM_MESSAGES       0x2530
#define MESSAGE_SIZE          0x180
#define EEPROM_MESSAGE_CURSOR 0x0638
#define MESSAGE_CURSOR_SIZE   0x18
#define MESSAGE_CURSOR_OFFSET 0x170

void drawLargeMessage(pw_state_t *s, uint8_t y, uint8_t message, uint8_t borders, uint8_t cursor) {
    setHeapPointer(s);
    uint16_t text = static_malloc(s, MESSAGE_SIZE);
    uint16_t aux = static_malloc(s, MESSAGE_CURSOR_SIZE);
    eepromReadToRamAlso(s, (uint16_t)(EEPROM_MESSAGES + message * MESSAGE_SIZE), text, MESSAGE_SIZE);
    textImageAddBorders(s, 96, 16, text, borders);

    if (cursor && (rd8(s, RAM_curUiFlags) >> 1) & 1) {
        eepromReadToRamAlso(s, EEPROM_MESSAGE_CURSOR, aux, MESSAGE_CURSOR_SIZE);
        uint16_t at = text + MESSAGE_CURSOR_OFFSET;
        for (int col = 0; col < 8; col++) {
            uint8_t mask = rd8(s, aux + 0x10 + col);
            wr8(s, at + col * 2, rd8(s, at + col * 2) & mask);
            wr8(s, at + col * 2 + 1, rd8(s, at + col * 2 + 1) & mask);
        }
        for (int i = 0; i < 16; i++) {
            wr8(s, at + i, rd8(s, at + i) | rd8(s, aux + i));
        }
    }
    drawImageToScreen(s, 0, (int8_t)y, text, 96, 16);
}
