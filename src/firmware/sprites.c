#include "pw_internal.h"

static void draw_eeprom_image(pw_state_t *s, uint16_t eeprom_addr, uint8_t x, int8_t y, uint8_t w, uint8_t h) {
    uint16_t size = w * 2 * (h / 8);
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, size);
    eepromReadToRamAlso(s, eeprom_addr, buf, size);
    drawImageToScreen(s, x, y, buf, w, h);
}

void drawItemSymbolOnScreen(pw_state_t *s, uint8_t x, int8_t y) {
    draw_eeprom_image(s, 0x0488, x, y, 8, 8);
}

void drawLargeItemChest(pw_state_t *s, uint8_t x, int8_t y) {
    draw_eeprom_image(s, 0x1910, x, y, 32, 24);
}

void drawLargePresentItem(pw_state_t *s, uint8_t x, int8_t y) {
    draw_eeprom_image(s, 0x1A90, x, y, 32, 24);
}

void drawPokemonFeelingBubble(pw_state_t *s, uint8_t feeling) {
    draw_eeprom_image(s, (uint16_t)(0x0670 + feeling * 0x60), 4, 4, 24, 16);
}

void drawCurPokemonSmallAnimatedFrame(pw_state_t *s, uint8_t x, int8_t y) {
    uint8_t frame = rd8(s, RAM_curUiFlags) & 1;
    draw_eeprom_image(s, (uint16_t)(0x91BE + frame * 0xC0), x, y, 32, 24);
}

void drawCurrentAreaSmallImage(pw_state_t *s) {
    uint16_t img = (rd8(s, RAM_RamCache_settingsByte) & 1) ? 0xC83C : 0x8FBE;
    draw_eeprom_image(s, img, 0, 24, 32, 24);
}

void drawCurPokemonSmallAnimatedFrameFlipped(pw_state_t *s, uint8_t x, int8_t y) {
    uint8_t frame = rd8(s, RAM_curUiFlags) & 1;
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0xC0);
    eepromReadToRamAlso(s, (uint16_t)(0x91BE + frame * 0xC0), buf, 0xC0);
    imageFlipHorizontal(s, 32, 24, buf);
    drawImageToScreen(s, x, y, buf, 32, 24);
}

void drawMusicNote(pw_state_t *s, uint8_t x, int8_t y, uint8_t shift) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x10);
    eepromReadToRamAlso(s, 0x2470, buf, 0x10);
    for (int i = 0; i < 0x10; i++) {
        lsrByteAtPtr(s, buf + i, shift);
    }
    drawImageToScreen(s, x, y, buf, 8, 8);
}

void drawRewardReceivedBanner(pw_state_t *s) {
    draw_eeprom_image(s, 0x1910, 32, 4, 32, 24);
    drawLargeMessage(s, 32, 0x42, 0x0D, 0);
    drawLargeMessage(s, 48, 0x0F, 0x0E, 1);
}

static void draw_eeprom_text(pw_state_t *s, uint16_t eeprom_addr, uint8_t w, uint8_t x, int8_t y, uint8_t borders,
                             uint16_t buf) {
    eepromReadToRamAlso(s, eeprom_addr, buf, w * 4);
    textImageAddBorders(s, w, 16, buf, borders);
    drawImageToScreen(s, x, y, buf, w, 16);
}

void showRxedItemName(pw_state_t *s, uint8_t x, int8_t y, uint8_t borders) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    draw_eeprom_text(s, 0xBD48, 96, x, y, borders, buf);
}

void drawPeerPokemonName(pw_state_t *s, uint8_t x, int8_t y, uint8_t borders) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x140);
    drawBoxOutlineRows32To47(s);
    draw_eeprom_text(s, 0xF580, 80, x, y, borders, buf);
}

void showEventPokeName(pw_state_t *s, uint8_t x, int8_t y, uint8_t borders) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x140);
    if ((uint8_t)y == 0x30) {
        drawBoxOutlineRows48To63(s);
    } else {
        drawBoxOutlineRows32To47(s);
    }
    draw_eeprom_text(s, 0xBC00, 80, x, y, borders, buf);
}

void drawPeerPlayPokemonFrame(pw_state_t *s, int8_t x, int8_t y, uint8_t flip) {
    uint8_t frame = rd8(s, RAM_curUiFlags) & 1;
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0xC0);
    eepromReadToRamAlso(s, (uint16_t)(0xF400 + frame * 0xC0), buf, 0xC0);
    if (flip) {
        imageFlipHorizontal(s, 32, 24, buf);
    }
    drawImageClipped(s, x, y, 32, 24, buf);
}

void drawLowBatteryIcon(pw_state_t *s, uint8_t x, int8_t y) {
    if (!(rd8(s, RAM_common_bit_flags) & 0x02)) {
        return;
    }
    if ((rd8(s, RAM_curUiFlags) >> 2) & 1) {
        return;
    }
    draw_eeprom_image(s, 0x0660, x, y, 8, 8);
}

void drawRouteAvailablePokemon(pw_state_t *s, uint8_t x, int8_t y, uint8_t n) {
    uint8_t frame = rd8(s, RAM_curUiFlags) & 1;
    draw_eeprom_image(s, (uint16_t)(0x9A7E + n * 0x180 + frame * 0xC0), x, y, 32, 24);
}

void drawNameOfEventPokemon(pw_state_t *s, uint8_t x, int8_t y, uint8_t borders) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x140);
    if ((uint8_t)y == 0x30) {
        drawBoxOutlineRows48To63(s);
    } else {
        drawBoxOutlineRows32To47(s);
    }
    draw_eeprom_text(s, 0xC6FC, 80, x, y, borders, buf);
}

void drawCurPokemonNameWithBorders(pw_state_t *s, uint8_t x, int8_t y, uint8_t borders) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x140);
    if ((uint8_t)y == 0x30) {
        drawBoxOutlineRows48To63(s);
    } else {
        drawBoxOutlineRows32To47(s);
    }
    draw_eeprom_text(s, 0x993E, 80, x, y, borders, buf);
}

void drawNameOfRouteAvailablePokemon(pw_state_t *s, uint8_t x, int8_t y, uint8_t idx, uint8_t borders) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    if ((uint8_t)y == 0x30) {
        drawBoxOutlineRows48To63(s);
    } else {
        drawBoxOutlineRows32To47(s);
    }
    draw_eeprom_text(s, (uint16_t)(0xA4FE + idx * 0x140), 80, x, y, borders, buf);
}

void drawRouteAvailableItemName(pw_state_t *s, uint8_t x, int8_t y, uint8_t idx, uint8_t borders) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    draw_eeprom_text(s, (uint16_t)(0xA8BE + idx * 0x180), 96, x, y, borders, buf);
}

void homeScreenDrawLargeCurrentPokemonImage(pw_state_t *s, uint8_t x, int8_t y) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x300);
    eepromReadToRamAlso(s, 0x8F00, buf, 0x10);
    uint16_t addr = 0x933E;
    if (rd8(s, RAM_gCurSubstateZ) == 0) {
        addr += ((rd8(s, RAM_curUiFlags) >> 1) & 1) * 0x300;
    }
    eepromReadToRamAlso(s, addr, buf, 0x300);
    drawImageToScreen(s, x, y, buf, 64, 48);
}

static void draw_cloud_with_bars(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    eepromReadToRamAlso(s, 0x1F70, buf, 0xC0);
    drawImageToScreen(s, 32, 16, buf, 32, 24);
    lcdFillRectangle(s, 0, 0, 96, 8, 3);
    lcdFillRectangle(s, 0, 56, 96, 8, 3);
}

void drawCloudWithBarsOnTopAndBottomOfScreen(pw_state_t *s) {
    draw_cloud_with_bars(s);
    wr8(s, RAM_gCurSubstateY, 0);
    wr8(s, RAM_gCurSubstateZ, 0);
}

void drawCloudWithBarsOnTopAndBottomOfScreen2(pw_state_t *s) {
    draw_cloud_with_bars(s);
    if (rd8(s, RAM_gCurSubstateZ) == 0) {
        wr8(s, RAM_gCurSubstateZ, 1);
    }
}

void drawPokewalkerAndCompletedMessage(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x100);
    eepromReadToRamAlso(s, 0x2350, buf, 0x100);
    drawImageToScreen(s, 32, 16, buf, 32, 32);
    drawLargeMessage(s, 0x30, 0x10, 0x0F, 0);
    uint8_t z = rd8(s, RAM_gCurSubstateZ);
    if (z < 0x10) {
        wr8(s, RAM_gCurSubstateZ, z + 1);
    }
    if (!is_F7C4_nonzero(s) && rd8(s, RAM_gCurSubstateZ) > 8) {
        resetSubstates(s);
        setCurrentlyActiveView(s, 0);
    }
}

#define ROM_BUSH_X 0xBF25

void drawLargePokemonImageWithBars(pw_state_t *s) {
    homeScreenDrawLargeCurrentPokemonImage(s, 0x10, 8);
    lcdFillRectangle(s, 0, 0, 96, 8, 3);
    lcdFillRectangle(s, 0, 56, 96, 8, 3);
    wr8(s, RAM_gCurSubstateZ, rd8(s, RAM_gCurSubstateZ) + 1);
}

static void message_screen_timeout(pw_state_t *s) {
    uint8_t z = rd8(s, RAM_gCurSubstateZ);
    if (z < 0x10) {
        wr8(s, RAM_gCurSubstateZ, z + 1);
    }
    if (!is_F7C4_nonzero(s) && rd8(s, RAM_gCurSubstateZ) > 8) {
        resetSubstates(s);
        setCurrentlyActiveView(s, 0);
    }
}

void pokemonDepartureScreen(pw_state_t *s) {
    setHeapPointer(s);
    static_malloc(s, 0x180);
    drawCurPokemonNameWithBorders(s, 0, 0x20, 5);
    drawLargeMessage(s, 0x30, 0x0E, 0x0E, 0);
    message_screen_timeout(s);
}

void pokemonArrivalScreen(pw_state_t *s) {
    drawCurPokemonSmallAnimatedFrame(s, 0x20, 4);
    drawCurPokemonNameWithBorders(s, 0, 0x20, 5);
    drawLargeMessage(s, 0x30, 0x0D, 0x0E, 0);
    message_screen_timeout(s);
}

void pokeradarUserTooSlowScreen(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0xC0);
    eepromReadToRamAlso(s, 0x1CB0, buf, 0xC0);
    for (uint8_t i = 0; i < 4; i++) {
        drawImageToScreen(s, rom_rd8(ROM_BUSH_X + i), (int8_t)((i & 1) * 24), buf, 32, 24);
    }
    drawLargeMessage(s, 0x30, 0x1E, 0x0F, 1);
    drawLowBatteryIcon(s, 0, 0);
}

#define ROM_BALL_DROP_Y      0xBD70
#define ROM_ITEM_SCREEN_LOCS 0xBD76
#define ROM_GOING_AWAY_Y     0xBD7C

static void draw_bars(pw_state_t *s) {
    lcdFillRectangle(s, 0, 0, 96, 8, 3);
    lcdFillRectangle(s, 0, 56, 96, 8, 3);
}

void drawCurSelectedPokeNameForSwitchScreen(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x40);
    eepromReadToRamAlso(s, 0x8F52, buf, 0x30);
    int8_t slot = (int8_t)rd8(s, RAM_gCurSubstateZ);
    eepromReadToRamAlso(s, (uint16_t)(0xCE8C + slot * 0x10), buf + 0x30, 0x10);
    for (uint8_t i = 0; i < 3; i++) {
        if (rd16(s, buf + i * 0x10) == rd16(s, buf + 0x30)) {
            drawNameOfRouteAvailablePokemon(s, 0, 0x30, i, 7);
            return;
        }
    }
}

void drawCurSelectedItemNameForSwitchScreen(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x14);
    int8_t slot = (int8_t)rd8(s, RAM_gCurSubstateZ);
    uint16_t at = (uint16_t)(0xCEBC + slot * 4);
    uint16_t item = eepromReadSingleByte(s, at) << 8;
    item |= eepromReadSingleByte(s, at + 1);
    eepromReadToRamAlso(s, 0x8F8C, buf, 0x14);
    for (uint8_t i = 0; i < 10; i++) {
        if (rd16(s, buf + i * 2) == item) {
            drawRouteAvailableItemName(s, 0, 0x30, i, 0x0F);
            return;
        }
    }
}

void drawMovingBlankIconWithBarsOnTopAndBottom(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    uint8_t z = rd8(s, RAM_gCurSubstateZ);
    if (z <= 4) {
        eepromReadToRamAlso(s, 0x2480, buf, 0x10);
        drawImageToScreen(s, 0x2C, (int8_t)mem_rd8(s, ROM_GOING_AWAY_Y + z), buf, 8, 8);
    }
    draw_bars(s);
    wr8(s, RAM_gCurSubstateZ, rd8(s, RAM_gCurSubstateZ) + 1);
}

void drawBallDropFromTopOfScreen(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    uint16_t img = rd8(s, RAM_currentlyActiveView) == 0x11 ? 0x0460 : 0x2480;
    eepromReadToRamAlso(s, img, buf, 0x10);
    uint8_t z = rd8(s, RAM_gCurSubstateZ);
    drawImageToScreen(s, 0x2C, (int8_t)mem_rd8(s, ROM_BALL_DROP_Y + z), buf, 8, 8);
    draw_bars(s);
    wr8(s, RAM_gCurSubstateZ, rd8(s, RAM_gCurSubstateZ) + 1);
}

void drawPokeballInMiddleOfScreen(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    eepromReadToRamAlso(s, 0x0460, buf, 0x10);
    drawImageToScreen(s, 0x2C, 0x10, buf, 8, 8);
    eepromReadToRamAlso(s, 0x2040, buf, 0x10);
    uint16_t loc = ROM_ITEM_SCREEN_LOCS + rd8(s, RAM_gCurSubstateZ) * 2;
    drawImageToScreen(s, mem_rd8(s, loc), (int8_t)mem_rd8(s, loc + 1), buf, 8, 8);
    draw_bars(s);
    uint8_t z = rd8(s, RAM_gCurSubstateZ) + 1;
    if (z > 2) {
        wr8(s, RAM_gCurSubstateY, 1);
        z = 0;
    }
    wr8(s, RAM_gCurSubstateZ, z);
}

#define ROM_FEELING_IMAGE_MAP 0xBEB0
#define ROM_POKEWALKER_IMAGE  0xBB74
#define ROM_HAPPY_FACE        0xBC94
#define ROM_SAD_FACE          0xBCB4
#define ROM_IR_ACTIVE_ICON    0xBCE4

void drawHomeScreenMain(pw_state_t *s) {
    if (rd8(s, RAM_gCurSubstateZ) != 0) {
        uint16_t i = (uint16_t)(rd8(s, RAM_gCurSubstateY) - 1);
        drawPokemonFeelingBubble(s, mem_rd8(s, ROM_FEELING_IMAGE_MAP + i));
    }
    drawCurrentAreaSmallImage(s);
    if (!(rd8(s, RAM_walker_status_flags) & 4)) {
        return;
    }
    uint8_t x = rd8(s, RAM_gCurSubstateA);
    uint8_t f = rd8(s, RAM_gCurSubstateA + 1);
    if (!(f & 2)) {
        homeScreenDrawLargeCurrentPokemonImage(s, x, 0);
    } else if (!(f & 4)) {
        drawCurPokemonSmallAnimatedFrame(s, x, 0x18);
    } else {
        setHeapPointer(s);
        uint16_t buf = static_malloc(s, 0x10);
        eepromReadToRamAlso(s, 0x8F00, buf, 0x10);
        if (!(rd8(s, buf + 0xE) & 1)) {
            drawCurPokemonSmallAnimatedFrameFlipped(s, rd8(s, RAM_gCurSubstateA), 0x18);
        } else {
            drawCurPokemonSmallAnimatedFrame(s, rd8(s, RAM_gCurSubstateA), 0x18);
        }
    }
    if (rd8(s, RAM_gCurSubstateA + 1) & 2) {
        homeScreenPacePokemon(s);
    } else {
        homeScreenSlidePokemon(s);
    }
}

static void compose_pokewalker_face(pw_state_t *s, uint16_t buf, uint16_t face) {
    for (uint16_t i = 0; i < 0x100; i++) {
        wr8(s, buf + i, rom_rd8(ROM_POKEWALKER_IMAGE + i));
    }
    for (uint16_t i = 0; i < 0x20; i++) {
        uint8_t b = rom_rd8(face + i);
        uint16_t p = buf + 0x50 + i;
        wr8(s, p, rd8(s, p) | (uint8_t)(b << 3));
        wr8(s, p + 0x40, rd8(s, p + 0x40) | (b >> 5));
    }
}

static void draw_pokewalker_face(pw_state_t *s, uint16_t buf, uint16_t face) {
    compose_pokewalker_face(s, buf, face);
    drawImageToScreen(s, 32, 16, buf, 32, 32);
}

void drawSadPokewalkerImage(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x100);
    draw_pokewalker_face(s, buf, ROM_SAD_FACE);
    uint8_t z = rd8(s, RAM_gCurSubstateZ) + 1;
    wr8(s, RAM_gCurSubstateZ, z);
    if (z > 8) {
        resetSubstates(s);
        setCurrentlyActiveView(s, 0);
    }
}

void drawHappyPokewalker(pw_state_t *s, uint8_t ir_icon) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x100);
    if (ir_icon) {
        drawImageToScreen(s, 0x2C, 0, ROM_IR_ACTIVE_ICON, 8, 8);
    }
    draw_pokewalker_face(s, buf, ROM_HAPPY_FACE);
}

void drawConnectingScreen(pw_state_t *s, uint8_t ir_icon) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    if (ir_icon) {
        eepromReadToRamAlso(s, 0x2450, buf, 0x20);
        drawImageToScreen(s, 0x2C, 0, buf, 8, 16);
    }
    eepromReadToRamAlso(s, 0x2350, buf, 0x100);
    drawImageToScreen(s, 32, 16, buf, 32, 32);
    drawLargeMessage(s, 0x30, 0, 0x0F, 0);
}

void showGoodJobBanner(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x140);
    eepromReadToRamAlso(s, 0x0280, buf, 0x140);
    for (uint8_t i = 0; i < 7; i++) {
        drawImageToScreen(s, i * 8, 8, buf + 0x120, 8, 16);
    }
    eepromReadToRamAlso(s, 0x13D0, buf, 0xA0);
    drawImageToScreen(s, 0x38, 8, buf, 40, 16);
    eepromReadToRamAlso(s, 0x2490, buf, 0xA0);
    drawImageToScreen(s, 0x38, 0x28, buf, 40, 16);
    drawNumberOnscreen(s, 0x30, 0x28, rd32(s, RAM_RamCache_STEP_COUNT_maybe), 0);
    drawLargeMessage(s, 0x18, 0x43, 0x0F, 1);
}

void showCommsErrorMessage(pw_state_t *s) {
    static const uint8_t lines[8][2] = {
        { 0x00, 0x01 }, { 0x00, 0x04 }, { 0x02, 0x03 }, { 0x00, 0x15 },
        { 0x09, 0x0A }, { 0x07, 0x08 }, { 0x0B, 0x0C }, { 0x00, 0x04 },
    };
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    eepromReadToRamAlso(s, 0x2350, buf, 0x100);
    drawImageToScreen(s, 32, 16, buf, 32, 32);
    uint8_t id = rd8(s, RAM_commsErrorMessageId) - 1;
    if (id < 8) {
        if (lines[id][0]) {
            drawLargeMessage(s, 0x20, lines[id][0], 0x0D, 0);
            drawLargeMessage(s, 0x30, lines[id][1], 0x0E, 1);
        } else {
            drawLargeMessage(s, 0x30, lines[id][1], 0x0F, 1);
        }
    }
    drawLowBatteryIcon(s, 0x58, 0);
}

void drawPokemonArrivedAnimation(pw_state_t *s) {
    switch (rd8(s, RAM_gCurSubstateY)) {
    case 0: drawBallDropFromTopOfScreen(s); break;
    case 1: drawCloudWithBarsOnTopAndBottomOfScreen2(s); break;
    case 2: drawLargePokemonImageWithBars(s); break;
    case 3: pokemonArrivalScreen(s); break;
    }
    drawLowBatteryIcon(s, 0, 0);
}

void drawSwitchScreen(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    eepromReadToRamAlso(s, 0x05F8, buf, 0x20);
    drawImageToScreen(s, 0, 0, buf, 8, 16);
    eepromReadToRamAlso(s, 0x8B30, buf, 0x140);
    drawImageToScreen(s, 8, 0, buf, 80, 16);
    eepromReadToRamAlso(s, 0x0528, buf, 0x20);
    uint8_t z = rd8(s, RAM_gCurSubstateZ);
    drawImageToScreen(s, (uint8_t)(z * 0x14 + 0x18), 0x18, buf + (rd8(s, RAM_curUiFlags) & 1) * 0x10, 8, 8);
    uint8_t items = rd8(s, RAM_gCurSubstateA) != 0;
    eepromReadToRamAlso(s, items ? 0x0488 : 0x0460, buf, 0x10);
    drawImageToScreen(s, 0x18, 0x20, buf, 8, 8);
    drawImageToScreen(s, 0x2C, 0x20, buf, 8, 8);
    drawImageToScreen(s, 0x40, 0x20, buf, 8, 8);
    int8_t sz = (int8_t)rd8(s, RAM_gCurSubstateZ);
    if (sz < 0 || sz > 2) {
        return;
    }
    if (!items) {
        drawCurSelectedPokeNameForSwitchScreen(s);
    } else {
        drawCurSelectedItemNameForSwitchScreen(s);
    }
    drawLowBatteryIcon(s, 0x58, 0);
}

void showPokemonFeelingsAndRandomFindings(pw_state_t *s) {
    setHeapPointer(s);
    static_malloc(s, 0xC0);
    uint16_t rec = rd16(s, RAM_gCurSubstateB);
    if (mem_rd8(s, rec) & 2) {
        drawCurPokemonSmallAnimatedFrame(s, 0x20, 4);
    }
    rec = rd16(s, RAM_gCurSubstateB);
    if ((mem_rd8(s, rec) & 0xE0) != 0xE0) {
        drawPokemonFeelingBubble(s, mem_rd8(s, rec) >> 5);
    }
    rec = rd16(s, RAM_gCurSubstateB);
    if (mem_rd8(s, rec) & 4) {
        drawItemSymbolOnScreen(s, 0x14, 0x14);
    }
    uint8_t z = rd8(s, RAM_gCurSubstateZ);
    rec = rd16(s, RAM_gCurSubstateB);
    uint8_t what = mem_rd8(s, rec + 2);
    switch (what) {
    case 0xFC: drawCurPokemonNameWithBorders(s, 0, 0x20, 5); break;
    case 0xFD: drawRouteAvailableItemName(s, 0, 0x20, z, 0x0D); break;
    case 0xFE: drawNumberAndWattsymbolOnscreen(s, 2, 0x20, z); break;
    case 0xFF: break;
    default: drawLargeMessage(s, 0x20, what, 0x0D, 0); break;
    }
    rec = rd16(s, RAM_gCurSubstateB);
    uint8_t borders = 0x0E;
    uint8_t msg = mem_rd8(s, rec + 3);
    if ((mem_rd8(s, rec) & 0x18) > 8) {
        msg += rd8(s, RAM_gCurSubstateA);
    } else if (mem_rd8(s, rec + 2) == 0xFF) {
        borders = 0x0F;
    }
    drawLargeMessage(s, 0x30, msg, borders, 1);
    drawLowBatteryIcon(s, 0, 0);
}

#define ROM_NEUTRAL_FACE  0xBC74
#define ROM_UP_ARROW_ICON 0xBCD4

void drawPokewalkerInitialConnectionInstructionScreen(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x100);
    compose_pokewalker_face(s, buf, ROM_NEUTRAL_FACE);
    if (!(rd8(s, RAM_curUiFlags) >> 2 & 1)) {
        drawImageToScreen(s, 32, 16, buf, 32, 32);
        return;
    }
    for (uint16_t i = 0; i < 0x10; i++) {
        uint16_t p = buf + 0xD8 + i;
        wr8(s, p, rd8(s, p) | (uint8_t)(rom_rd8(ROM_UP_ARROW_ICON + i) << 4));
    }
    drawImageToScreen(s, 32, 16, buf, 32, 32);
    for (uint16_t i = 0; i < 0x10; i++) {
        wr8(s, buf + i, rom_rd8(ROM_UP_ARROW_ICON + i) >> 4);
    }
    drawImageToScreen(s, 0x2C, 0x30, buf, 8, 8);
}

#define ROM_MAIN_MENU_ICON_Y 0xBF14

void mainMenuDraw(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x140);
    uint16_t cell = static_malloc(s, 0x80);
    uint8_t cur = rd8(s, RAM_menuCurItem);
    eepromReadToRamAlso(s, (uint16_t)(0x0910 + cur * 0x140), buf, 0x140);
    drawImageToScreen(s, 8, 0, buf, 80, 16);
    for (uint16_t i = 0; i < 6; i++) {
        for (uint16_t k = 0; k < 0x80; k++) {
            wr8(s, cell + k, 0);
        }
        uint8_t y = rom_rd8(ROM_MAIN_MENU_ICON_Y + i);
        if (rd8(s, RAM_menuCurItem) == i) {
            uint16_t frame = 3 + (rd8(s, RAM_curUiFlags) & 1);
            eepromReadToRamAlso(s, 0x04F8 + frame * 0x10, buf, 0x10);
            drawImageToBuffer(s, 8, 8, buf, 4, y - 8, cell, 16);
        }
        eepromReadToRamAlso(s, 0x1090 + i * 0x40, buf, 0x40);
        drawImageToBuffer(s, 16, 16, buf, 0, y, cell, 16);
        drawImageToScreen(s, i * 16, 16, cell, 16, 32);
    }
    switch (rd8(s, RAM_gCurSubstateY)) {
    case 0:
        drawNumberOnscreen(s, 0x48, 0x30, rd16(s, RAM_RamCache_curWatts), 0);
        cur = rd8(s, RAM_menuCurItem);
        if (cur == 0) {
            drawNumberOnscreen(s, 8, 0x30, 10, 0);
        } else if (cur == 1) {
            drawNumberOnscreen(s, 8, 0x30, 3, 0);
        }
        eepromReadToRamAlso(s, 0x0420, buf, 0x40);
        drawImageToScreen(s, 0x50, 0x30, buf, 16, 16);
        if (rd8(s, RAM_menuCurItem) < 2) {
            drawImageToScreen(s, 0x18, 0x30, buf, 16, 16);
            eepromReadToRamAlso(s, 0x0400, buf, 0x20);
            drawImageToScreen(s, 0x28, 0x30, buf, 8, 16);
        }
        break;
    case 1: drawLargeMessage(s, 0x30, 0x14, 0x0F, 1); break;
    case 2: drawLargeMessage(s, 0x30, 0x15, 0x0F, 1); break;
    case 3: drawLargeMessage(s, 0x30, 0x16, 0x0F, 1); break;
    }
    eepromReadToRamAlso(s, 0x05B8, buf, 0x40);
    drawImageToScreen(s, 0, 0, buf, 8, 16);
    drawImageToScreen(s, 0x58, 0, buf + 0x20, 8, 16);
    drawLowBatteryIcon(s, 0x58, 0);
}

void drawSettingsScreen(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x140);
    eepromReadToRamAlso(s, 0x0F50, buf, 0x140);
    drawImageToScreen(s, 8, 0, buf, 80, 16);
    eepromReadToRamAlso(s, 0x1690, buf, 0xA0);
    drawImageToScreen(s, 8, 0x10, buf, 40, 16);
    eepromReadToRamAlso(s, 0x1730, buf, 0xA0);
    drawImageToScreen(s, 0x38, 0x10, buf, 40, 16);
    eepromReadToRamAlso(s, 0x04F8, buf, 0xC0);
    uint8_t zx = rd8(s, RAM_gCurSubstateZ) * 0x30;
    uint8_t blink = rd8(s, RAM_curUiFlags) & 1;
    switch (rd8(s, RAM_gCurSubstateY)) {
    case 0: drawImageToScreen(s, zx, 0x14, buf + (9 + blink) * 0x10, 8, 8); break;
    case 1: {
        drawImageToScreen(s, zx, 0x14, buf + 0xB0, 8, 8);
        uint8_t vol = rd8(s, RAM_RamCache_settingsByte) >> 1 & 3;
        blink = rd8(s, RAM_curUiFlags) & 1;
        drawImageToScreen(s, (uint8_t)(vol * 0x20), 0x2C, buf + (9 + blink) * 0x10, 8, 8);
        eepromReadToRamAlso(s, 0x17D0, buf, 0x120);
        drawImageToScreen(s, 0x08, 0x28, buf, 24, 16);
        drawImageToScreen(s, 0x28, 0x28, buf + 0x60, 24, 16);
        drawImageToScreen(s, 0x48, 0x28, buf + 0xC0, 24, 16);
        break;
    }
    case 2: {
        uint8_t shade = rd8(s, RAM_RamCache_settingsByte) >> 3 & 0xF;
        blink = rd8(s, RAM_curUiFlags) & 1;
        drawImageToScreen(s, (uint8_t)(shade * 8 + 8), 0x20, buf + (3 + blink) * 0x10, 8, 8);
        drawImageToScreen(s, (uint8_t)(rd8(s, RAM_gCurSubstateZ) * 0x30), 0x14, buf + 0xB0, 8, 8);
        eepromReadToRamAlso(s, 0x18F0, buf, 0x20);
        for (uint8_t i = 0; i < 10; i++) {
            drawImageToScreen(s, i * 8 + 8, 0x28, buf, 8, 16);
        }
        break;
    }
    }
    eepromReadToRamAlso(s, 0x05F8, buf, 0x20);
    drawImageToScreen(s, 0, 0, buf, 8, 16);
    drawLowBatteryIcon(s, 0x58, 0);
}

void drawStepStatisticsScreen(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x140);
    eepromReadToRamAlso(s, 0x05B8, buf, 0xC0);
    drawImageToScreen(s, 0, 0, buf, 8, 16);
    if (rd8(s, RAM_gCurSubstateY) < 7) {
        drawImageToScreen(s, 0x58, 0, buf + 0x20, 8, 16);
    }
    eepromReadToRamAlso(s, 0x14F0, buf, 0xA0);
    drawImageToScreen(s, 0x28, 0, buf, 40, 16);
    eepromReadToRamAlso(s, 0x1590, buf, 0x100);
    drawImageToScreen(s, 0, 0x20, buf, 64, 16);
    eepromReadToRamAlso(s, 0x13D0, buf, 0xA0);
    drawImageToScreen(s, 0x38, 0x10, buf, 40, 16);
    drawImageToScreen(s, 0x38, 0x30, buf, 40, 16);
    eepromReadToRamAlso(s, 0x03E0, buf, 0x20);
    drawImageToScreen(s, 0x18, 0, buf, 8, 16);
    eepromReadToRamAlso(s, 0x0280, buf, 0x140);
    uint8_t day = rd8(s, RAM_gCurSubstateY);
    drawImageToScreen(s, 0x20, 0, buf + day * 0x20, 8, 16);

    uint16_t at = (uint16_t)(0xCEF0 + (uint16_t)(day - 1) * 4);
    uint32_t steps = 0;
    for (int i = 0; i < 4; i++) {
        steps = steps << 8 | eepromReadSingleByte(s, at + i);
    }
    drawNumberOnscreen(s, 0x30, 0x10, steps, 0);
    drawNumberOnscreen(s, 0x58, 0x20, rd16(s, RAM_RamCache_totalDays), 0);
    drawNumberOnscreen(s, 0x30, 0x30, rd32(s, RAM_RamCache_totalSteps), 0);
}

#define ROM_RADAR_SPARKLE_FRAME 0xBF21

void pokeRadarDraw(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x100);
    uint16_t arrow = 0x04F8 + (9 + (rd8(s, RAM_curUiFlags) & 1)) * 0x10;
    eepromReadToRamAlso(s, arrow, buf, 0x10);
    uint8_t a = rd8(s, RAM_gCurSubstateA);
    drawImageToScreen(s, (uint8_t)(mem_rd8(s, ROM_BUSH_X + a) - 8), (int8_t)((a & 1) * 0x18 + 8), buf, 8, 8);
    eepromReadToRamAlso(s, 0x1CB0, buf, 0xC0);
    for (uint8_t i = 0; i < 4; i++) {
        drawImageToScreen(s, rom_rd8(ROM_BUSH_X + i), (int8_t)((i & 1) * 24), buf, 32, 24);
    }
    if (rd8(s, RAM_gCurSubstateE) != 0) {
        eepromReadToRamAlso(s, 0x1D70, buf, 0x100);
        uint8_t b = rd8(s, RAM_gCurSubstateB + 1);
        drawImageToScreen(s, (uint8_t)(mem_rd8(s, ROM_BUSH_X + b) + 0x10), (int8_t)((b & 1) * 0x18), buf + 0xC0, 16,
                          16);
        switch (rd8(s, RAM_gCurSubstateZ)) {
        case 3: drawLargeMessage(s, 0x30, 0x1D, 0x0F, 0); break;
        case 1: {
            uint8_t h = rd8(s, RAM_gCurSubstateD) * 8;
            lcdFillRectangle(s, 0, 0, 0x60, h, 3);
            h = rd8(s, RAM_gCurSubstateD) * 8;
            lcdFillRectangle(s, 0, (uint8_t)(0x40 - h), 0x60, h, 3);
            wr8(s, RAM_gCurSubstateD, rd8(s, RAM_gCurSubstateD) + 1);
            break;
        }
        case 2:
            drawNumberAndWattsymbolOnscreen(s, 2, 0x20, rd8(s, RAM_gCurSubstateY));
            drawLargeMessage(s, 0x30, 0x0F, 0x0E, 1);
            break;
        }
    } else {
        drawLargeMessage(s, 0x30, 0x1C, 0x0F, 0);
        if (rd8(s, RAM_gCurSubstateC) == 0) {
            eepromReadToRamAlso(s, 0x1D70, buf, 0x100);
            uint8_t frame = mem_rd8(s, ROM_RADAR_SPARKLE_FRAME + rd8(s, RAM_gCurSubstateA + 1));
            uint8_t b = rd8(s, RAM_gCurSubstateB + 1);
            drawImageToScreen(s, (uint8_t)(mem_rd8(s, ROM_BUSH_X + b) + 0x10), (int8_t)((b & 1) * 0x18),
                              buf + frame * 0x40, 16, 16);
        }
    }
    drawLowBatteryIcon(s, 0, 0);
}

static void draw_route_item_name_for(pw_state_t *s, uint16_t item) {
    uint16_t items = static_malloc(s, 0x14);
    eepromReadToRamAlso(s, 0x8F8C, items, 0x14);
    for (uint8_t i = 0; i < 10; i++) {
        if (rd16(s, items + i * 2) == item) {
            drawRouteAvailableItemName(s, 0, 0x30, i, 0x0F);
            return;
        }
    }
}

static uint16_t read_item_slot_id(pw_state_t *s, uint16_t at) {
    uint16_t item = eepromReadSingleByte(s, at) << 8;
    item |= eepromReadSingleByte(s, at + 1);
    eepromReadSingleByte(s, at + 2);
    eepromReadSingleByte(s, at + 3);
    return item;
}

void giftedItemsView(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x140);
    eepromReadToRamAlso(s, 0x05B8, buf, 0x20);
    drawImageToScreen(s, 0, 0, buf, 8, 16);
    eepromReadToRamAlso(s, 0x0E10, buf, 0x140);
    drawImageToScreen(s, 8, 0, buf, 80, 16);
    eepromReadToRamAlso(s, 0x1A90, buf, 0xC0);
    drawImageToScreen(s, 0x3C, 0x18, buf, 32, 24);
    eepromReadToRamAlso(s, 0x04F8 + (3 + (rd8(s, RAM_curUiFlags) & 1)) * 0x10, buf, 0x10);
    uint8_t y = rd8(s, RAM_gCurSubstateY);
    drawImageToScreen(s, (uint8_t)(y % 5 * 8 + 0x10), (int8_t)(uint8_t)(y / 5 * 0x10 + 0x10), buf, 8, 8);
    eepromReadToRamAlso(s, 0x0488, buf, 0x10);
    for (uint8_t i = 0; i < 5; i++) {
        if (rd16(s, RAM_gCurSubstateB) & (1 << i)) {
            drawImageToScreen(s, i * 8 + 0x10, 0x18, buf, 8, 8);
        }
    }
    for (uint8_t i = 0; i < 5; i++) {
        if (rd16(s, RAM_gCurSubstateB) & (0x20 << i)) {
            drawImageToScreen(s, i * 8 + 0x10, 0x28, buf, 8, 8);
        }
    }
    draw_route_item_name_for(s, read_item_slot_id(s, (uint16_t)(0xCEC8 + rd8(s, RAM_gCurSubstateY) * 4)));
    drawLowBatteryIcon(s, 0x58, 0);
}

void drawPokemonAndItemsScreen(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    eepromReadToRamAlso(s, 0x0E10, buf, 0x140);
    drawImageToScreen(s, 8, 0, buf, 80, 16);
    uint8_t y = rd8(s, RAM_gCurSubstateY);
    switch (y) {
    case 0:
        drawCurPokemonSmallAnimatedFrame(s, 0x3C, 0x18);
        drawCurPokemonNameWithBorders(s, 0, 0x30, 7);
        break;
    case 1:
    case 2:
    case 3: {
        uint8_t slot = y - 1;
        eepromReadToRamAlso(s, 0x8F52, buf, 0x30);
        eepromReadToRamAlso(s, 0xCE8C + slot * 0x10, buf + 0x30, 0x10);
        for (uint8_t i = 0; i < 3; i++) {
            if (rd16(s, buf + i * 0x10) == rd16(s, buf + 0x30)) {
                drawRouteAvailablePokemon(s, 0x3C, 0x18, i);
                drawNameOfRouteAvailablePokemon(s, 0, 0x30, i, 7);
                break;
            }
        }
        break;
    }
    case 4:
        eepromReadToRamAlso(s, 0xBA80 + (rd8(s, RAM_curUiFlags) & 1) * 0xC0, buf, 0x180);
        drawImageToScreen(s, 0x3C, 0x18, buf, 32, 24);
        showEventPokeName(s, 0, 0x30, 7);
        break;
    case 5:
        eepromReadToRamAlso(s, 0x19D0, buf, 0xC0);
        drawImageToScreen(s, 0x3C, 0x18, buf, 32, 24);
        drawLargeMessage(s, 0x30, 0x11, 0x0F, 0);
        break;
    case 6:
    case 7:
    case 8:
        drawLargeItemChest(s, 0x3C, 0x18);
        draw_route_item_name_for(s, read_item_slot_id(s, 0xCEBC + (y - 6) * 4));
        break;
    case 9:
        eepromReadToRamAlso(s, 0x1910, buf, 0xC0);
        drawImageToScreen(s, 0x3C, 0x18, buf, 32, 24);
        showRxedItemName(s, 0, 0x30, 0x0F);
        break;
    }
    eepromReadToRamAlso(s, 0x04F8 + (3 + (rd8(s, RAM_curUiFlags) & 1)) * 0x10, buf, 0x10);
    y = rd8(s, RAM_gCurSubstateY);
    uint8_t ax = y % 5 * 8 + 0x10;
    if (y == 0) {
        ax -= 8;
    }
    drawImageToScreen(s, ax, (int8_t)(uint8_t)(y / 5 * 0x10 + 0x10), buf, 8, 8);
    eepromReadToRamAlso(s, 0x05D8, buf, 0x40);
    drawImageToScreen(s, 0, 0, buf + 0x20, 8, 16);
    if (rd16(s, RAM_gCurSubstateB)) {
        drawImageToScreen(s, 0x58, 0, buf, 8, 16);
    }
    eepromReadToRamAlso(s, 0x0460, buf, 0x10);
    if (rd16(s, RAM_gCurSubstateA) & 1) {
        drawImageToScreen(s, 8, 0x18, buf, 8, 8);
    }
    for (uint8_t i = 0; i < 3; i++) {
        if (rd16(s, RAM_gCurSubstateA) & (2 << i)) {
            drawImageToScreen(s, i * 8 + 0x18, 0x18, buf, 8, 8);
        }
    }
    eepromReadToRamAlso(s, 0x0488, buf, 0x10);
    for (uint8_t i = 0; i < 3; i++) {
        if (rd16(s, RAM_gCurSubstateA) & (0x40 << i)) {
            drawImageToScreen(s, i * 8 + 0x18, 0x28, buf, 8, 8);
        }
    }
    if (rd16(s, RAM_gCurSubstateA) & 0x10) {
        eepromReadToRamAlso(s, 0x0470, buf, 0x10);
        drawImageToScreen(s, 0x30, 0x18, buf, 8, 8);
    }
    if (rd16(s, RAM_gCurSubstateA) & 0x200) {
        eepromReadToRamAlso(s, 0x0498, buf, 0x10);
        drawImageToScreen(s, 0x30, 0x28, buf, 8, 8);
    }
    if (rd16(s, RAM_gCurSubstateA) & 0x20) {
        eepromReadToRamAlso(s, 0x04A8, buf, 0x10);
        drawImageToScreen(s, 0x10, 0x28, buf, 8, 8);
    }
    drawLowBatteryIcon(s, 0x58, 0);
}

void drawSomethingDepartingAndCompletedMessage(pw_state_t *s) {
    switch (rd8(s, RAM_gCurSubstateY)) {
    case 2: drawLargePokemonImageWithBars(s); break;
    case 1: drawCloudWithBarsOnTopAndBottomOfScreen(s); break;
    case 0: drawMovingBlankIconWithBarsOnTopAndBottom(s); break;
    case 3: pokemonDepartureScreen(s); break;
    case 6:
    case 7: drawPokewalkerAndCompletedMessage(s); break;
    }
    drawLowBatteryIcon(s, 0, 0);
}

void drawItemMapStampOrRouteReceived(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    uint8_t what = rd8(s, RAM_gCurSubstateA);
    uint8_t msg = 0;
    static const uint16_t stamps[4] = { 0x4B8, 0x4C8, 0x4D8, 0x4E8 };
    switch (what) {
    case 0:
        eepromReadToRamAlso(s, 0xBA80 + (rd8(s, RAM_curUiFlags) & 1) * 0xC0, buf, 0x180);
        drawImageToScreen(s, 0x20, 8, buf, 0x20, 0x18);
        showEventPokeName(s, 0, 0x20, 5);
        break;
    case 1:
    case 3:
        eepromReadToRamAlso(s, 0x19D0, buf, 0xC0);
        drawImageToScreen(s, 0x20, 4, buf, 0x20, 0x18);
        msg = what == 1 ? 0x13 : 0x11;
        break;
    case 2:
        drawLargeItemChest(s, 0x20, 4);
        showRxedItemName(s, 0, 0x20, 0xD);
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        eepromReadToRamAlso(s, stamps[what - 4], buf, 0x10);
        drawImageToScreen(s, 0x2C, 0x10, buf, 8, 8);
        msg = 0x12;
        break;
    }
    if (msg) {
        drawLargeMessage(s, 0x20, msg, 0xD, 0);
    }
    drawLargeMessage(s, 0x30, 0xF, 0xE, 0);
    if (rd8(s, RAM_gCurSubstateZ) < 0x10) {
        wr8(s, RAM_gCurSubstateZ, rd8(s, RAM_gCurSubstateZ) + 1);
    }
}

#define ROM_DOWSING_BUSH_SHAKE 0xBD82

void dowsingScreen(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    eepromReadToRamAlso(s, 0x280 + rd8(s, RAM_gCurSubstateB) * 0x20, buf, 0x20);
    drawImageToScreen(s, 0x40, 0, buf, 8, 0x10);
    eepromReadToRamAlso(s, 0x1BD0, buf, 0x80);
    drawImageToScreen(s, 0x20, 0, buf, 0x20, 0x10);
    eepromReadToRamAlso(s, 0x1C50, buf, 0x60);
    drawImageToScreen(s, 0x48, 0, buf, 0x18, 0x10);
    eepromReadToRamAlso(s, (rd8(s, RAM_RamCache_settingsByte) & 1) ? 0xC83C : 0x8FBE, buf, 0xC0);
    drawImageToScreen(s, 0, 0, buf, 0x20, 0x18);
    if (rd8(s, RAM_gCurSubstateA)) {
        wr8(s, RAM_gCurSubstateA, rd8(s, RAM_gCurSubstateA) - 1);
    }
    eepromReadToRamAlso(s, 0x1B50, buf, 0x80);
    uint8_t shaking = rd8(s, RAM_gCurSubstateA + 1);
    for (uint16_t i = 0; i < 6; i++) {
        if (i == rd8(s, RAM_gCurSubstateC)) {
            drawImageToScreen(s, i * 0x10, 0x18, buf + 0x40, 0x10, 0x10);
        } else if (i != shaking) {
            drawImageToScreen(s, i * 0x10, 0x18, buf, 0x10, 0x10);
        }
    }
    moveImageVerticaly(s, 0x10, 0x10, buf, (int8_t)rom_rd8(ROM_DOWSING_BUSH_SHAKE + (rd8(s, RAM_curUiFlags) & 3)));
    drawImageToScreen(s, (uint8_t)(shaking * 0x10), 0x18, buf, 0x10, 0x10);
    drawLargeMessage(s, 0x30, 0x17, 0xF, 0);
}

void drawSomethingReceivedAnimation(pw_state_t *s) {
    switch (rd8(s, RAM_gCurSubstateY)) {
    case 0: drawBallDropFromTopOfScreen(s); break;
    case 4: drawPokeballInMiddleOfScreen(s); break;
    case 1: drawCloudWithBarsOnTopAndBottomOfScreen2(s); break;
    case 3: drawItemMapStampOrRouteReceived(s); break;
    }
    drawLowBatteryIcon(s, 0, 0);
}

static void overline(pw_state_t *s, uint16_t img, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        wr8(s, img + i, rd8(s, img + i) | 1);
    }
}

void drawHomeScreenSmallIconsOnBottom(pw_state_t *s) {
    drawLineAtRow48(s);
    uint8_t flags = eepromReadSingleByte(s, 0xB800);
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    if (flags & 0x20) {
        eepromReadToRamAlso(s, 0x470, buf, 0x10);
        overline(s, buf, 0x10);
        drawImageToScreen(s, 0, 0x30, buf, 8, 8);
    }
    if (flags & 0x40) {
        eepromReadToRamAlso(s, 0xBD40, buf, 8);
        if (rd16(s, buf + 6)) {
            eepromReadToRamAlso(s, 0x498, buf, 0x10);
            overline(s, buf, 0x10);
            drawImageToScreen(s, 8, 0x30, buf, 8, 8);
        }
    }
    eepromReadToRamAlso(s, 0x4B8, buf, 0x40);
    overline(s, buf, 0x40);
    for (uint8_t i = 0; i < 4; i++) {
        if (flags & 1 << i) {
            drawImageToScreen(s, 0x10 + i * 8, 0x30, buf + i * 0x10, 8, 8);
        }
    }
    uint16_t list = buf + 0x10;
    eepromReadToRamAlso(s, 0x460, buf, 0x10);
    eepromReadToRamAlso(s, 0xCE8C, list, 0x30);
    for (uint8_t i = 0; i < 3; i++) {
        if (rd16(s, list + i * 0x10)) {
            drawImageToScreen(s, i * 8, 0x38, buf, 8, 8);
        }
    }
    eepromReadToRamAlso(s, 0x488, buf, 0x10);
    eepromReadToRamAlso(s, 0xCEBC, list, 0xC);
    for (uint8_t i = 0; i < 3; i++) {
        if (rd16(s, list + i * 4)) {
            drawImageToScreen(s, 0x18 + i * 8, 0x38, buf, 8, 8);
        }
    }
    if (flags & 0x10) {
        eepromReadToRamAlso(s, 0x4A8, buf, 0x10);
        drawImageToScreen(s, 0x30, 0x38, buf, 8, 8);
    }
    drawNumberOnscreen(s, 0x58, 0x30, rd32(s, RAM_stepCountTodaySoFar), 1);
    drawLowBatteryIcon(s, 0, 0);
}

void dowsingDraw(pw_state_t *s) {
    uint8_t phase = rd8(s, RAM_gCurSubstateZ);
    if (phase == 1) {
        dowsingScreen(s);
        drawLowBatteryIcon(s, 0, 0);
        return;
    }
    setHeapPointer(s);
    uint16_t icons = static_malloc(s, 0x140);
    uint16_t buf = static_malloc(s, 0x180);
    eepromReadToRamAlso(s, 0x280, icons, 0x140);
    drawImageToScreen(s, 0x40, 0, (uint16_t)(icons + rd8(s, RAM_gCurSubstateB) * 0x20), 8, 0x10);
    eepromReadToRamAlso(s, 0x1BD0, buf, 0x80);
    drawImageToScreen(s, 0x20, 0, buf, 0x20, 0x10);
    eepromReadToRamAlso(s, 0x1C50, buf, 0x60);
    drawImageToScreen(s, 0x48, 0, buf, 0x18, 0x10);
    eepromReadToRamAlso(s, (rd8(s, RAM_RamCache_settingsByte) & 1) ? 0xC83C : 0x8FBE, buf, 0xC0);
    drawImageToScreen(s, 0, 0, buf, 0x20, 0x18);
    if (rd8(s, RAM_gCurSubstateA)) {
        wr8(s, RAM_gCurSubstateA, rd8(s, RAM_gCurSubstateA) - 1);
    }
    eepromReadToRamAlso(s, 0x1B50, buf, 0x80);
    uint8_t chosen = rd8(s, RAM_gCurSubstateA + 1);
    for (uint16_t i = 0; i < 6; i++) {
        if (rd8(s, RAM_gCurSubstateZ) == 2 && i == chosen) {
            continue;
        }
        uint16_t img = i == rd8(s, RAM_gCurSubstateC) ? buf + 0x40 : buf;
        drawImageToScreen(s, (uint8_t)(i * 0x10), 0x18, img, 0x10, 0x10);
    }
    uint8_t hint = rd8(s, RAM_gCurSubstateB + 1);
    switch (phase) {
    case 0:
        eepromReadToRamAlso(s, 0x4F8 + (rd8(s, RAM_curUiFlags) & 1) * 0x10, buf, 0x10);
        drawImageToScreen(s, (uint8_t)(chosen * 0x10 + 4), 0x28, buf, 8, 8);
        drawLargeMessage(s, 0x30, 0x17, 0xF, 0);
        break;
    case 2:
        eepromReadToRamAlso(s, 0x488, buf, 0x10);
        drawImageToScreen(s, (uint8_t)(chosen * 0x10 + 4), 0x18, buf, 8, 8);
        if (rd8(s, RAM_gCurSubstateD)) {
            drawNumberAndWattsymbolOnscreen(s, 2, 0x20, rd8(s, RAM_gCurSubstateD));
            drawLargeMessage(s, 0x30, 0xF, 0xE, 1);
            break;
        }
        if (rd8(s, RAM_gCurSubstateY) >= 0xA) {
            showRxedItemName(s, 0, 0x20, 0xD);
        } else {
            drawRouteAvailableItemName(s, 0, 0x20, rd8(s, RAM_gCurSubstateY), 0xD);
        }
        drawLargeMessage(s, 0x30, 0x18, 0xE, 1);
        break;
    case 3:
        drawLargeMessage(s, 0x30, 0x19, 0xF, 1);
        if (rd8(s, RAM_gCurSubstateB)) {
            break;
        }
        eepromReadToRamAlso(s, 0x488, buf, 0x10);
        for (int k = 0; k < 3; k++) {
            drawImageToScreen(s, (uint8_t)(hint * 0x10 + 4), 0x16, buf, 8, 8);
        }
        break;
    case 4: {
        int16_t d = (int16_t)(chosen - hint);
        if (d < 0)
            d = -d;
        drawLargeMessage(s, 0x30, d < 2 ? 0x1A : 0x1B, 0xF, 1);
        break;
    }
    }
    drawLowBatteryIcon(s, 0, 0);
}

void hex4(char out[5], uint16_t v) {
    static const char digits[] = "0123456789ABCDEF";
    out[0] = digits[v >> 12];
    out[1] = digits[v >> 8 & 0xF];
    out[2] = digits[v >> 4 & 0xF];
    out[3] = digits[v & 0xF];
    out[4] = 0;
}

void drawSomethingWithTinyChars2(pw_state_t *s) {
    char str[6] = { 0 };
    wr8(s, RAM_byte_FFF7AF, 0x3C);
    wr8(s, RAM_byte_FFF7B0, 0x1E);
    str[0] = (char)(rd8(s, RAM_gCurSubstateA) + '0');
    draw_tiny_string(s, 0x4C, 0, str);
    str[0] = (char)(rd8(s, RAM_gCurSubstateF) + '0');
    draw_tiny_string(s, 0x54, 0, str);
    hex4(str, rd16(s, RAM_word_FFF7DA));
    draw_tiny_string(s, 0x20, 8, str);
    hex4(str, rd16(s, RAM_word_FFF7DC));
    draw_tiny_string(s, 0x40, 8, str);
    hex4(str, rd16(s, RAM_word_FFF7DE));
    draw_tiny_string(s, 0x20, 0x10, str);
    str[1] = 0;
    str[0] = (char)(rd8(s, RAM_gCurSubstateA + 1) + '0');
    draw_tiny_string(s, 0x4C, 0x10, str);
    str[0] = (char)(rd8(s, RAM_gCurSubstateF + 1) + '0');
    draw_tiny_string(s, 0x54, 0x10, str);
    if (rd8(s, RAM_gCurSubstateA + 1) == rd8(s, RAM_gCurSubstateF + 1)) {
        lcdc_command(s, 0xA7);
        draw_tiny_string(s, 8, 0x20, "OK");
    }
}

static void draw_bcd(pw_state_t *s, uint8_t x, uint8_t y, uint16_t digits, uint8_t bcd) {
    drawImageToScreen(s, x, y, digits + (bcd >> 4 & 7) * 0x20, 8, 0x10);
    drawImageToScreen(s, x + 8, y, digits + (bcd & 0xF) * 0x20, 8, 0x10);
}

void drawTrainerCard(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x140);
    eepromReadToRamAlso(s, 0xCD0, buf, 0x140);
    drawImageToScreen(s, 8, 0, buf, 0x50, 0x10);
    eepromReadToRamAlso(s, 0x1210, buf, 0x40);
    drawImageToScreen(s, 0, 0x10, buf, 0x10, 0x10);
    eepromReadToRamAlso(s, 0x1250, buf, 0x140);
    drawImageToScreen(s, 0x10, 0x10, buf, 0x50, 0x10);
    eepromReadToRamAlso(s, 0x1390, buf, 0x40);
    drawImageToScreen(s, 0, 0x20, buf, 0x10, 0x10);
    eepromReadToRamAlso(s, (rd8(s, RAM_RamCache_settingsByte) & 1) ? 0xC8FC : 0x907E, buf, 0x140);
    drawImageToScreen(s, 0x10, 0x20, buf, 0x50, 0x10);
    eepromReadToRamAlso(s, 0x5B8, buf, 0xC0);
    drawImageToScreen(s, 0, 0, buf + 0x40, 8, 0x10);
    drawImageToScreen(s, 0x58, 0, buf + 0x20, 8, 0x10);
    eepromReadToRamAlso(s, 0x1470, buf, 0x80);
    drawImageToScreen(s, 0, 0x30, buf, 0x20, 0x10);
    eepromReadToRamAlso(s, 0x280, buf, 0x140);
    draw_bcd(s, 0x20, 0x30, buf, rd8(s, RAM_currentHour));
    draw_bcd(s, 0x38, 0x30, buf, rd8(s, RAM_currentMinute));
    draw_bcd(s, 0x50, 0x30, buf, rd8(s, RAM_currentSecond));
    eepromReadToRamAlso(s, 0x3C0, buf, 0x20);
    drawImageToScreen(s, 0x30, 0x30, buf, 8, 0x10);
    drawImageToScreen(s, 0x48, 0x30, buf, 8, 0x10);
}

void drawTrainerCardAndStatsScreens(pw_state_t *s) {
    switch (rd8(s, RAM_gCurSubstateZ)) {
    case 0:
        if (!rd8(s, RAM_gCurSubstateY)) {
            drawTrainerCard(s);
        } else {
            drawStepStatisticsScreen(s);
        }
        break;
    case 1: showGoodJobBanner(s); break;
    case 2: drawRewardReceivedBanner(s); break;
    }
    drawLowBatteryIcon(s, 0x58, 0);
}

#define ROM_SINE 0xBDD0

static void draw_wild_name(pw_state_t *s) {
    uint8_t y = rd8(s, RAM_gCurSubstateY);
    if (y < 4) {
        drawNameOfRouteAvailablePokemon(s, 0, 0x20, y - 1, 5);
    } else {
        drawNameOfEventPokemon(s, 0, 0x20, 5);
    }
}

static void draw_attack_star(pw_state_t *s, uint16_t buf, uint16_t img, uint8_t sound) {
    if (rd8(s, RAM_gCurSubstateB) == 4) {
        playSound(s, sound);
        eepromReadToRamAlso(s, img, buf, 0x80);
        drawImageToScreen(s, 0x28, 0, buf, 0x10, 0x20);
    }
}

void battleScreenDraw(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x300);
    uint16_t ball = static_malloc(s, 0x18);
    uint16_t ball_mask = static_malloc(s, 8);
    uint8_t frame_bit = rd8(s, RAM_curUiFlags) & 1;
    eepromReadToRamAlso(s, 0x91BE + frame_bit * 0xC0, buf, 0xC0);
    drawImageToScreen(s, rd8(s, RAM_gCurSubstateC), 8, buf, 0x20, 0x18);
    eepromReadToRamAlso(s, 0x2030, buf, 0x10);
    for (uint8_t i = 0; i < rd8(s, RAM_gCurSubstateA); i++) {
        drawImageToScreen(s, (uint8_t)(i * 8 + 0x38), 0, buf, 8, 8);
    }
    if (rd8(s, RAM_gCurSubstateF) & 1) {
        for (uint8_t i = 0; i < rd8(s, RAM_gCurSubstateA + 1); i++) {
            drawImageToScreen(s, (uint8_t)(i * 8 + 8), 0x18, buf, 8, 8);
        }
    }
    uint8_t z = rd8(s, RAM_gCurSubstateZ);
    if (z <= 0xA) {
        uint8_t who = rd8(s, RAM_gCurSubstateY);
        uint16_t img;
        if (who < 4) {
            eepromReadToRamAlso(s, (uint16_t)(0x8F52 + (uint16_t)(who - 1) * 0x10), buf, 0x10);
            img = (uint16_t)(0x9A7E + (uint16_t)(who - 1) * 0x180 + frame_bit * 0xC0);
        } else {
            eepromReadToRamAlso(s, 0xBF08, buf, 0x10);
            img = (uint16_t)(0xBF7C + frame_bit * 0xC0);
        }
        uint8_t facing = rd8(s, buf + 0xE) & 1;
        eepromReadToRamAlso(s, img, buf, 0x180);
        if (!facing) {
            imageFlipHorizontal(s, 0x20, 0x18, buf);
        }
        int8_t x = (int8_t)rd8(s, RAM_gCurSubstateD);
        if (z != 0xA) {
            drawImageClipped(s, x, 0, 0x20, 0x18, buf);
        } else {
            uint8_t step = (uint8_t)(rd8(s, RAM_gCurSubstateB) << 2);
            uint8_t bx = (uint8_t)(0x2C - step);
            uint16_t arc = (uint16_t)(rom_rd8(ROM_SINE + step * 2) << 8 | rom_rd8(ROM_SINE + step * 2 + 1));
            uint8_t by = (uint8_t)(0x14 - (uint8_t)(arc >> 7));
            eepromReadToRamAlso(s, 0x460, ball, 0x10);
            eepromReadToRamAlso(s, 0x480, ball_mask, 8);
            drawImageToScreen(s, bx, by, ball, 8, 8);
            imageOverlayMasked(s, buf, ball, 0x20, 0x18, ball_mask, (uint8_t)(bx - x), by, 8);
            drawImageToScreen(s, (uint8_t)x, 0, buf, 0x20, 0x18);
        }
    }
    uint8_t b = rd8(s, RAM_gCurSubstateB);
    uint8_t done = b >= rd8(s, RAM_gCurSubstateB + 1);
    uint8_t f = rd8(s, RAM_gCurSubstateF);
    switch (z) {
    case 0:
        if (b < 3) {
            uint8_t h = (uint8_t)((3 - b) * 8);
            lcdFillRectangle(s, 0, 0, 0x60, h, 3);
            lcdFillRectangle(s, 0, (uint8_t)(0x40 - h), 0x60, h, 3);
        }
        break;
    case 1:
        if (!done || is_F7C4_nonzero(s))
            break;
        draw_wild_name(s);
        drawLargeMessage(s, 0x30, 0x1F, 0xE, 1);
        break;
    case 2:
        eepromReadToRamAlso(s, 0x2050, buf, 0x300);
        drawImageToScreen(s, 0, 0x20, buf, 0x60, 0x20);
        break;
    case 3:
        switch (f >> 3 & 3) {
        case 0:
            draw_attack_star(s, buf, 0x1E70, 0xB);
            if (b < 4)
                break;
            drawCurPokemonNameWithBorders(s, 0, 0x20, 5);
            drawLargeMessage(s, 0x30, 0x23, 0xE, 0);
            break;
        case 1:
            if (b == 4)
                playSound(s, 0xC);
            if (b < 4)
                break;
            draw_wild_name(s);
            drawLargeMessage(s, 0x30, 0x24, 0xE, 0);
            break;
        case 2:
            draw_attack_star(s, buf, 0x1EF0, 0xD);
            if (b < 4)
                break;
            drawLargeMessage(s, 0x20, 0x25, 0xD, 0);
            drawLargeMessage(s, 0x30, 0x26, 0xE, 0);
            break;
        }
        break;
    case 4:
        switch (f >> 1 & 3) {
        case 0:
            draw_attack_star(s, buf, 0x1E70, 0xB);
            if (b < 4)
                break;
            draw_wild_name(s);
            drawLargeMessage(s, 0x30, 0x23, 0xE, 0);
            break;
        case 1:
            if (b == 4)
                playSound(s, 0xC);
            if (b < 4)
                break;
            drawCurPokemonNameWithBorders(s, 0, 0x20, 5);
            drawLargeMessage(s, 0x30, 0x24, 0xE, 0);
            break;
        }
        break;
    case 5:
        draw_wild_name(s);
        drawLargeMessage(s, 0x30, 0x22, 0xE, 0);
        break;
    case 6:
        drawNumberAndWattsymbolOnscreen(s, 2, 0x20, rd16(s, RAM_gCurSubstateE));
        drawLargeMessage(s, 0x30, 0x2A, 0xE, 1);
        break;
    case 7:
        if (b <= 3)
            break;
        draw_wild_name(s);
        drawLargeMessage(s, 0x30, 0x21, 0xE, 1);
        break;
    case 8: drawLargeMessage(s, 0x30, 0x29, 0xF, 0); break;
    case 9: drawLargeMessage(s, 0x30, 0x28, 0xF, 0); break;
    case 0xA: drawLargeMessage(s, 0x30, 0x27, 0xF, 0); break;
    case 0xB:
    case 0x11:
        eepromReadToRamAlso(s, 0x1F70, buf, 0xC0);
        drawImageToScreen(s, 8, 0, buf, 0x20, 0x18);
        break;
    case 0xC:
    case 0xE:
    case 0xD: {
        static const uint8_t wobble[4] = { 0x13, 0x14, 0x15, 0x14 };
        uint8_t x = z == 0xD ? wobble[rd8(s, RAM_curUiFlags) & 3] : 0x14;
        eepromReadToRamAlso(s, 0x460, buf, 0x10);
        drawImageToScreen(s, x, 0xC, buf, 8, 8);
        break;
    }
    case 0xF:
        eepromReadToRamAlso(s, 0x460, buf, 0x10);
        drawImageToScreen(s, 0x14, 0xC, buf, 8, 8);
        eepromReadToRamAlso(s, 0x2040, buf, 0x10);
        drawImageToScreen(s, (uint8_t)(0xC - b), (uint8_t)(0xA - (uint8_t)(b << 1)), buf, 8, 8);
        drawImageToScreen(s, (uint8_t)(b + 0x1C), (uint8_t)(0xC - (uint8_t)(b << 1)), buf, 8, 8);
        break;
    case 0x10:
        eepromReadToRamAlso(s, 0x460, buf, 0x10);
        drawImageToScreen(s, 0x14, 0xC, buf, 8, 8);
        draw_wild_name(s);
        drawLargeMessage(s, 0x30, 0x20, 0xE, 1);
        break;
    }
    drawLowBatteryIcon(s, 0, 0);
    b = rd8(s, RAM_gCurSubstateB) + 1;
    if (b >= rd8(s, RAM_gCurSubstateB + 1)) {
        b = rd8(s, RAM_gCurSubstateB + 1);
    }
    wr8(s, RAM_gCurSubstateB, b);
}

#define ROM_NOTE_SHIFT 0xBE70

static void draw_notes(pw_state_t *s, uint8_t x, uint8_t n, uint16_t table) {
    for (uint8_t i = 0; i < n; i++) {
        drawMusicNote(s, (uint8_t)(x + i * 8), 0, rom_rd8(table + i));
    }
}

void drawPeerPlay(pw_state_t *s) {
    uint8_t z = rd8(s, RAM_gCurSubstateZ);
    if (z < 3) {
        drawCurPokemonSmallAnimatedFrame(s, 0x38, 8);
        uint8_t flip = !(rd8(s, RAM_gCurSubstateY) & 2);
        uint8_t x = 8;
        if (z == 0) {
            x = (uint8_t)(8 - (uint8_t)((7 - rd8(s, RAM_gCurSubstateA)) * 3));
        }
        drawPeerPlayPokemonFrame(s, (int8_t)x, 8, flip);
    }
    uint8_t a = rd8(s, RAM_gCurSubstateA);
    uint8_t n1 = (uint8_t)(a + 1), n2 = (uint8_t)((a >> 1) + 1);
    switch (z) {
    case 1:
        drawPeerPokemonName(s, 2, 0x20, 1);
        drawLargeMessage(s, 0x30, 0x2B, 0xE, 0);
        break;
    case 2: {
        uint8_t msg = rd8(s, RAM_gCurSubstateA + 1);
        switch (msg) {
        case 0x2C: draw_notes(s, 0x1C, n1 > 5 ? 5 : n1, ROM_NOTE_SHIFT); break;
        case 0x2D: draw_notes(s, 0x1C, n1 > 4 ? 4 : n1, ROM_NOTE_SHIFT); break;
        case 0x2E: draw_notes(s, 0x24, n2 > 3 ? 3 : n2, ROM_NOTE_SHIFT + 1); break;
        case 0x2F: draw_notes(s, 0x24, n2 > 2 ? 2 : n2, ROM_NOTE_SHIFT + 1); break;
        case 0x30: drawMusicNote(s, 0x2C, 0, rom_rd8(ROM_NOTE_SHIFT + 2)); break;
        }
        drawLargeMessage(s, 0x30, msg, 0xF, 0);
        break;
    }
    case 3:
        drawLargePresentItem(s, 0x20, 4);
        drawLargeMessage(s, 0x30, 0x31, 0xF, 0);
        break;
    case 4:
        drawLargePresentItem(s, 0x20, 4);
        if (rd8(s, RAM_gCurSubstateB + 1)) {
            drawNumberAndWattsymbolOnscreen(s, 2, 0x20, rd8(s, RAM_gCurSubstateB + 1));
        } else {
            drawRouteAvailableItemName(s, 0, 0x20, rd8(s, RAM_gCurSubstateB), 0xD);
        }
        drawLargeMessage(s, 0x30, 0xF, 0xE, 0);
        break;
    }
    a = rd8(s, RAM_gCurSubstateA) + 1;
    wr8(s, RAM_gCurSubstateA, a);
    if (a >= 8) {
        z = rd8(s, RAM_gCurSubstateZ) + 1;
        wr8(s, RAM_gCurSubstateZ, z);
        wr8(s, RAM_gCurSubstateA, 0);
        if (z == 2)
            playSound(s, 9);
        else if (z == 4)
            playSound(s, 6);
    }
    if (rd8(s, RAM_gCurSubstateZ) >= 5) {
        resetSubstates(s);
        setCurrentlyActiveView(s, 0);
    }
    drawLowBatteryIcon(s, 0, 0);
}
