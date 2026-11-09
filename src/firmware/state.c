#include "pw_internal.h"

void interactionHandlerNone(pw_state_t *s) {
    (void)s;
}

uint16_t get_packet_data_pointer(pw_state_t *s) {
    (void)s;
    return RAM_TX_PACKET_payload;
}

void setVolume(pw_state_t *s, uint8_t volume) {
    wr8(s, RAM_likelyVolume, volume);
}

void setCurrentlyActiveView(pw_state_t *s, uint8_t view) {
    wr8(s, RAM_currentlyActiveView, view);
}

void set_gCurSubstateZ_to_1(pw_state_t *s) {
    wr8(s, RAM_gCurSubstateZ, 1);
}

void clear_gCurSubstateY(pw_state_t *s) {
    wr8(s, RAM_gCurSubstateY, 0);
}

void clearSubstatesYZ(pw_state_t *s) {
    wr8(s, RAM_gCurSubstateY, 0);
    wr8(s, RAM_gCurSubstateZ, 0);
}

void resetSubstates(pw_state_t *s) {
    wr8(s, RAM_gCurSubstateY, 0);
    wr8(s, RAM_gCurSubstateZ, 0);
    wr8(s, RAM_gCurSubstateA, 0x20);
    wr8(s, RAM_gCurSubstateA + 1, rd8(s, RAM_gCurSubstateA + 1) & ~0x07);
}

void setEventLoopFunc(pw_state_t *s, uint16_t loop) {
    wr16(s, RAM_prevEventLoopFunc, rd16(s, RAM_currentEventLoopFunc));
    wr16(s, RAM_currentEventLoopFunc, loop);
}

uint8_t is_F7C4_nonzero(pw_state_t *s) {
    return rd16(s, RAM_word_FFF7C4) != 0;
}

void write_F7C4_clear_F7C8_and_F7CA(pw_state_t *s, uint16_t value) {
    wr16(s, RAM_word_FFF7C4, value);
    wr16(s, RAM_word_FFF7C8, 0);
    wr16(s, RAM_word_FFF7CA, 0);
}

void incrementSaturating_F790(pw_state_t *s) {
    uint16_t v = rd16(s, RAM_word_FFF790);
    if ((uint16_t)(v + 1) != 0) {
        wr16(s, RAM_word_FFF790, v + 1);
    }
}

void clear_bytes_at_F8EE_F8EF_words_at_F8EA_F8E6(pw_state_t *s) {
    wr8(s, RAM_byte_FFF8EE, 0);
    wr32(s, RAM_dword_FFF8EA, 0);
    wr32(s, RAM_dword_FFF8E6, 0);
    wr8(s, RAM_byte_FFF8EF, 0);
}

void lslByteAtPtr(pw_state_t *s, uint16_t ptr, uint8_t by) {
    int n = by <= 0x7F ? by : 0;
    wr8(s, ptr, shl8(rd8(s, ptr), n));
}

void lsrByteAtPtr(pw_state_t *s, uint16_t ptr, uint8_t by) {
    int n = by <= 0x7F ? by : 0;
    wr8(s, ptr, shr8(rd8(s, ptr), n));
}

uint16_t orr_0x0101_into_three_words(pw_state_t *s, uint16_t ptr) {
    for (int i = 0; i < 3; i++, ptr += 2) {
        wr16(s, ptr, rd16(s, ptr) | 0x0101);
    }
    return ptr;
}

uint8_t findFreeItemSlot(pw_state_t *s, uint16_t slots) {
    for (uint8_t i = 0; i < 3; i++) {
        if (rd16(s, slots + i * 4) == 0) {
            return i;
        }
    }
    return 3;
}

uint8_t findFirstEmptyCaughtSlot(pw_state_t *s, uint16_t slots) {
    for (uint8_t i = 0; i < 3; i++) {
        if (rd16(s, slots + i * 16) == 0) {
            return i;
        }
    }
    return 3;
}

#define EEPROM_CAUGHT_AND_FOUND 0xCE8C
#define CAUGHT_AND_FOUND_SIZE   0x64
#define EEPROM_CEF0             0xCEF0
#define CEF0_SIZE               0x1C
#define EEPROM_SPECIAL_ITEM     0xBD40
#define SPECIAL_ITEM_SIZE       0x188
#define EEPROM_EVENT_LOG        0xCF0C

void eraseCaughtPokemonAndFoundItems(pw_state_t *s) {
    eepromFillWithByte(s, EEPROM_CAUGHT_AND_FOUND, CAUGHT_AND_FOUND_SIZE, 0);
}

void fill_eeprom_0xCEF0_to_CF0B_with_zero(pw_state_t *s) {
    eepromFillWithByte(s, EEPROM_CEF0, CEF0_SIZE, 0);
}

void clearEventLogEntries(pw_state_t *s) {
    uint16_t entry = EEPROM_EVENT_LOG;
    for (int i = 0; i < 24; i++, entry += 0x88) {
        eepromWriteSingleByte(s, entry + 0x84, 0);
    }
}

void readSpecialItemInfo(pw_state_t *s, uint16_t ram) {
    eepromReadToRamAlso(s, EEPROM_SPECIAL_ITEM, ram, SPECIAL_ITEM_SIZE);
}

void writeSpecialItemInfo(pw_state_t *s, uint16_t ram) {
    eepromWriteData(s, EEPROM_SPECIAL_ITEM, ram, SPECIAL_ITEM_SIZE);
}

void writeNintendoSignature(pw_state_t *s) {
    for (uint8_t i = 0; i < 8; i++) {
        eepromWriteSingleByte(s, i, rom_rd8(ROM_EEPROM_HEADER + i));
    }
}

void drawNG6inTinyChars(pw_state_t *s) {
    drawTinyChars(s, 8, 8, 0xBF93);
}

void shortDelay(pw_state_t *s) {
    (void)s;
}

uint8_t checkKeyStates(pw_state_t *s, uint8_t mask) {
    return rd8(s, RAM_CUR_KEYS) & mask;
}

void addWatts(pw_state_t *s, uint8_t watts) {
    uint16_t total = rd16(s, RAM_RamCache_curWatts) + watts;
    if (total > 9999) {
        total = 9999;
    }
    wr16(s, RAM_RamCache_curWatts, total);
    writeReliableDataToEeprom(s, EEPROM_HEALTH0, EEPROM_HEALTH1, RAM_RamCache_totalSteps, HEALTH_SIZE);
}

void clearWattsAndErasePokemon(pw_state_t *s) {
    wr16(s, RAM_RamCache_curWatts, 0);
    writeReliableDataToEeprom(s, EEPROM_HEALTH0, EEPROM_HEALTH1, RAM_RamCache_totalSteps, HEALTH_SIZE);
    eraseCaughtPokemonAndFoundItems(s);
}

void applyDefaultSettings(pw_state_t *s) {
    wr8(s, RAM_gCurSubstateY, 0);
    wr8(s, RAM_gCurSubstateA, 0);
    wr16(s, RAM_gCurSubstateC, 1);
    wr8(s, RAM_RamCache_settingsByte, (rd8(s, RAM_RamCache_settingsByte) & 0xF9) | 4);
    setVolume(s, 2);
    lcdSetContrast(s, 4);
}

uint8_t checkForNintendoSignature(pw_state_t *s) {
    for (uint8_t i = 0; i < 8; i++) {
        if (eepromReadSingleByte(s, i) != (uint16_t)(int16_t)(int8_t)rom_rd8(ROM_EEPROM_HEADER + i)) {
            return 0;
        }
    }
    return 1;
}

void bitfieldInsert(pw_state_t *s, uint16_t ptr, uint8_t value, uint8_t width, uint8_t offset) {
    uint8_t mask = (uint8_t)((1 << (width & 7)) - 1);
    uint16_t pair = (uint16_t)(mask << 8 | (value & mask));

    int n = (int8_t)(8 - offset - width);
    if (n != 0) {
        do {
            pair <<= 1;
        } while (--n > 0);
    }
    mask = pair >> 8;
    wr8(s, ptr, (rd8(s, ptr) & ~mask) | (uint8_t)pair);
}

#define EEPROM_ROUTE_INFO  0x8F00
#define ROUTE_INFO_SIZE    0xBE
#define ROUTE_ITEMS_OFFSET 0x8C

uint16_t getItemIdForNthItemOnRoute(pw_state_t *s, uint8_t n) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, ROUTE_INFO_SIZE);
    eepromReadToRamAlso(s, EEPROM_ROUTE_INFO, buf, ROUTE_INFO_SIZE);
    return rd16(s, buf + ROUTE_ITEMS_OFFSET + n * 2);
}

#define ROM_BATTLE_CHANCE_TABLE 0xBB45

uint8_t rollBattleChance(pw_state_t *s) {
    uint8_t roll;
    uint32_t rem;
    divmod32(rand32(s) >> 3, 100, &rem);
    roll = rem;
    uint8_t k = rd8(s, RAM_gCurSubstateA + 1);
    if (k == 0) {
        return 0;
    }
    return roll < rom_rd8(ROM_BATTLE_CHANCE_TABLE + k - 1);
}

#define EEPROM_WALKER_INFO0 0x00ED
#define EEPROM_WALKER_INFO1 0x01ED
#define WALKER_INFO_SIZE    0x68
#define WALKER_INFO_FLAGS   0x5B

void updateCachedWalkerStatus(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, WALKER_INFO_SIZE);
    readReliableDataFromEeprom(s, EEPROM_WALKER_INFO0, EEPROM_WALKER_INFO1, buf, WALKER_INFO_SIZE);
    uint8_t flags = rd8(s, buf + WALKER_INFO_FLAGS);
    uint8_t status = rd8(s, RAM_walker_status_flags) & ~0x06;
    status |= (flags & 1) << 1;
    status |= ((flags >> 1) & 1) << 2;
    wr8(s, RAM_walker_status_flags, status);
}

#define PEER_LOG_ENTRY_SIZE 0x224

void pushSeenTrainersLogDown(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, PEER_LOG_ENTRY_SIZE);
    uint16_t src = 0xEF44, dst = 0xF168;
    for (int i = 0; i < 10; i++) {
        eepromReadToRamAlso(s, src, buf, PEER_LOG_ENTRY_SIZE);
        eepromWriteData(s, dst, buf, PEER_LOG_ENTRY_SIZE);
        src -= PEER_LOG_ENTRY_SIZE;
        dst -= PEER_LOG_ENTRY_SIZE;
    }
}

void goToDowsingMicroApp(pw_state_t *s) {
    wr8(s, RAM_gCurSubstateZ, 0);
    wr8(s, RAM_gCurSubstateA + 1, 0);
    wr8(s, RAM_gCurSubstateB, 2);
    uint8_t r = (uint16_t)(rand32(s) << 3) >> 8;
    wr8(s, RAM_gCurSubstateB + 1, r % 6);
    wr8(s, RAM_gCurSubstateC, 0xFF);
    wr8(s, RAM_gCurSubstateD, 0);
}

void enterDebugView22(pw_state_t *s) {
    wr8(s, RAM_ACCEL_SAMPLE_NEXT_WRITE_IDX, 0);
    wr16(s, RAM_gCurSubstateY, 0x0010);
    wr8(s, RAM_gCurSubstateA, 0);
    wr8(s, RAM_gCurSubstateA + 1, 0);
    wr16(s, RAM_gCurSubstateB, 0);
    wr16(s, RAM_gCurSubstateC, 0);
    wr16(s, RAM_gCurSubstateE, 0);
    eepromReadToRamAlso(s, 0x0008, RAM_gCurSubstateF, 8);
    wr8(s, RAM_walker_status_flags, rd8(s, RAM_walker_status_flags) | 0x02);
}

uint8_t cursorToPrevOwnedSlot(pw_state_t *s, uint16_t owned) {
    uint8_t y = rd8(s, RAM_gCurSubstateY);
    if (y == 0) {
        return 1;
    }
    wr8(s, RAM_gCurSubstateY, --y);
    for (uint8_t i = 0; i < 10; i++) {
        y = rd8(s, RAM_gCurSubstateY);

        uint16_t bit = 1;
        int n = y <= 0x80 ? y : 0;
        for (int k = 0; k < n; k++) {
            bit <<= 1;
        }
        if (bit & owned) {
            return 0;
        }
        if (y == 0) {
            return 1;
        }
        wr8(s, RAM_gCurSubstateY, y - 1);
    }
    return 0;
}

static uint16_t bit16(uint8_t n) {
    int k = n <= 0x80 ? n : 0;
    return k >= 16 ? 0 : (uint16_t)(1u << k);
}

uint8_t hasParticipatedInEvent(pw_state_t *s, uint16_t buf, uint8_t idx) {
    if (idx == 0) {
        return 0;
    }
    readReliableDataFromEeprom(s, EEPROM_WALKER_INFO0, EEPROM_WALKER_INFO1, buf, WALKER_INFO_SIZE);
    uint8_t byte = rd8(s, buf + 0x38 + (idx >> 3));
    return (byte & bit16(idx & 7)) != 0;
}

uint8_t cursorToPrevOwnedSlotWrapping(pw_state_t *s, uint16_t owned) {
    uint8_t y = (rd8(s, RAM_gCurSubstateY) + 9) % 10;
    wr8(s, RAM_gCurSubstateY, y);
    for (uint8_t i = 0; i < 10; i++) {
        y = rd8(s, RAM_gCurSubstateY);
        if (bit16(y) & owned) {
            return 0xFF;
        }
        y = (y + 9) % 10;
        wr8(s, RAM_gCurSubstateY, y);
    }
    return y;
}

#define WIPE_SYNC_TIME 220924800

void wipeOutStepsAndSyncTime(pw_state_t *s, uint8_t full) {
    if (full) {
        wr32(s, RAM_RamCache_totalSteps, 0);
        wr16(s, RAM_RamCache_totalDays, 0);
        wr32(s, RAM_RamCache_lastSyncTime, WIPE_SYNC_TIME);
        wr32(s, RAM_RamCache_STEP_COUNT_maybe, 0);
    }
    wr16(s, RAM_word_FFF790, 0);
    wr16(s, RAM_RamCache_curWatts, 0);
    wr8(s, RAM_stepToWattDividerState, 0);
    wr8(s, RAM_RamCache_settingsByte, (rd8(s, RAM_RamCache_settingsByte) & 0xA4) | 0x24);
    wr8(s, RAM_likelyCurEventLogPointer, 0);
    writeReliableDataToEeprom(s, EEPROM_HEALTH0, EEPROM_HEALTH1, RAM_RamCache_totalSteps, HEALTH_SIZE);
}

void setThatWeveParticipatedInAGivenEvent(pw_state_t *s, uint16_t buf, uint8_t idx) {
    if (idx == 0) {
        return;
    }
    readReliableDataFromEeprom(s, EEPROM_WALKER_INFO0, EEPROM_WALKER_INFO1, buf, WALKER_INFO_SIZE);
    uint16_t p = buf + 0x38 + (idx >> 3);
    wr8(s, p, rd8(s, p) | (uint8_t)bit16(idx & 7));
    writeReliableDataToEeprom(s, EEPROM_WALKER_INFO0, EEPROM_WALKER_INFO1, buf, WALKER_INFO_SIZE);
}

void homeScreenSlidePokemon(pw_state_t *s) {
    uint8_t x = rd8(s, RAM_gCurSubstateA);
    uint8_t f = rd8(s, RAM_gCurSubstateA + 1);
    if (!(f & 4)) {
        x -= 4;
        if (x <= 0x20) {
            x = 0x20;
        }
        wr8(s, RAM_gCurSubstateA, x);
    } else {
        x += 4;
        wr8(s, RAM_gCurSubstateA, x);
        if (x >= 0x60) {
            wr8(s, RAM_gCurSubstateA + 1, (f | 2) & ~4);
        }
    }
    if ((rd8(s, RAM_common_bit_flags) & 0x80) && rd8(s, RAM_gCurSubstateA) <= 0x20) {
        wr8(s, RAM_gCurSubstateA + 1, rd8(s, RAM_gCurSubstateA + 1) | 5);
    }
}

void homeScreenPacePokemon(pw_state_t *s) {
    if (rd8(s, RAM_curUiFlags) & 3) {
        return;
    }
    uint8_t x = rd8(s, RAM_gCurSubstateA);
    uint8_t f = rd8(s, RAM_gCurSubstateA + 1);
    int turn = 0;
    if (!(f & 4)) {
        x -= 4;
        wr8(s, RAM_gCurSubstateA, x);
        if (x <= 0x20) {
            x = 0x20;
            turn = 1;
        }
    } else {
        x += 4;
        wr8(s, RAM_gCurSubstateA, x);
        if (x >= 0x40) {
            x = 0x40;
            turn = 1;
        }
    }
    if (turn) {
        wr8(s, RAM_gCurSubstateA, x);
        wr8(s, RAM_gCurSubstateA + 1, f ^ 4);
    }
    if (!(rd8(s, RAM_common_bit_flags) & 0x80)) {
        wr8(s, RAM_gCurSubstateA, 0x68);
        wr8(s, RAM_gCurSubstateA + 1, rd8(s, RAM_gCurSubstateA + 1) & ~6);
    }
}

void end_walk_action(pw_state_t *s) {
    uint16_t buf = RAM_sometimes_new_rxed_data_for_0xED;
    wr8(s, RAM_walker_status_flags, rd8(s, RAM_walker_status_flags) & ~4);
    wr8(s, RAM_RamCache_settingsByte, rd8(s, RAM_RamCache_settingsByte) & ~1);
    wr8(s, RAM_likelyCurEventLogPointer, 0);
    wr16(s, RAM_RamCache_curWatts, 0);
    writeReliableDataToEeprom(s, EEPROM_HEALTH0, EEPROM_HEALTH1, RAM_RamCache_totalSteps, HEALTH_SIZE);
    readReliableDataFromEeprom(s, EEPROM_WALKER_INFO0, EEPROM_WALKER_INFO1, buf, WALKER_INFO_SIZE);
    wr32(s, buf + 4, 0);
    wr16(s, buf + 0xA, 0);
    wr8(s, buf + 0x5B, rd8(s, buf + 0x5B) & ~6);
    writeReliableDataToEeprom(s, EEPROM_WALKER_INFO0, EEPROM_WALKER_INFO1, buf, WALKER_INFO_SIZE);
    eraseCaughtPokemonAndFoundItems(s);
    clearEventLogEntries(s);
    eepromFillWithByte(s, 0xB800, 0x06C8, 0);
    eepromFillWithByte(s, 0xDE24, 0x1568, 0);
    eepromFillWithByte(s, 0x8F00, 0x10, 0);
}

uint8_t cursorToNextOwnedSlot(pw_state_t *s, uint16_t owned) {
    uint8_t y = rd8(s, RAM_gCurSubstateY);
    if (y == 9) {
        return 1;
    }
    y++;
    for (uint8_t n = 0; n < 10; n++, y++) {
        if (bit16(y) & owned) {
            wr8(s, RAM_gCurSubstateY, y);
            return 0;
        }
        if (y == 9) {
            return 1;
        }
    }
    return 0;
}

void cursorToNextOwnedSlotWrapping(pw_state_t *s, uint16_t owned) {
    wr8(s, RAM_gCurSubstateY, (rd8(s, RAM_gCurSubstateY) + 1) % 10);
    for (uint8_t i = 0; i < 10; i++) {
        if (bit16(rd8(s, RAM_gCurSubstateY)) & owned) {
            return;
        }
        wr8(s, RAM_gCurSubstateY, (rd8(s, RAM_gCurSubstateY) + 1) % 10);
    }
}

void cursorToFirstSlotOfA(pw_state_t *s) {
    wr8(s, RAM_gCurSubstateY, 9);
    cursorToNextOwnedSlotWrapping(s, rd16(s, RAM_gCurSubstateA));
}

void cursorToFirstSlotOfB(pw_state_t *s) {
    wr8(s, RAM_gCurSubstateY, 9);
    cursorToNextOwnedSlotWrapping(s, rd16(s, RAM_gCurSubstateB));
}

void accountForStepTaken(pw_state_t *s) {
    uint8_t a = rd8(s, RAM_step_detection_related_A);
    if (rd8(s, RAM_step_detection_related_B) == a) {
        return;
    }
    uint8_t c = rd8(s, RAM_step_detection_related_C) + a;
    wr8(s, RAM_step_detection_related_C, c);
    if (c <= 0x40) {
        return;
    }
    uint16_t walk = rd16(s, RAM_someWalkingRelatedState) + 1;
    wr16(s, RAM_someWalkingRelatedState, walk > 9999 ? 9999 : walk);
    uint32_t today = rd32(s, RAM_stepCountTodaySoFar) + 1;
    wr32(s, RAM_stepCountTodaySoFar, today > 99999 ? 99999 : today);
    clampTotalStepToLimit(s, rd32(s, RAM_RamCache_totalSteps) + 1);
    uint8_t div = rd8(s, RAM_stepToWattDividerState) + 1;
    wr8(s, RAM_stepToWattDividerState, div);
    if (div >= 20) {
        wr8(s, RAM_stepToWattDividerState, div - 20);
        uint16_t watts = rd16(s, RAM_RamCache_curWatts) + 1;
        wr16(s, RAM_RamCache_curWatts, watts > 9999 ? 9999 : watts);
    }
    uint8_t b = rd8(s, RAM_step_detection_related_B) + 1;
    wr8(s, RAM_step_detection_related_B, b > a ? a : b);
    wr8(s, RAM_step_detection_related_C, rd8(s, RAM_step_detection_related_C) - 0x40);
}

void dayEnded(pw_state_t *s) {
    uint16_t days = rd16(s, RAM_RamCache_totalDays);
    if (days < 9999) {
        wr16(s, RAM_RamCache_totalDays, days + 1);
    }
    writeReliableDataToEeprom(s, EEPROM_HEALTH0, EEPROM_HEALTH1, RAM_RamCache_totalSteps, HEALTH_SIZE);
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x1C);
    eepromReadToRamAlso(s, 0xCEF0, buf, 0x1C);
    for (int i = 0; i < 6; i++) {
        wr32(s, buf + 0x18 - i * 4, rd32(s, buf + 0x14 - i * 4));
    }
    wr32(s, buf, rd32(s, RAM_stepCountTodaySoFar));
    eepromWriteData(s, 0xCEF0, buf, 0x1C);
    wr32(s, RAM_stepCountTodaySoFar, 0);
    uint16_t rec = 0xDC00;
    for (int i = 0; i < 10; i++) {
        rec += 0x224;
        eepromFillWithByte(s, rec + 8, 0x28, 0xFF);
    }
}

static uint8_t to_bcd(uint32_t v) {
    return (uint8_t)((v / 10) << 4 | v % 10);
}

void rtcSet(pw_state_t *s, uint32_t seconds) {
    wr8(s, RAM_currentSecond, to_bcd(seconds % 60));
    wr8(s, RAM_currentMinute, to_bcd(seconds / 60 % 60));
    wr8(s, RAM_currentHour, to_bcd(seconds / 3600 % 24));
    hw_rtc_set(s, rd8(s, RAM_currentSecond), rd8(s, RAM_currentMinute), rd8(s, RAM_currentHour));
}

void pokemonJoinsEmptyWalkerWalk(pw_state_t *s) {
    wr16(s, RAM_someWalkingRelatedState, 0);
    wr8(s, RAM_walker_status_flags, rd8(s, RAM_walker_status_flags) | 4);
    setHeapPointer(s);
    uint16_t info = static_malloc(s, WALKER_INFO_SIZE);
    readReliableDataFromEeprom(s, EEPROM_WALKER_INFO0, EEPROM_WALKER_INFO1, info, WALKER_INFO_SIZE);
    if (rd8(s, info + 0x5B) & 2) {
        return;
    }
    wr8(s, info + 0x5B, rd8(s, info + 0x5B) | 2);
    wr32(s, info + 4, rd32(s, info));
    wr16(s, info + 0xA, rd16(s, info + 8));
    wr8(s, info + 0x5B, rd8(s, info + 0x5B) | 4);
    writeReliableDataToEeprom(s, EEPROM_WALKER_INFO0, EEPROM_WALKER_INFO1, info, WALKER_INFO_SIZE);

    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x180);
    eepromReadToRamAlso(s, 0x9D7E, buf, 0x180);
    eepromWriteData(s, 0x91BE, buf, 0x180);
    eepromReadToRamAlso(s, 0x9EFE, buf, 0x600);
    eepromWriteData(s, 0x933E, buf, 0x600);
    eepromReadToRamAlso(s, 0xA77E, buf, 0x140);
    eepromWriteData(s, 0x993E, buf, 0x140);

    setHeapPointer(s);
    uint16_t team = static_malloc(s, 0xBE);
    eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
    memoryCopy(s, team + 0x72, team, 0x10);
    wr8(s, team + 0xD, rd8(s, team + 0xD) & ~0x80);
    wr8(s, team + 0x26, 0x46);
    for (uint16_t i = 0; i < 0x16; i++) {
        wr8(s, team + 0x10 + i, 0);
    }
    eepromWriteData(s, 0x8F00, team, 0xBE);
    wr16(s, RAM_word_FFF790, 0);
    writeReliableDataToEeprom(s, EEPROM_HEALTH0, EEPROM_HEALTH1, RAM_RamCache_totalSteps, HEALTH_SIZE);
    clearEventLogEntries(s);
}

void chooseWalkEvent(pw_state_t *s) {
    uint8_t flags = rd8(s, RAM_walker_status_flags);
    if ((flags & 0x18) != 0x10 || rd8(s, RAM_currentlyActiveView) != 0 || !(flags & 1)) {
        return;
    }
    wr8(s, RAM_walker_status_flags, flags & ~1);
    wr8(s, RAM_gCurSubstateY, 0);
    wr8(s, RAM_gCurSubstateZ, 0);
    uint16_t steps = rd16(s, RAM_someWalkingRelatedState);
    if (rand32(s) % 100 >= 40) {
        return;
    }
    uint8_t event;
    if (!(rd8(s, RAM_walker_status_flags) & 4)) {
        if (rd16(s, RAM_someWalkingRelatedState) < 300) {
            return;
        }
        event = 7;
    } else {
        if (rd16(s, RAM_word_FFF7A2) < 3600) {
            return;
        }
        setHeapPointer(s);
        uint16_t team = static_malloc(s, 0xBE);
        eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
        uint8_t f = rd8(s, team + 0x26);
        setHeapPointer(s);
        uint16_t found = static_malloc(s, 0xC);
        eepromReadToRamAlso(s, 0xCEBC, found, 0xC);
        if (findFreeItemSlot(s, found) < 3 && f >= 90 && steps >= 500) {
            event = 1;
        } else if (f >= 80 && steps >= 250) {
            event = 2;
        } else if (steps >= 200) {
            event = 3;
        } else if (steps >= 100) {
            event = 4;
        } else if (rd16(s, RAM_word_FFF790) >= 60 && steps <= 50) {
            event = 5;
        } else {
            return;
        }
    }
    wr8(s, RAM_gCurSubstateY, event);
    wr8(s, RAM_gCurSubstateZ, 0x30);
}

#define EEPROM_EVENT_LOG_BASE 0xCF0C
#define EVENT_LOG_ENTRY_SIZE  0x88
#define EVENT_LOG_ENTRIES     23

static void log_pokemon_flags(pw_state_t *s, uint16_t at, uint8_t b, uint8_t top2) {
    wr8(s, at, (rd8(s, at) & 0xE0) | (b & 0x1F));
    bitfieldInsert(s, at, top2, 2, 1);
}

void logEvent(pw_state_t *s, uint16_t team, uint16_t buf, uint8_t type, uint8_t special, uint16_t extra, uint8_t poke) {
    uint16_t entry = EEPROM_EVENT_LOG_BASE + rd8(s, RAM_likelyCurEventLogPointer) * EVENT_LOG_ENTRY_SIZE;
    uint8_t old = eepromReadSingleByte(s, entry + 0x84);
    if (old != 0 && type == 0x1B) {
        return;
    }
    if (old == 0x19) {
        uint8_t next = (rd8(s, RAM_likelyCurEventLogPointer) + 1) % EVENT_LOG_ENTRIES;
        wr8(s, RAM_likelyCurEventLogPointer, next);
        entry = EEPROM_EVENT_LOG_BASE + next * EVENT_LOG_ENTRY_SIZE;
    }
    if (type > 0xA) {
        for (uint16_t i = 0; i < EVENT_LOG_ENTRY_SIZE; i++) {
            wr8(s, buf + i, 0);
        }
    }
    wr8(s, buf + 0x84, type);
    wr16(s, buf + 0xE, extra);
    wr32(s, buf, rd32(s, RAM_RamCache_lastSyncTime));
    wr16(s, buf + 0x78, rd16(s, RAM_someWalkingRelatedState));
    wr32(s, buf + 0x7C, rd32(s, RAM_stepCountTodaySoFar));
    wr16(s, buf + 0xA, rd16(s, team));
    for (uint16_t i = 0; i < 0x16; i++) {
        wr8(s, buf + 0x20 + i, rd8(s, team + 0x10 + i));
    }
    wr8(s, buf + 0x77, rd8(s, team + 0x26));
    uint8_t f = rd8(s, team + 0xD);
    log_pokemon_flags(s, buf + 0x85, f, f >> 5 & 3);
    wr8(s, buf + 0x85, (rd8(s, buf + 0x85) & 0x7F) | (rd8(s, team + 0xE) & 2) << 6);
    if (!special) {
        wr8(s, buf + 0x76, rd8(s, team + 0x27));
        for (uint16_t i = 0; i < 0x2A; i++) {
            wr8(s, buf + 0x4C + i, rd8(s, team + 0x28 + i));
        }
    } else {
        wr8(s, buf + 0x76, eepromReadSingleByte(s, 0xBF06));
        eepromReadToRamAlso(s, 0xBF50, buf + 0x4C, 0x2A);
    }
    if (poke >= 1 && poke <= 3) {
        uint16_t wild = team + 0x52 + (poke - 1) * 0x10;
        wr16(s, buf + 0xC, rd16(s, wild));
        uint8_t wf = rd8(s, wild + 0xD);
        log_pokemon_flags(s, buf + 0x86, wf, wf >> 5 & 3);
    } else if (poke == 4) {
        eepromReadToRamAlso(s, (type == 0xF || type == 0x10) ? 0xBF08 : 0xBA44, buf + 0xC, 2);
        uint8_t ef = eepromReadSingleByte(s, 0xBF0D);
        log_pokemon_flags(s, buf + 0x86, ef, ef >> 5 & 3);
    }
    eepromWriteData(s, entry, buf, EVENT_LOG_ENTRY_SIZE);
    wr8(s, RAM_likelyCurEventLogPointer, (rd8(s, RAM_likelyCurEventLogPointer) + 1) % EVENT_LOG_ENTRIES);
    writeReliableDataToEeprom(s, EEPROM_HEALTH0, EEPROM_HEALTH1, RAM_RamCache_totalSteps, HEALTH_SIZE);
}

void takeItemAndPokeInventory(pw_state_t *s, uint16_t flags) {
    setHeapPointer(s);
    uint16_t caught = static_malloc(s, 0x30);
    uint16_t items = static_malloc(s, 0x34);
    wr16(s, flags, 0);
    wr16(s, flags + 2, 0);
    if (rd8(s, RAM_walker_status_flags) & 4) {
        wr16(s, flags, rd16(s, flags) | 1);
    }
    eepromReadToRamAlso(s, 0xCE8C, caught, 0x30);
    eepromReadToRamAlso(s, 0xCEBC, items, 0x34);
    for (uint8_t i = 0; i < 3; i++) {
        if (rd16(s, caught + i * 0x10)) {
            wr16(s, flags, rd16(s, flags) | 2 << i);
        }
    }
    for (uint8_t i = 0; i < 3; i++) {
        if (rd16(s, items + i * 4)) {
            wr16(s, flags, rd16(s, flags) | 0x40 << i);
        }
    }
    for (uint8_t i = 0; i < 10; i++) {
        if (rd16(s, items + 0xC + i * 4)) {
            wr16(s, flags + 2, rd16(s, flags + 2) | 1 << i);
        }
    }
    uint8_t ev = eepromReadSingleByte(s, 0xB800);
    if (ev & 0x20) {
        wr16(s, flags, rd16(s, flags) | 0x10);
    }
    if (ev & 0x10) {
        wr16(s, flags, rd16(s, flags) | 0x20);
    }
    if (ev & 0x40) {
        wr8(s, flags, rd8(s, flags) | 2);
    }
}

#define ROM_PEAK_SEARCH_ORDER 0xBEF8
#define RAM_SPECTRUM          RAM_sometimes_new_rxed_data_for_0xED

uint32_t findWalkingPeak(pw_state_t *s, uint16_t spec) {
    uint16_t all_max = 0;
    for (int i = 1; i < 0x1E; i++) {
        uint16_t v = rd16(s, spec + i * 2);
        if (v > all_max) {
            all_max = v;
        }
    }
    uint16_t mags = spec + 0xA, best = 0;
    uint8_t bin = 0xFF;
    for (int i = 0; i < 10; i++) {
        uint8_t k = rom_rd8(ROM_PEAK_SEARCH_ORDER + i);
        uint16_t v = rd16(s, mags + k * 2);
        if (v >= 0x200 && (uint16_t)(best * 3) < (uint16_t)(v << 1)) {
            bin = k;
            best = v;
        }
    }
    if (rd8(s, RAM_byte_FFF8EF)) {
        if (all_max > (uint16_t)(best << 2) / 3) {
            bin = 0xFF;
        }
    } else if (all_max > (uint16_t)(best << 1)) {
        bin = 0xFF;
    }
    if (bin == 0xFF) {
        wr8(s, RAM_byte_FFF8EE, 0);
        wr32(s, RAM_dword_FFF8EA, 0);
        wr8(s, RAM_byte_FFF8EF, 1);
        return 0;
    }
    wr8(s, RAM_byte_FFF8EF, 0);
    return (uint32_t)(int32_t)(int16_t)spectrumPeakCentroid(s, mags, bin);
}

static void add_steps(pw_state_t *s, uint8_t n) {
    uint16_t walk = rd16(s, RAM_someWalkingRelatedState) + n;
    wr16(s, RAM_someWalkingRelatedState, walk > 9999 ? 9999 : walk);
    uint32_t today = rd32(s, RAM_stepCountTodaySoFar) + n;
    wr32(s, RAM_stepCountTodaySoFar, today > 99999 ? 99999 : today);
    clampTotalStepToLimit(s, rd32(s, RAM_RamCache_totalSteps) + n);
    uint8_t div = rd8(s, RAM_stepToWattDividerState) + n;
    wr8(s, RAM_stepToWattDividerState, div);
    if (div >= 20) {
        wr8(s, RAM_stepToWattDividerState, div - 20);
        uint16_t watts = rd16(s, RAM_RamCache_curWatts) + 1;
        wr16(s, RAM_RamCache_curWatts, watts > 9999 ? 9999 : watts);
    }
}

static uint8_t accumulate_steps(pw_state_t *s, uint32_t amount) {
    uint32_t acc = rd32(s, RAM_dword_FFF8E6) + amount;
    uint8_t steps = (uint8_t)((uint32_t)((int32_t)acc >> 1) >> 8);
    wr8(s, RAM_step_detection_related_A, steps);
    wr32(s, RAM_dword_FFF8E6, acc & 0x1FF);
    return steps;
}

void processAccelSamples(pw_state_t *s) {
    wr8(s, RAM_common_bit_flags, rd8(s, RAM_common_bit_flags) & ~0x80);
    for (int i = 0; i < 32; i++) {
        wr16(s, RAM_SPECTRUM + i * 2, 0);
    }
    processAccelerometerSamplesAxis(s, RAM_ACCEL_SAMPLES_X);
    processAccelerometerSamplesAxis(s, RAM_ACCEL_SAMPLES_Y);
    processAccelerometerSamplesAxis(s, RAM_ACCEL_SAMPLES_Z);
    uint32_t peak = findWalkingPeak(s, RAM_SPECTRUM);

    if (rd8(s, RAM_currentlyActiveView) == 0x17) {
        uint8_t a = rd8(s, RAM_gCurSubstateA), f = rd8(s, RAM_gCurSubstateF);
        uint16_t b = rd16(s, RAM_gCurSubstateB), c = rd16(s, RAM_gCurSubstateC), e = rd16(s, RAM_gCurSubstateE);
        if (a < f) {
            if (peak) {
                wr8(s, RAM_gCurSubstateA, a + 1);
                uint16_t lo = rd16(s, RAM_word_FFF7DA), hi = rd16(s, RAM_word_FFF7DC);
                if (b < lo || c < lo || e < lo || b > hi || c > hi || e > hi) {
                    wr8(s, RAM_currentlyActiveView, 0x18);
                }
            }
        } else if (rd8(s, RAM_gCurSubstateA + 1) < rd8(s, RAM_gCurSubstateF + 1)) {
            uint16_t lim = rd16(s, RAM_word_FFF7DE);
            if (b < lim && c < lim && e < lim) {
                wr8(s, RAM_gCurSubstateA + 1, rd8(s, RAM_gCurSubstateA + 1) + 1);
            }
        }
    }

    if (peak == 0) {
        wr32(s, RAM_dword_FFF8EA, 0);
        return;
    }
    wr8(s, RAM_common_bit_flags, rd8(s, RAM_common_bit_flags) | 0x80);
    uint32_t pending = rd32(s, RAM_dword_FFF8EA);
    if (pending) {
        wr32(s, RAM_dword_FFF8EA, 0);
        add_steps(s, accumulate_steps(s, pending));
    }
    if (accumulate_steps(s, peak)) {
        wr8(s, RAM_byte_FFF7B0, 0x1E);
    }
    wr8(s, RAM_step_detection_related_B, 0);
    wr8(s, RAM_step_detection_related_C, 0x20);
}

void recordPokeCaughtNoEventConsideration(pw_state_t *s) {
    uint8_t y = rd8(s, RAM_gCurSubstateY);
    if (!y) {
        return;
    }
    setHeapPointer(s);
    uint16_t caught = static_malloc(s, 0x30);
    eepromReadToRamAlso(s, 0xCE8C, caught, 0x30);
    uint16_t slot = caught + (uint16_t)((int8_t)rd8(s, RAM_gCurSubstateZ) * 0x10);
    eepromReadToRamAlso(s, 0x8F52 + (uint16_t)(y - 1) * 0x10, slot, 0x10);
    eepromWriteData(s, 0xCE8C, caught, 0x30);
    uint16_t team = static_malloc(s, 0xBE);
    eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
    uint16_t buf = static_malloc(s, 0x88);
    logEvent(s, team, buf, 0xD, 0, 0, rd8(s, RAM_gCurSubstateY));
}

void recordItemFoundNoEventConsideration(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t items = static_malloc(s, 0xC);
    eepromReadToRamAlso(s, 0xCEBC, items, 0xC);
    uint16_t slot = items + (uint16_t)((int8_t)rd8(s, RAM_gCurSubstateZ) * 4);
    eepromReadToRamAlso(s, 0x8F8C + rd8(s, RAM_gCurSubstateY) * 2, slot, 2);
    eepromWriteData(s, 0xCEBC, items, 0xC);
    uint16_t team = static_malloc(s, 0xBE);
    eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
    uint16_t buf = static_malloc(s, 0x88);
    uint16_t item = rd16(s, team + 0x8C + rd8(s, RAM_gCurSubstateY) * 2);
    logEvent(s, team, buf, 0xB, 0, item, 0);
}

void sub_A3AA(pw_state_t *s) {
    wr8(s, RAM_common_bit_flags, rd8(s, RAM_common_bit_flags) | 4);
    if (rd32(s, RAM_RamCache_totalSteps) < 9999999 && rd32(s, RAM_RamCache_STEP_COUNT_maybe) < 9999999) {
        wr32(s, RAM_RamCache_STEP_COUNT_maybe, rd32(s, RAM_RamCache_STEP_COUNT_maybe) + 1);
    }
    writeReliableDataToEeprom(s, EEPROM_HEALTH0, EEPROM_HEALTH1, RAM_RamCache_totalSteps, HEALTH_SIZE);
    if (rd8(s, RAM_walker_status_flags) & 4) {
        setHeapPointer(s);
        uint16_t team = static_malloc(s, 0xBE);
        eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
        uint8_t special = rd8(s, RAM_RamCache_settingsByte) & 1;
        uint16_t buf = static_malloc(s, 0x88);
        logEvent(s, team, buf, 0x1B, special, 0, 0);
    }
    wr16(s, RAM_someWalkingRelatedState, 0);
    if (rd8(s, RAM_byte_FFF7A8) == rd8(s, RAM_currentHour)) {
        wr8(s, RAM_byte_FFF7A7, rd8(s, RAM_byte_FFF7A7) | 4);
    }
}

#define ROM_GIFT_SUBSTATE_B 0xBD84

void addRandomGift(pw_state_t *s, uint8_t type) {
    uint16_t item = 0;
    wr8(s, RAM_gCurSubstateY, type);
    uint16_t b = ROM_GIFT_SUBSTATE_B + type * 2;
    wr16(s, RAM_gCurSubstateB, (uint16_t)(rom_rd8(b) << 8 | rom_rd8((uint16_t)(b + 1))));
    setCurrentlyActiveView(s, 0xC);
    wr16(s, RAM_word_FFF7A2, 0);
    switch (type) {
    case 1: {
        uint16_t walked = rd16(s, RAM_someWalkingRelatedState);
        wr8(s, RAM_gCurSubstateZ, walked < 4500 ? 9 - walked / 500 : 0);
        item = getItemIdForNthItemOnRoute(s, rd8(s, RAM_gCurSubstateZ));
        setHeapPointer(s);
        uint16_t items = static_malloc(s, 0xC);
        eepromReadToRamAlso(s, 0xCEBC, items, 0xC);
        uint8_t slot = findFreeItemSlot(s, items);
        if (slot < 3) {
            wr16(s, items + slot * 4, item);
            eepromWriteData(s, 0xCEBC, items, 0xC);
        }
        break;
    }
    case 2:
    case 3:
    case 4: {
        uint8_t watts = type == 2 ? 50 : type == 3 ? 20 : 10;
        wr8(s, RAM_gCurSubstateZ, watts);
        addWatts(s, watts);
        break;
    }
    case 7: pokemonJoinsEmptyWalkerWalk(s); break;
    default: wr8(s, RAM_gCurSubstateZ, 0); break;
    }
    setHeapPointer(s);
    uint16_t team = static_malloc(s, 0xBE);
    eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
    uint8_t special = rd8(s, RAM_RamCache_settingsByte) & 1;
    uint16_t buf = static_malloc(s, 0x88);
    logEvent(s, team, buf, (uint8_t)(type + 0x10), special, item, 0);
    if (type >= 2 && type <= 5) {
        uint32_t rem;
        divmod32(rand32(s) >> 3, 3, &rem);
        wr8(s, RAM_gCurSubstateA, (uint8_t)rem);
    } else {
        wr8(s, RAM_gCurSubstateA, 0);
    }
}

void calculateAndApplyPeerGift(pw_state_t *s) {
    setHeapPointer(s);
    static_malloc(s, 0xBE);
    uint16_t peer = static_malloc(s, 0x38);
    eepromReadToRamAlso(s, 0xF6C0, peer, 0x38);
    uint16_t items = static_malloc(s, 0x28);
    eepromReadToRamAlso(s, 0xCEC8, items, 0x28);
    uint32_t score = rd32(s, RAM_stepCountTodaySoFar) + rd32(s, peer);
    uint16_t walked = (uint16_t)(rd16(s, RAM_someWalkingRelatedState) + rd16(s, peer + 4));
    score += (uint16_t)(walked * 10);
    if (score > 20000) {
        score = 20000;
    }
    uint8_t slot = 0;
    while (slot < 10 && rd16(s, items + slot * 4)) {
        slot++;
    }
    wr8(s, RAM_gCurSubstateB + 1, 0);
    if (slot >= 10) {
        uint32_t watts = score / 200;
        if ((uint8_t)watts == 0)
            watts = 1;
        if ((uint8_t)watts > 99)
            watts = 99;
        wr8(s, RAM_gCurSubstateB + 1, (uint8_t)watts);
        addWatts(s, (uint8_t)watts);
    }
    uint8_t band = score >= 20000 ? 0 : score >= 10000 ? 1 : score >= 5000 ? 2 : score >= 2500 ? 3 : 4;
    wr8(s, RAM_gCurSubstateA + 1, 0x2C + band);
    if (rd8(s, RAM_gCurSubstateB + 1)) {
        return;
    }
    uint8_t n = band * 2 + (rd32(s, RAM_stepCountTodaySoFar) <= rd32(s, peer));
    wr8(s, RAM_gCurSubstateB, n);
    wr16(s, items + slot * 4, getItemIdForNthItemOnRoute(s, n));
    eepromWriteData(s, 0xCEC8, items, 0x28);
}

static void eeprom_copy(pw_state_t *s, uint16_t from, uint16_t to, uint16_t buf, uint16_t len) {
    eepromReadToRamAlso(s, from, buf, len);
    eepromWriteData(s, to, buf, len);
}

void recordPokeCaughtNormal(pw_state_t *s) {
    uint8_t y = rd8(s, RAM_gCurSubstateY) - 1;
    if (y > 3) {
        return;
    }
    setHeapPointer(s);
    if (y < 3) {
        uint16_t caught = static_malloc(s, 0x30);
        eepromReadToRamAlso(s, 0xCE8C, caught, 0x30);
        uint8_t slot = findFirstEmptyCaughtSlot(s, caught);
        if (slot >= 3) {
            wr8(s, RAM_gCurSubstateA, 0);
            set_gCurSubstateZ_to_1(s);
            setCurrentlyActiveView(s, 7);
            return;
        }
        eepromReadToRamAlso(s, 0x8F52 + y * 0x10, caught + slot * 0x10, 0x10);
        eepromWriteData(s, 0xCE8C, caught, 0x30);
        uint16_t team = static_malloc(s, 0xBE);
        eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
        uint16_t buf = static_malloc(s, 0x88);
        logEvent(s, team, buf, 0xD, 0, 0, rd8(s, RAM_gCurSubstateY));
        return;
    }
    uint16_t events = static_malloc(s, 0x68);
    uint8_t event = eepromReadSingleByte(s, 0xBF7A);
    if (hasParticipatedInEvent(s, events, event)) {
        return;
    }
    uint8_t flags = eepromReadSingleByte(s, 0xB800);
    if (flags & 0x20) {
        return;
    }
    uint16_t buf = static_malloc(s, 0x170);
    eeprom_copy(s, 0xBF08, 0xBA44, buf, 0x10);
    eeprom_copy(s, 0xBF18, 0xBA54, buf, 0x2C);
    eeprom_copy(s, 0xBF7C, 0xBA80, buf, 0x170);
    eeprom_copy(s, 0xC6FC, 0xBC00, buf, 0x140);
    eepromWriteSingleByte(s, 0xB800, flags | 0x20);
    setThatWeveParticipatedInAGivenEvent(s, events, event);
    uint16_t team = static_malloc(s, 0xBE);
    eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
    uint16_t log = static_malloc(s, 0x88);
    logEvent(s, team, log, 0xE, 1, 0, rd8(s, RAM_gCurSubstateY));
}

void maybeFoundSomeEventItem(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t events = static_malloc(s, 0x68);
    uint16_t held = static_malloc(s, 0x188);
    uint16_t route = static_malloc(s, 0x7C);
    eepromReadToRamAlso(s, 0xBF00, route, 0x7C);
    uint16_t threshold = rd16(s, route + 0x4A);
    threshold = (uint16_t)(threshold << 8 | threshold >> 8);
    int happening = 0;
    if (rd32(s, RAM_stepCountTodaySoFar) >= threshold && (uint8_t)(rand32(s) >> 3) % 100 < rd8(s, route + 0x4C)) {
        readSpecialItemInfo(s, held);
        happening = rd16(s, held + 6) == 0 && !hasParticipatedInEvent(s, events, rd8(s, route + 0x7B));
    }
    if (!happening) {
        uint8_t watts = (uint8_t)(rd8(s, RAM_gCurSubstateB) * 4 + 2);
        wr8(s, RAM_gCurSubstateD, watts);
        addWatts(s, watts);
        return;
    }
    setThatWeveParticipatedInAGivenEvent(s, events, rd8(s, route + 0x7B));
    uint16_t team = static_malloc(s, 0xBE);
    eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
    wr8(s, RAM_gCurSubstateY, 0xA);
    wr32(s, held, rd32(s, route));
    wr16(s, held + 4, rd16(s, route + 4));
    wr16(s, held + 6, rd16(s, route + 0x48));
    eepromReadToRamAlso(s, 0xCA3C, held + 8, 0x180);
    writeSpecialItemInfo(s, held);
    eepromWriteSingleByte(s, 0xB800, eepromReadSingleByte(s, 0xB800) | 0x40);
    if (rd8(s, RAM_walker_status_flags) & 4) {
        uint16_t log = static_malloc(s, 0x88);
        logEvent(s, team, log, 0xC, 1, rd16(s, held + 6), 0);
    }
}

static void write_reliable_byte(pw_state_t *s, uint16_t addr0, uint16_t addr1, uint8_t v) {
    eepromWriteSingleByte(s, addr0, v);
    eepromWriteSingleByte(s, addr0 + 1, (uint8_t)(v + 1));
    eepromWriteSingleByte(s, addr1, v);
    eepromWriteSingleByte(s, addr1 + 1, (uint8_t)(v + 1));
}

#define EEPROM_COPY_MARKER0 0x016F
#define EEPROM_COPY_MARKER1 0x026F

void startWalkEepromActions(pw_state_t *s) {
    write_reliable_byte(s, EEPROM_COPY_MARKER0, EEPROM_COPY_MARKER1, 0xA5);
    copyTeamAndconfigDataFromStagingArea(s);
    write_reliable_byte(s, EEPROM_COPY_MARKER0, EEPROM_COPY_MARKER1, 0);
    for (uint16_t i = 0; i < 24; i++) {
        eepromWriteSingleByte(s, EEPROM_EVENT_LOG_BASE + i * EVENT_LOG_ENTRY_SIZE + 0x84, 0);
    }
    eepromFillWithByte(s, 0xDE24, 0x1568, 0);
    wr16(s, RAM_someWalkingRelatedState, 0);
    wr16(s, RAM_word_FFF790, 0);
    wr8(s, RAM_likelyCurEventLogPointer, 0);
    wr16(s, RAM_RamCache_curWatts, 0);
    writeReliableDataToEeprom(s, EEPROM_HEALTH0, EEPROM_HEALTH1, RAM_RamCache_totalSteps, HEALTH_SIZE);
    wr8(s, RAM_walker_status_flags, rd8(s, RAM_walker_status_flags) | 6);
    uint16_t info = RAM_RamCache_ED_data;
    readReliableDataFromEeprom(s, EEPROM_WALKER_INFO0, EEPROM_WALKER_INFO1, info, WALKER_INFO_SIZE);
    wr8(s, info + 0x5B, (rd8(s, info + 0x5B) | 3) & ~4);
    wr32(s, info, rd32(s, RAM_sometimes_new_rxed_data_for_0xED));
    wr32(s, info + 4, rd32(s, RAM_dword_FFF7EA));
    wr16(s, info + 8, rd16(s, RAM_word_FFF7EE));
    wr16(s, info + 0xA, rd16(s, RAM_word_FFF7F0));
    wr32(s, info + 0xC, rd32(s, RAM_dword_FFF7F2));
    for (uint16_t i = 0; i < 0x12; i++) {
        wr8(s, info + 0x48 + i, rd8(s, RAM_byte_FFF82E + i));
    }
    wr8(s, info + 0x5C, rd8(s, RAM_byte_FFF842));
    wr8(s, info + 0x5D, rd8(s, RAM_byte_FFF843));
    wr8(s, info + 0x5E, 0);
    wr8(s, info + 0x5F, 2);
    writeReliableDataToEeprom(s, EEPROM_WALKER_INFO0, EEPROM_WALKER_INFO1, info, WALKER_INFO_SIZE);
    setHeapPointer(s);
    uint16_t team = static_malloc(s, 0xBE);
    eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
    uint8_t special = rd8(s, RAM_RamCache_settingsByte) & 1;
    uint16_t buf = static_malloc(s, 0x88);
    logEvent(s, team, buf, 0x19, special, 0, 0);
    eraseCaughtPokemonAndFoundItems(s);
}

void logPeerPlay(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t team = static_malloc(s, 0xBE);
    eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
    uint16_t peer = static_malloc(s, 0x38);
    eepromReadToRamAlso(s, 0xF6C0, peer, 0x38);
    uint16_t buf = static_malloc(s, 0x88);
    wr32(s, buf + 4, rd32(s, peer + 8));
    wr16(s, buf + 8, rd16(s, peer + 0xC));
    wr16(s, buf + 0xC, rd16(s, peer + 0xE));
    uint8_t f = rd8(s, peer + 0x36);
    log_pokemon_flags(s, buf + 0x86, f, f >> 5 & 3);
    wr8(s, buf + 0x86, (rd8(s, buf + 0x86) & 0x7F) | (f & 0x80));
    wr16(s, buf + 0x7A, rd16(s, peer + 4));
    wr32(s, buf + 0x80, rd32(s, peer));
    for (uint16_t i = 0; i < 0x16; i++) {
        wr8(s, buf + 0x36 + i, rd8(s, peer + 0x10 + i));
    }
    for (uint16_t i = 0; i < 0x12; i++) {
        wr8(s, buf + 0x10 + i, rd8(s, peer + 0x26 + i));
    }
    uint8_t item = rd8(s, RAM_gCurSubstateB);
    if (rd8(s, RAM_gCurSubstateB + 1) || item >= 10) {
        return;
    }
    uint8_t special = rd8(s, RAM_RamCache_settingsByte) & 1;
    logEvent(s, team, buf, (uint8_t)(item + 1), special, rd16(s, team + 0x8C + item * 2), 0);
}

void goToPeerPlayMicroApp(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t peer = static_malloc(s, 0x38);
    eepromReadToRamAlso(s, 0xF6C0, peer, 0x38);
    wr8(s, RAM_gCurSubstateY, (rd8(s, RAM_gCurSubstateY) & ~2) | (rd8(s, peer + 0x37) & 1) << 1);
    wr8(s, RAM_gCurSubstateZ, 0);
    wr8(s, RAM_gCurSubstateA, 0);
    calculateAndApplyPeerGift(s);
    pushSeenTrainersLogDown(s);
    logPeerPlay(s);
}

void sub_A34A(pw_state_t *s) {
    if (rd8(s, RAM_common_bit_flags) & 0x20) {
        return;
    }
    if (rd8(s, RAM_byte_FFF7A7) & 1)
        incrementSaturating_F790(s);
    if (rd8(s, RAM_byte_FFF7A7) & 2)
        sub_A3AA(s);
    if (rd8(s, RAM_byte_FFF7A7) & 4)
        dayEnded(s);
    wr8(s, RAM_byte_FFF7A7, rd8(s, RAM_byte_FFF7A7) & 0xF8);
}

void some_sort_of_eeprom_init(pw_state_t *s, uint8_t forget_trainer, uint8_t full) {
    wr32(s, RAM_stepCountTodaySoFar, 0);
    wr16(s, RAM_someWalkingRelatedState, 0);
    wr8(s, RAM_walker_status_flags, rd8(s, RAM_walker_status_flags) & ~6);
    uint16_t info = get_packet_data_pointer(s);
    readReliableDataFromEeprom(s, EEPROM_WALKER_INFO0, EEPROM_WALKER_INFO1, info, WALKER_INFO_SIZE);
    wr32(s, info, 0);
    wr32(s, info + 4, 0);
    wr16(s, info + 8, 0);
    wr16(s, info + 0xA, 0);
    wr32(s, info + 0xC, 0);
    for (uint16_t i = 0; i < 0x12; i++) {
        wr8(s, info + 0x48 + i, 0);
    }
    if (forget_trainer) {
        for (uint16_t i = 0; i < 0x10; i++) {
            wr8(s, info + 0x38 + i, 0);
        }
    }
    wr8(s, info + 0x5B, 0);
    wr8(s, info + 0x5A, 0);
    wr8(s, info + 0x5E, 0);
    wr8(s, info + 0x5F, 2);
    wr32(s, info + 0x60, 0);
    readReliableDataFromEeprom(s, 0x0083, 0x0183, info + 0x10, 0x28);
    if (full) {
        wr32(s, info + 0x64, 0);
    }
    writeReliableDataToEeprom(s, EEPROM_WALKER_INFO0, EEPROM_WALKER_INFO1, info, WALKER_INFO_SIZE);
    wipeOutStepsAndSyncTime(s, full);
    if (full) {
        eepromFillWithByte(s, 0xCE80, 0xD4C, 0);
    } else {
        eraseCaughtPokemonAndFoundItems(s);
        clearEventLogEntries(s);
        fill_eeprom_0xCEF0_to_CF0B_with_zero(s);
    }
    if (forget_trainer) {
        eepromFillWithByte(s, 0xB800, 0x6C8, 0);
    }
    eepromFillWithByte(s, 0xDE24, 0x1568, 0);
}

#define EEPROM_SOUND_TABLE 0x8CB0
#define EEPROM_SOUND_DATA  0x8CF0

void playSound(pw_state_t *s, uint8_t idx) {
    if (!rd8(s, RAM_likelyVolume)) {
        return;
    }
    uint16_t entry = EEPROM_SOUND_TABLE + idx * 4;
    uint16_t offset = eepromReadSingleByte(s, entry) | eepromReadSingleByte(s, entry + 1) << 8;
    uint8_t len = eepromReadSingleByte(s, entry + 2);
    uint8_t sum = eepromReadSingleByte(s, entry + 3);
    if (len > 0xC0) {
        return;
    }
    uint16_t buf = RAM_ACCEL_SAMPLES_X;
    wr16(s, RAM_word_FFF7C4, buf);
    eepromReadToRamAlso(s, (uint16_t)(EEPROM_SOUND_DATA + offset), buf, len);
    uint8_t total = 0;
    for (uint16_t i = 0; i < len / 2; i++) {
        total += rd8(s, buf + i * 2) + rd8(s, buf + i * 2 + 1);
    }
    if (total != sum || (rd8(s, (uint16_t)(buf + len / 2 * 2 - 1)) & 0x7F) < 0x7E) {
        wr16(s, RAM_word_FFF7C4, 0);
        return;
    }
    wr16(s, RAM_word_FFF7C8, 0);
    wr16(s, RAM_word_FFF7CA, 0);
}
