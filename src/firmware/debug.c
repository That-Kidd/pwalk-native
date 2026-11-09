#include <stddef.h>
#include "pw_internal.h"

#define ROM_SOUND_TEST_BEEP 0xBFA2

uint8_t eepromFactoryTest(pw_state_t *s, uint16_t start) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x100);
    uint8_t c = 0;
    uint16_t addr = start;
    do {
        for (int i = 0; i < 0x100; i++) {
            wr8(s, buf + i, c++);
        }
        eepromWriteData(s, addr, buf, 0x100);
        addr += 0x100;
        c++;
    } while (addr);
    addr = start;
    c = 0;
    do {
        eepromReadToRamAlso(s, addr, buf, 0x100);
        for (int i = 0; i < 0x100; i++) {
            if (rd8(s, buf + i) != c++) {
                return 0;
            }
        }
        addr += 0x100;
        c++;
    } while (addr);
    addr = start;
    do {
        eepromFillPageWithByte(s, addr, 0xFF);
        addr += 0x80;
    } while (addr);
    return 1;
}

uint8_t adcClibrationDataVerifySelfChecksum(uint16_t word) {
    uint8_t sum = (uint8_t)((word >> 4 & 0xF) + (word >> 8 & 0xF) + (word & 0xF)) & 0xF;
    return sum == word >> 12;
}

static void sub_A962(pw_state_t *s) {
    static const uint8_t planes[4][2] = { { 0x00, 0x00 }, { 0x00, 0xFF }, { 0xFF, 0x00 }, { 0xFF, 0xFF } };
    for (uint8_t stripe = 0; stripe < 8; stripe++) {
        lcd_set_address(s, 0, stripe);
        for (uint8_t x = 0; x < 0x60; x++) {
            lcdc_data(s, planes[x / 24][0]);
            lcdc_data(s, planes[x / 24][1]);
        }
    }
}

static void sub_AC1E(pw_state_t *s) {
    lcd_set_address(s, 1, 0);
    for (int i = 0; i < 0xBC; i++) {
        lcdc_data(s, 0x01);
    }
    for (uint8_t stripe = 0; stripe < 8; stripe++) {
        lcd_set_address(s, 0, stripe);
        lcdc_data(s, 0xFF);
        lcdc_data(s, 0xFF);
    }
    for (uint8_t stripe = 0; stripe < 8; stripe++) {
        lcd_set_address(s, 0x5F, stripe);
        lcdc_data(s, 0xFF);
        lcdc_data(s, 0xFF);
    }
    lcd_set_address(s, 1, 7);
    for (int i = 0; i < 0xBC; i++) {
        lcdc_data(s, 0x80);
    }
}

static uint8_t sub_a_lo(pw_state_t *s) {
    return rd8(s, RAM_gCurSubstateA + 1);
}
static uint8_t sub_b_hi(pw_state_t *s) {
    return rd8(s, RAM_gCurSubstateB);
}
static uint8_t sub_b_lo(pw_state_t *s) {
    return rd8(s, RAM_gCurSubstateB + 1);
}

void interactionHandlerDebugMode22(pw_state_t *s) {
    wr8(s, RAM_byte_FFF7AF, 0x3C);
    wr8(s, RAM_byte_FFF7B0, 0x1E);
    uint8_t a = rd8(s, RAM_gCurSubstateA);
    uint8_t y = rd8(s, RAM_gCurSubstateY);
    uint8_t sec, mn, hr;
    switch (y) {
    case 0:
        if (rd8(s, RAM_gCurSubstateZ)) {
            wr8(s, RAM_gCurSubstateY, y + 1);
        }
        return;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:

        if (a < 4) {
            return;
        }
        if (checkKeyStates(s, KEY_ENTER | KEY_RIGHT)) {
            goto beep_next;
        }
        if (y == 1 || !checkKeyStates(s, KEY_LEFT)) {
            return;
        }
        write_F7C4_clear_F7C8_and_F7CA(s, ROM_SOUND_TEST_BEEP);
        y--;
        goto set;
    case 7:
        if (checkKeyStates(s, KEY_LEFT))
            goto beep_next;
        return;
    case 8:
        if (checkKeyStates(s, KEY_ENTER))
            goto beep_next;
        return;
    case 9:
        if (checkKeyStates(s, KEY_RIGHT))
            goto beep_next;
        return;
    case 10:
        if (a < 2)
            return;
        goto next;
    case 11:

        hw_rtc_read(s, &sec, &mn, &hr);
        wr8(s, RAM_gCurSubstateA + 1, sec);
        wr8(s, RAM_gCurSubstateB + 1, eepromFactoryTest(s, 0x300));
        some_sort_of_eeprom_init(s, 1, 1);
        hw_rtc_read(s, &sec, &mn, &hr);
        wr8(s, RAM_gCurSubstateB, sec);
        goto next;
    case 12:
    case 14:
        if (!sub_b_lo(s) || a < 4)
            return;
        goto beep_next;
    case 13:
        readReliableDataFromEeprom(s, 0x80, 0x180, RAM_gCurSubstateC, 2);
        wr8(s, RAM_gCurSubstateB + 1, adcClibrationDataVerifySelfChecksum(rd16(s, RAM_gCurSubstateC)));
        if (!rd16(s, RAM_gCurSubstateC)) {
            wr8(s, RAM_gCurSubstateB + 1, 0);
        }
        goto next;
    case 15:

        if (sub_a_lo(s) == sub_b_hi(s))
            return;
        goto beep_next;
    case 16: wr8(s, RAM_gCurSubstateB + 1, 1); goto next;
    case 17:
        if (!sub_b_lo(s))
            return;
        goto beep_next;
    case 18:
        if (a < 4 || !checkKeyStates(s, KEY_ANY))
            return;
        lcdc_command(s, 0xA6);
        wr8(s, RAM_currentlyActiveView, 0x17);
        enterDebugView22(s);
        write_F7C4_clear_F7C8_and_F7CA(s, ROM_SOUND_TEST_BEEP);
        return;
    default: return;
    }
beep_next:
    write_F7C4_clear_F7C8_and_F7CA(s, ROM_SOUND_TEST_BEEP);
next:
    y++;
set:
    wr8(s, RAM_gCurSubstateY, y);
    wr8(s, RAM_gCurSubstateA, 0);
}

void drawSomethingWithTinyChars(pw_state_t *s) {
    const char *msg = NULL;
    uint8_t x = 0x20, y = 8;
    switch (rd8(s, RAM_gCurSubstateY)) {
    case 0:
        if (!rd8(s, RAM_gCurSubstateZ))
            msg = "NG1";
        break;
    case 1: lcdFillBackBuffer(s, 3); break;
    case 2: lcdFillBackBuffer(s, 2); break;
    case 3: lcdFillBackBuffer(s, 1); break;
    case 4: lcdFillBackBuffer(s, 0); break;
    case 5: sub_AC1E(s); break;
    case 6: sub_A962(s); break;

    case 7:
        if (!(rd8(s, RAM_curUiFlags) >> 1 & 1)) {
            msg = "V";
            x = 0x06;
            y = 0x38;
        }
        break;
    case 8:
        if (!(rd8(s, RAM_curUiFlags) >> 1 & 1)) {
            msg = "V";
            x = 0x2D;
            y = 0x38;
        }
        break;
    case 9:
        if (!(rd8(s, RAM_curUiFlags) >> 1 & 1)) {
            msg = "V";
            x = 0x55;
            y = 0x38;
        }
        break;
    case 10:
    case 11: msg = "EEP"; break;
    case 12:
        if (!rd8(s, RAM_gCurSubstateB + 1))
            msg = "NG2";
        break;
    case 14:
        if (!rd8(s, RAM_gCurSubstateB + 1))
            msg = "NG3";
        break;
    case 15:
        if (rd8(s, RAM_gCurSubstateA + 1) == rd8(s, RAM_gCurSubstateB))
            msg = "NG4";
        break;
    case 17:
        if (!rd8(s, RAM_gCurSubstateB + 1))
            msg = "NG5";
        break;
    case 18: {
        char hex[5];
        lcdc_command(s, 0xA7);
        draw_tiny_string(s, 0x20, 0, "OK");
        hex4(hex, rd16(s, RAM_gCurSubstateC));
        draw_tiny_string(s, 0x20, 0x18, hex);
        if (!(rd8(s, RAM_curUiFlags) >> 1 & 1)) {
            draw_tiny_string(s, 0x06, 0x38, "V");
            draw_tiny_string(s, 0x2D, 0x38, "V");
            draw_tiny_string(s, 0x55, 0x38, "V");
        }
        break;
    }
    default: break;
    }
    if (msg) {
        draw_tiny_string(s, x, y, msg);
    }
    if (rd8(s, RAM_gCurSubstateA) < 4) {
        wr8(s, RAM_gCurSubstateA, rd8(s, RAM_gCurSubstateA) + 1);
    }
}
