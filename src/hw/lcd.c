#include <string.h>
#include "pw_internal.h"

static const uint8_t lcd_command_length[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 2, 2, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

static void lcdc_execute(pw_lcd_t *lcd) {
    uint8_t c = lcd->cmd[0];
    if (c <= 0x0F) {
        lcd->column = (lcd->column & 0xE0) | (c & 0x0F) << 1;
    } else if (c <= 0x17) {
        lcd->column = (lcd->column & 0x1E) | (c & 0x07) << 5;
    } else if (c >= 0x40 && c <= 0x43) {
        lcd->start_line = lcd->cmd[1];
    } else if (c == 0x81) {
        lcd->contrast = lcd->cmd[1] & 0x3F;
    } else if (c == 0xA9) {
        lcd->power_save = 1;
    } else if (c == 0xAE || c == 0xAF) {
        lcd->display_on = c & 1;
    } else if (c >= 0xB0 && c <= 0xBF) {
        lcd->page = c & 0x0F;
    } else if (c == 0xE1) {
        lcd->power_save = 0;
    } else if (c == 0xE2) {
        lcd->power_save = 0;
        lcd->page = 0;
        lcd->column = 0;
        lcd->start_line = 0;
        lcd->contrast = 0x20;
    }
}

void lcdc_command(pw_state_t *s, uint8_t byte) {
    pw_lcd_t *lcd = &s->lcd;
    lcd->cmd[lcd->cmd_len++] = byte;
    if (lcd->cmd_len >= lcd_command_length[lcd->cmd[0]]) {
        lcdc_execute(lcd);
        lcd->cmd_len = 0;
    }
}

void lcdc_data(pw_state_t *s, uint8_t byte) {
    pw_lcd_t *lcd = &s->lcd;
    lcd->ram[lcd->page][lcd->column++] = byte;
}

void lcdc_reset(pw_state_t *s) {
    memset(&s->lcd, 0, sizeof(s->lcd));
    s->lcd.contrast = 0x20;
}

void pw_render(const pw_state_t *s, uint8_t pixels[PW_SCREEN_HEIGHT][PW_SCREEN_WIDTH]) {
    const pw_lcd_t *lcd = &s->lcd;
    int blank = !lcd->display_on || lcd->power_save;
    for (int y = 0; y < PW_SCREEN_HEIGHT; y++) {
        int line = y + lcd->start_line;
        int page = line / 8, bit = line % 8;
        for (int x = 0; x < PW_SCREEN_WIDTH; x++) {
            uint8_t shade = 0;
            if (!blank && page < PW_LCD_PAGES) {
                uint8_t hi = lcd->ram[page][x * 2], lo = lcd->ram[page][x * 2 + 1];
                shade = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
            }
            pixels[y][x] = shade;
        }
    }
}

void lcd_set_address(pw_state_t *s, uint8_t x, uint8_t stripe) {
    lcdc_command(s, 0x10 | ((x >> 4) & 7));
    lcdc_command(s, x & 0x0F);
    lcdc_command(s, (uint8_t)(rd8(s, RAM_byte_FFF7E4) * 8 + stripe + 0xB0));
}

void lcdSendSingleByteCommand(pw_state_t *s, uint8_t cmd) {
    lcdc_command(s, cmd);
}

void lcdClearBackBuffer(pw_state_t *s, uint8_t height) {
    uint16_t stripes = height >> 3;
    for (uint8_t stripe = 0; stripe < stripes; stripe++) {
        lcd_set_address(s, 0, stripe);
        for (int col = 0; col < PW_SCREEN_WIDTH; col++) {
            lcdc_data(s, 0);
            lcdc_data(s, 0);
        }
    }
}

void lcdFlipBuffers(pw_state_t *s) {
    uint8_t back = rd8(s, RAM_byte_FFF7E4);
    lcdc_command(s, 0x40);
    lcdc_command(s, (uint8_t)(back * 0x40));
    wr8(s, RAM_byte_FFF7E4, back ^ 1);
}

void drawImageToScreen(pw_state_t *s, uint8_t x, int8_t y, uint16_t img, uint8_t w, uint8_t h) {
    int y_low = y & 7;

    int first = (uint8_t)(y / 8);
    int end = (uint8_t)((h + y + 7) / 8);
    int last = end - 1;
    uint16_t stride = w * 2;
    uint16_t src = img;

    for (int stripe = first; stripe < end; stripe++) {
        lcd_set_address(s, x, (uint8_t)stripe);
        for (int col = 0; col < w; col++, src += 2) {
            for (int plane = 0; plane < 2; plane++) {
                uint8_t out;
                if (y_low == 0) {
                    out = mem_rd8(s, src + plane);
                } else if (stripe == last) {
                    out = mem_rd8(s, src - stride + plane) >> (8 - y_low);
                } else if (stripe == first) {
                    out = mem_rd8(s, src + plane) << y_low;
                } else {
                    out = mem_rd8(s, src - stride + plane) >> (8 - y_low) | mem_rd8(s, src + plane) << y_low;
                }
                lcdc_data(s, out);
            }
        }
    }
}

void lcdFillRectangle(pw_state_t *s, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t shade) {
    static const uint8_t planes[4][2] = { { 0x00, 0x00 }, { 0x00, 0xFF }, { 0xFF, 0x00 }, { 0xFF, 0xFF } };
    int end_col = x + w;
    int end_stripe = (y + h) / 8;
    for (uint8_t stripe = y >> 3; stripe < end_stripe; stripe++) {
        lcd_set_address(s, x, stripe);
        for (uint8_t col = x; col < end_col; col++) {
            if (shade < 4) {
                lcdc_data(s, planes[shade][0]);
                lcdc_data(s, planes[shade][1]);
            }
        }
    }
}

void drawImageToBuffer(pw_state_t *s, uint8_t w, uint8_t h, uint16_t img, uint8_t x, uint8_t y, uint16_t buf,
                       uint8_t buf_w) {
    int y_low = y & 7;
    uint16_t dst = buf + ((y >> 3) * buf_w + x) * 2;
    uint16_t dst_stride = buf_w * 2;
    for (uint8_t stripe = 0; stripe < (h >> 3); stripe++) {
        for (uint8_t col = 0; col < w; col++) {
            uint16_t src = img + col * 2;
            uint16_t top = dst + col * 2, bottom = top + dst_stride;
            for (int plane = 0; plane < 2; plane++) {
                uint8_t b = rd8(s, src + plane);
                wr8(s, top + plane, rd8(s, top + plane) | (uint8_t)(b << y_low));
                wr8(s, bottom + plane, rd8(s, bottom + plane) | (uint8_t)(b >> (8 - y_low)));
            }
        }
        dst += dst_stride;
        img += w * 2;
    }
}

void lcdSetAddress(pw_state_t *s, uint8_t x, uint8_t stripe) {
    lcd_set_address(s, x, stripe);
}

void lcdShowBuffer(pw_state_t *s, uint8_t buffer) {
    if (buffer > 1) {
        return;
    }
    lcdc_command(s, 0x40);
    lcdc_command(s, (uint8_t)(buffer * 0x40));
    wr8(s, RAM_byte_FFF7E4, buffer ^ 1);
}

void lcdFillBackBuffer(pw_state_t *s, uint8_t shade) {
    static const uint8_t planes[4][2] = { { 0x00, 0x00 }, { 0x00, 0xFF }, { 0xFF, 0x00 }, { 0xFF, 0xFF } };
    for (uint8_t stripe = 0; stripe < 8; stripe++) {
        lcd_set_address(s, 0, stripe);
        for (int col = 0; col < PW_SCREEN_WIDTH; col++) {
            if (shade < 4) {
                lcdc_data(s, planes[shade][0]);
                lcdc_data(s, planes[shade][1]);
            }
        }
    }
}

static void box_stripe(pw_state_t *s, uint8_t stripe, uint8_t edge) {
    lcd_set_address(s, 0, stripe);
    lcdc_data(s, 0xFF);
    lcdc_data(s, 0xFF);
    for (int i = 0; i < 2 * (PW_SCREEN_WIDTH - 2); i++) {
        lcdc_data(s, edge);
    }
    lcdc_data(s, 0xFF);
    lcdc_data(s, 0xFF);
}

static void box_sides(pw_state_t *s, uint8_t stripe) {
    lcd_set_address(s, 0, stripe);
    lcdc_data(s, 0xFF);
    lcdc_data(s, 0xFF);
    lcd_set_address(s, PW_SCREEN_WIDTH - 1, stripe);
    lcdc_data(s, 0xFF);
    lcdc_data(s, 0xFF);
}

void drawBoxOutlineRows32To47(pw_state_t *s) {
    box_stripe(s, 4, 0x01);
    box_sides(s, 5);
}

void drawBoxOutlineRows48To63(pw_state_t *s) {
    box_stripe(s, 6, 0x01);
    box_stripe(s, 7, 0x80);
}

void lcdSetContrast(pw_state_t *s, uint8_t contrast) {
    lcdc_command(s, 0x81);
    lcdc_command(s, rd8(s, RAM_base_contrast_value) + contrast);
}

void lcdExitPowerSaveMode(pw_state_t *s) {
    lcdc_command(s, 0xE1);
}

void lcdEnterPowerSaveMode(pw_state_t *s) {
    lcdc_command(s, 0xA9);
}

#define EEPROM_LCD_SCRIPT0    0x00AC
#define EEPROM_LCD_SCRIPT1    0x01AC
#define LCD_SCRIPT_SIZE       0x40
#define ROM_LCD_INIT_COMMANDS 0xBEB8
#define LCD_SCRIPT_END        0xFE
#define LCD_SCRIPT_DELAY      0xFD

void lcdInit(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t script = static_malloc(s, LCD_SCRIPT_SIZE);
    readReliableDataFromEeprom(s, EEPROM_LCD_SCRIPT0, EEPROM_LCD_SCRIPT1, script, LCD_SCRIPT_SIZE);

    lcdc_command(s, 0xE1);
    uint8_t first = rd8(s, script);
    if (first == 0x00 || first == 0xFF) {
        script = ROM_LCD_INIT_COMMANDS;
    }
    wr8(s, RAM_base_contrast_value, mem_rd8(s, script++));
    for (uint8_t c; (c = mem_rd8(s, script)) != LCD_SCRIPT_END;) {
        if (c == LCD_SCRIPT_DELAY) {
            script += 2;
        }
        lcdc_command(s, mem_rd8(s, script++));
    }

    lcdc_command(s, 0xA6);
    lcdSetContrast(s, (rd8(s, RAM_RamCache_settingsByte) >> 3) & 0x0F);
    wr8(s, RAM_byte_FFF7E4, 1);
    lcdClearBackBuffer(s, 64);
    wr8(s, RAM_byte_FFF7E4, 0);
    lcdClearBackBuffer(s, 64);
    lcdc_command(s, 0xAF);
}

void drawImageClipped(pw_state_t *s, int8_t x, int8_t y, uint16_t w, uint16_t h, uint16_t img) {
    int yl = y & 7;
    int16_t first = y / 8;
    int16_t end = (int16_t)((uint16_t)(y + h + 7) >> 3);
    int16_t last = end - 1;
    int16_t end_col = x + w;
    uint16_t stride = w * 2;
    uint16_t src = img;

    for (int16_t stripe = first; stripe < end; stripe++) {
        if (stripe >= 8) {
            break;
        }
        if (stripe < 0) {
            src += stride;
            continue;
        }
        lcd_set_address(s, x < 0 ? 0 : (uint8_t)x, (uint8_t)stripe);
        for (int16_t col = x; col < end_col; col++, src += 2) {
            if (col < 0) {
                continue;
            }
            uint8_t a, b;
            if (yl == 0) {
                a = rd8(s, src);
                b = rd8(s, src + 1);
            } else if (stripe == last) {
                a = shr8(rd8(s, src - stride), 8 - yl);
                b = shr8(rd8(s, src - stride + 1), 8 - yl);
            } else if (stripe != first) {
                a = shl8(rd8(s, src), yl) | shr8(rd8(s, src - stride + 1), 8 - yl);
                b = shl8(rd8(s, src + 1), yl) | shr8(rd8(s, src - stride), 8 - yl);
            } else {
                a = shl8(rd8(s, src), yl);
                b = shl8(rd8(s, src + 1), yl);
            }
            lcdc_data(s, a);
            lcdc_data(s, b);
        }
    }
}

void drawLineAtRow48(pw_state_t *s) {
    lcdc_command(s, 0x10);
    lcdc_command(s, 0x00);
    lcdc_command(s, (uint8_t)(rd8(s, RAM_byte_FFF7E4) * 8 + 0xB6));
    for (int i = 0; i < 0xC0; i++) {
        lcdc_data(s, 0x01);
    }
}
