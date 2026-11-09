#include "pw_internal.h"

void interactionHandlerPokeRadarFailureUserTooSlow(pw_state_t *s) {
    if (!checkKeyStates(s, KEY_ANY)) {
        return;
    }
    wr8(s, RAM_gCurSubstateA, 0);
    playSound(s, 4);
    resetSubstates(s);
    setCurrentlyActiveView(s, 0);
}

void interactionHandlerRandomGiftFromBoredPokemon(pw_state_t *s) {
    if (!checkKeyStates(s, KEY_ENTER)) {
        return;
    }
    uint16_t step = rd16(s, RAM_gCurSubstateB);
    if (mem_rd8(s, step) & 1) {
        resetSubstates(s);
        setCurrentlyActiveView(s, 0);
        return;
    }
    step += 4;
    wr16(s, RAM_gCurSubstateB, step);
    uint8_t sound = mem_rd8(s, step + 1);
    if (sound != 0x10) {
        playSound(s, sound);
    }
}

void interactionHandlerPokemonArrivedAnimation(pw_state_t *s) {
    uint8_t z = rd8(s, RAM_gCurSubstateZ);
    switch (rd8(s, RAM_gCurSubstateY)) {
    case 0:
        if (z > 4) {
            wr8(s, RAM_gCurSubstateY, 1);
            wr8(s, RAM_gCurSubstateZ, 0);
        }
        break;
    case 1:
        if (z) {
            wr8(s, RAM_gCurSubstateY, 2);
            wr8(s, RAM_gCurSubstateZ, 0);
            playSound(s, 0);
        }
        break;
    case 2:
        if (z > 8) {
            wr8(s, RAM_gCurSubstateZ, 0);
            wr8(s, RAM_gCurSubstateY, 3);
            playSound(s, 6);
        }
        break;
    }
}

void interactionHandlerWalkEnd(pw_state_t *s) {
    uint8_t z = rd8(s, RAM_gCurSubstateZ);
    switch (rd8(s, RAM_gCurSubstateY)) {
    case 5:
        wr8(s, RAM_gCurSubstateZ, 0);
        wr8(s, RAM_gCurSubstateY, 2);
        playSound(s, 0);
        break;
    case 2:
        if (z > 8) {
            wr8(s, RAM_gCurSubstateZ, 0);
            wr8(s, RAM_gCurSubstateY, 1);
        }
        break;
    case 0:
        if (z >= 9) {
            wr8(s, RAM_gCurSubstateY, 3);
            wr8(s, RAM_gCurSubstateZ, 0);
            playSound(s, 6);
        }
        break;
    case 6:
        wr8(s, RAM_gCurSubstateY, 7);
        wr8(s, RAM_gCurSubstateZ, 0);
        playSound(s, 6);
        break;
    }
}

void interactionHandlerSwitchScreen(pw_state_t *s) {
    if (checkKeyStates(s, KEY_LEFT)) {
        uint8_t z = rd8(s, RAM_gCurSubstateZ);
        if (!z) {
            resetSubstates(s);
            setCurrentlyActiveView(s, 0);
            playSound(s, 1);
            return;
        }
        wr8(s, RAM_gCurSubstateZ, z - 1);
        playSound(s, 2);
    }
    if (checkKeyStates(s, KEY_RIGHT)) {
        uint8_t z = rd8(s, RAM_gCurSubstateZ);
        if (z == 2) {
            playSound(s, 1);
            return;
        }
        wr8(s, RAM_gCurSubstateZ, z + 1);
        playSound(s, 2);
    }
    if (checkKeyStates(s, KEY_ENTER)) {
        if (!rd8(s, RAM_gCurSubstateA)) {
            recordPokeCaughtNoEventConsideration(s);
        } else {
            recordItemFoundNoEventConsideration(s);
        }
        resetSubstates(s);
        setCurrentlyActiveView(s, 0);
        playSound(s, 0);
    }
}

void interactionHandlerGiftedItemsScreen(pw_state_t *s) {
    if (checkKeyStates(s, KEY_LEFT)) {
        if (cursorToPrevOwnedSlot(s, rd16(s, RAM_gCurSubstateB))) {
            if (!rd16(s, RAM_gCurSubstateA)) {
                playSound(s, 1);
                return;
            }
            wr8(s, RAM_gCurSubstateY, 0);
            cursorToPrevOwnedSlotWrapping(s, rd16(s, RAM_gCurSubstateA));
            setCurrentlyActiveView(s, 0xA);
            playSound(s, 2);
            return;
        }
        playSound(s, 2);
    }
    if (checkKeyStates(s, KEY_RIGHT)) {
        if (cursorToNextOwnedSlot(s, rd16(s, RAM_gCurSubstateB))) {
            playSound(s, 1);
            return;
        }
        playSound(s, 2);
    }
    if (checkKeyStates(s, KEY_ENTER)) {
        resetSubstates(s);
        setCurrentlyActiveView(s, 0);
        playSound(s, 0);
    }
}

void interactionHandlerPokemonAndItemsScreen(pw_state_t *s) {
    if (checkKeyStates(s, KEY_LEFT)) {
        if (cursorToPrevOwnedSlot(s, rd16(s, RAM_gCurSubstateA))) {
            clear_gCurSubstateY(s);
            setCurrentlyActiveView(s, 1);
            playSound(s, 1);
            return;
        }
        playSound(s, 2);
    }
    if (checkKeyStates(s, KEY_RIGHT)) {
        if (cursorToNextOwnedSlot(s, rd16(s, RAM_gCurSubstateA))) {
            if (!rd16(s, RAM_gCurSubstateB)) {
                playSound(s, 1);
                return;
            }
            cursorToFirstSlotOfB(s);
            setCurrentlyActiveView(s, 0xB);
            playSound(s, 2);
            return;
        }
        playSound(s, 2);
    }
    if (checkKeyStates(s, KEY_ENTER)) {
        if (rd16(s, RAM_gCurSubstateB)) {
            cursorToFirstSlotOfB(s);
            setCurrentlyActiveView(s, 0xB);
        } else {
            resetSubstates(s);
            setCurrentlyActiveView(s, 0);
        }
        playSound(s, 0);
    }
}

static void settings_choose(pw_state_t *s) {
    if (checkKeyStates(s, KEY_LEFT)) {
        if (!rd8(s, RAM_gCurSubstateZ)) {
            playSound(s, 1);
            clear_gCurSubstateY(s);
            setCurrentlyActiveView(s, 1);
            return;
        }
        wr8(s, RAM_gCurSubstateZ, 0);
        playSound(s, 2);
    }
    if (checkKeyStates(s, KEY_RIGHT) && rd8(s, RAM_gCurSubstateZ) != 1) {
        wr8(s, RAM_gCurSubstateZ, 1);
        playSound(s, 2);
    }
}

static uint8_t settings_volume(pw_state_t *s) {
    return rd8(s, RAM_RamCache_settingsByte) >> 1 & 3;
}

static void settings_sound(pw_state_t *s) {
    if (checkKeyStates(s, KEY_LEFT)) {
        bitfieldInsert(s, RAM_RamCache_settingsByte, (settings_volume(s) + 2) % 3, 2, 5);
        setVolume(s, settings_volume(s));
        playSound(s, 2);
    }
    if (checkKeyStates(s, KEY_RIGHT)) {
        bitfieldInsert(s, RAM_RamCache_settingsByte, (settings_volume(s) + 1) % 3, 2, 5);
        setVolume(s, settings_volume(s));
        playSound(s, 2);
    }
}

static uint8_t settings_contrast(pw_state_t *s) {
    return rd8(s, RAM_RamCache_settingsByte) >> 3 & 0xF;
}

static void settings_shade(pw_state_t *s) {
    if (checkKeyStates(s, KEY_LEFT)) {
        if (rd8(s, RAM_RamCache_settingsByte) & 0x78) {
            bitfieldInsert(s, RAM_RamCache_settingsByte, settings_contrast(s) - 1, 4, 1);
            playSound(s, 2);
        }
        lcdSetContrast(s, settings_contrast(s));
    }
    if (checkKeyStates(s, KEY_RIGHT)) {
        if ((rd8(s, RAM_RamCache_settingsByte) & 0x78) < 0x48) {
            bitfieldInsert(s, RAM_RamCache_settingsByte, settings_contrast(s) + 1, 4, 1);
            playSound(s, 2);
        }
        lcdSetContrast(s, settings_contrast(s));
    }
}

void interactionHandlerSettings(pw_state_t *s) {
    switch (rd8(s, RAM_gCurSubstateY)) {
    case 0: settings_choose(s); break;
    case 1: settings_sound(s); break;
    case 2: settings_shade(s); break;
    }
    if (!checkKeyStates(s, KEY_ENTER)) {
        return;
    }
    if (!rd8(s, RAM_gCurSubstateY)) {
        playSound(s, 0);
        wr8(s, RAM_gCurSubstateY, rd8(s, RAM_gCurSubstateZ) + 1);
        return;
    }
    playSound(s, 0);
    resetSubstates(s);
    setCurrentlyActiveView(s, 0);
    writeReliableDataToEeprom(s, 0x0156, 0x0256, RAM_RamCache_totalSteps, 0x18);
}

void interactionHandlerTrainerCardAndStatsScreens(pw_state_t *s) {
    if (!rd8(s, RAM_gCurSubstateZ)) {
        if (checkKeyStates(s, KEY_LEFT)) {
            uint8_t y = rd8(s, RAM_gCurSubstateY);
            if (!y) {
                playSound(s, 1);
                clear_gCurSubstateY(s);
                setCurrentlyActiveView(s, 1);
                return;
            }
            wr8(s, RAM_gCurSubstateY, y - 1);
            playSound(s, 2);
        }
        if (checkKeyStates(s, KEY_RIGHT) && rd8(s, RAM_gCurSubstateY) < 7) {
            wr8(s, RAM_gCurSubstateY, rd8(s, RAM_gCurSubstateY) + 1);
            playSound(s, 2);
        }
    }
    if (!checkKeyStates(s, KEY_ENTER)) {
        return;
    }
    switch (rd8(s, RAM_gCurSubstateZ)) {
    case 0:
        if (rd32(s, RAM_RamCache_totalSteps) >= 9999999) {
            wr8(s, RAM_gCurSubstateZ, 1);
            playSound(s, 0);
            return;
        }
        break;
    case 1: {
        uint8_t seen = eepromReadSingleByte(s, 0xCE88);
        if (!(seen & 1)) {
            eepromWriteSingleByte(s, 0xCE88, seen | 1);
            wr8(s, RAM_gCurSubstateZ, 2);
            playSound(s, 7);
            return;
        }
        break;
    }
    case 2: break;
    default: return;
    }
    playSound(s, 0);
    resetSubstates(s);
    setCurrentlyActiveView(s, 0);
}

void interactionHandlerSomethingReceived(pw_state_t *s) {
    uint8_t z = rd8(s, RAM_gCurSubstateZ);
    switch (rd8(s, RAM_gCurSubstateY)) {
    case 0:
        if (z > 4) {
            wr8(s, RAM_gCurSubstateY, 4);
            wr8(s, RAM_gCurSubstateZ, 0);
            playSound(s, 0);
        }
        return;
    case 1:
        if (z) {
            wr8(s, RAM_gCurSubstateY, 3);
            wr8(s, RAM_gCurSubstateZ, 0);
            playSound(s, 6);
        }
        return;
    case 3: break;
    default: return;
    }
    if (is_F7C4_nonzero(s) || rd8(s, RAM_gCurSubstateZ) <= 8) {
        return;
    }
    uint8_t what = rd8(s, RAM_gCurSubstateA);
    if (what == 0 || what == 2) {
        setHeapPointer(s);
        uint16_t team = static_malloc(s, 0xBE);
        eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
        uint8_t special = rd8(s, RAM_RamCache_settingsByte) & 1;
        if (what == 0) {
            uint16_t poke = static_malloc(s, 0x10);
            eepromReadToRamAlso(s, 0xBA44, poke, 0x10);
            uint16_t log = static_malloc(s, 0x88);
            logEvent(s, team, log, 0x1D, special, 0, 4);
        } else {
            uint16_t held = static_malloc(s, 0x188);
            eepromReadToRamAlso(s, 0xBD40, held, 0x188);
            uint16_t log = static_malloc(s, 0x88);
            logEvent(s, team, log, 0x1C, special, rd16(s, held + 6), 0);
        }
    }
    resetSubstates(s);
    setCurrentlyActiveView(s, 0);
}

#define ROM_RADAR_WAIT  0xBF1A
#define ROM_RADAR_DELAY 0xBF1E

void pickPokemonToEncounter(pw_state_t *s) {
    wr8(s, RAM_gCurSubstateY, 0);
    if (rd8(s, RAM_RamCache_settingsByte) & 1) {
        uint8_t event = eepromReadSingleByte(s, 0xBF7A);
        setHeapPointer(s);
        uint16_t events = static_malloc(s, 0x68);
        if (!hasParticipatedInEvent(s, events, event) && !(eepromReadSingleByte(s, 0xB800) & 0x20)) {
            setHeapPointer(s);
            uint16_t rule = static_malloc(s, 4);
            eepromReadToRamAlso(s, 0xBF44, rule, 4);
            uint16_t steps = rd16(s, rule);
            steps = (uint16_t)(steps << 8 | steps >> 8);
            if (rd32(s, RAM_stepCountTodaySoFar) >= steps && rd8(s, rule + 2) > rand32(s) % 100) {
                wr8(s, RAM_gCurSubstateY, 4);
                wr8(s, RAM_gCurSubstateB, ((uint16_t)rand32(s) >> 3 & 1) + 3);
                return;
            }
        }
    }
    setHeapPointer(s);
    uint16_t caught = static_malloc(s, 0x30);
    eepromReadToRamAlso(s, 0xCE8C, caught, 0x30);
    uint8_t roll = (uint8_t)(rand32(s) % 100);
    setHeapPointer(s);
    uint16_t team = static_malloc(s, 0xBE);
    eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
    for (uint8_t i = 0; i < 3; i++) {
        uint16_t steps = rd16(s, team + 0x82 + i * 2);
        steps = (uint16_t)(steps << 8 | steps >> 8);
        if (rd32(s, RAM_stepCountTodaySoFar) < steps || roll >= rd8(s, team + 0x88 + i)) {
            continue;
        }
        wr8(s, RAM_gCurSubstateY, i + 1);
        wr8(s, RAM_gCurSubstateB, (uint8_t)(((uint16_t)rand32(s) >> 3 & 1) - i + 3));
        return;
    }
    wr8(s, RAM_gCurSubstateY, 3);
    wr8(s, RAM_gCurSubstateB, ((uint16_t)rand32(s) >> 3 & 1) + 1);
}

static uint8_t radar_random_bush(pw_state_t *s) {
    return (uint16_t)((uint16_t)rand32(s) << 3) >> 8 & 3;
}

void goToPokeRadarMicroApp(pw_state_t *s) {
    pickPokemonToEncounter(s);
    wr8(s, RAM_gCurSubstateZ, 0);
    wr8(s, RAM_gCurSubstateA, 0);
    wr8(s, RAM_gCurSubstateA + 1, 0);
    wr8(s, RAM_gCurSubstateC, 5);
    wr8(s, RAM_gCurSubstateD, rom_rd8(ROM_RADAR_WAIT));
    wr8(s, RAM_gCurSubstateB + 1, radar_random_bush(s));
    wr8(s, RAM_gCurSubstateE, 0);
}

void startBattle(pw_state_t *s) {
    wr8(s, RAM_gCurSubstateZ, 0);
    wr8(s, RAM_gCurSubstateA, 4);
    wr8(s, RAM_gCurSubstateA + 1, 4);
    wr8(s, RAM_gCurSubstateB, 0);
    wr8(s, RAM_gCurSubstateB + 1, 6);
    wr8(s, RAM_gCurSubstateC, 0x38);
    wr8(s, RAM_gCurSubstateD, 0xE0);
    wr8(s, RAM_gCurSubstateF, rd8(s, RAM_gCurSubstateF) & 0x1E);
    wr8(s, RAM_gCurSubstateF + 1, 0);
    playSound(s, 0xA);
}

static void radar_choose(pw_state_t *s) {
    if (checkKeyStates(s, KEY_LEFT)) {
        wr8(s, RAM_gCurSubstateA, (rd8(s, RAM_gCurSubstateA) + 3) & 3);
        playSound(s, 2);
    }
    if (checkKeyStates(s, KEY_RIGHT)) {
        wr8(s, RAM_gCurSubstateA, (rd8(s, RAM_gCurSubstateA) + 1) & 3);
        playSound(s, 2);
    }
    if (checkKeyStates(s, KEY_ENTER) && rd8(s, RAM_gCurSubstateD)) {
        if (rd8(s, RAM_gCurSubstateA) == rd8(s, RAM_gCurSubstateB + 1)) {
            playSound(s, 3);
            wr8(s, RAM_gCurSubstateZ, 3);
            wr8(s, RAM_gCurSubstateE, 0x10);
            return;
        }
        if (!rd8(s, RAM_gCurSubstateA + 1)) {
            playSound(s, 4);
            return;
        }
    } else {
        if (rd8(s, RAM_gCurSubstateC)) {
            wr8(s, RAM_gCurSubstateC, rd8(s, RAM_gCurSubstateC) - 1);
            return;
        }
        if (rd8(s, RAM_gCurSubstateD)) {
            wr8(s, RAM_gCurSubstateD, rd8(s, RAM_gCurSubstateD) - 1);
        }
        if (rd8(s, RAM_gCurSubstateD)) {
            return;
        }
    }
    playSound(s, 0xE);
    setCurrentlyActiveView(s, 6);
}

void interactionHandlerPokeRadar(pw_state_t *s) {
    if (is_F7C4_nonzero(s)) {
        return;
    }
    switch (rd8(s, RAM_gCurSubstateZ)) {
    case 0: radar_choose(s); break;
    case 1:
        if (rd8(s, RAM_gCurSubstateD) > 4) {
            startBattle(s);
            setCurrentlyActiveView(s, 4);
        }
        break;
    case 2:
        if (checkKeyStates(s, KEY_ANY)) {
            playSound(s, 0);
            resetSubstates(s);
            setCurrentlyActiveView(s, 0);
        }
        break;
    case 3: {
        uint8_t e = rd8(s, RAM_gCurSubstateE);
        if (!e) {
            break;
        }
        wr8(s, RAM_gCurSubstateE, --e);
        if (e) {
            break;
        }
        uint8_t round = rd8(s, RAM_gCurSubstateA + 1);
        if ((int16_t)round >= (int16_t)(rd8(s, RAM_gCurSubstateB) - 1)) {
            wr8(s, RAM_gCurSubstateZ, 1);
            wr8(s, RAM_gCurSubstateE, 1);
            wr8(s, RAM_gCurSubstateD, 0);
            break;
        }
        wr8(s, RAM_gCurSubstateZ, 0);
        uint8_t r = (uint8_t)(rand32(s) >> 2);
        uint8_t range = rom_rd8(ROM_RADAR_DELAY + round);

        uint8_t delay = range ? r % range : 0;
        wr8(s, RAM_gCurSubstateC, (uint8_t)(delay + 0x10));
        wr8(s, RAM_gCurSubstateA + 1, ++round);
        wr8(s, RAM_gCurSubstateD, rom_rd8(ROM_RADAR_WAIT + round));
        wr8(s, RAM_gCurSubstateB + 1, radar_random_bush(s));
        break;
    }
    }
}

void dowsingFindItem(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t items = static_malloc(s, 0xC);
    eepromReadToRamAlso(s, 0xCEBC, items, 0xC);
    wr8(s, RAM_gCurSubstateE, findFreeItemSlot(s, items));
    if (rd8(s, RAM_RamCache_settingsByte) & 1) {
        maybeFoundSomeEventItem(s);
        return;
    }
    uint16_t team = static_malloc(s, 0xBE);
    eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
    uint8_t i;
    for (i = 0; i < 10; i++) {
        uint16_t steps = rd16(s, team + 0xA0 + i * 2);
        steps = (uint16_t)(steps << 8 | steps >> 8);
        if (rd32(s, RAM_stepCountTodaySoFar) < steps) {
            continue;
        }
        if ((uint8_t)(rand32(s) >> 3) % 100 < rd8(s, team + 0xB4 + i)) {
            break;
        }
    }
    if (i > 9) {
        i = 9;
    }
    wr8(s, RAM_gCurSubstateY, i);
    wr16(s, RAM_gCurSubstateF, rd16(s, team + 0x8C + i * 2));
}

void interactionHandlerDowsing(pw_state_t *s) {
    switch (rd8(s, RAM_gCurSubstateZ)) {
    case 0:
        if (checkKeyStates(s, KEY_ENTER)) {
            if (rd8(s, RAM_gCurSubstateA + 1) == rd8(s, RAM_gCurSubstateC)) {
                playSound(s, 1);
                return;
            }
            playSound(s, 0);
            wr8(s, RAM_gCurSubstateZ, 1);
            wr8(s, RAM_gCurSubstateA, 4);
            return;
        }
        if (checkKeyStates(s, KEY_LEFT)) {
            wr8(s, RAM_gCurSubstateA + 1, (rd8(s, RAM_gCurSubstateA + 1) + 5) % 6);
            playSound(s, 2);
        }
        if (checkKeyStates(s, KEY_RIGHT)) {
            wr8(s, RAM_gCurSubstateA + 1, (rd8(s, RAM_gCurSubstateA + 1) + 1) % 6);
            playSound(s, 2);
        }
        return;
    case 1:
        if (rd8(s, RAM_gCurSubstateA) || is_F7C4_nonzero(s)) {
            return;
        }
        if (rd8(s, RAM_gCurSubstateA + 1) == rd8(s, RAM_gCurSubstateB + 1)) {
            wr8(s, RAM_gCurSubstateZ, 2);
            dowsingFindItem(s);
            playSound(s, 5);
            return;
        }
        wr8(s, RAM_gCurSubstateZ, 3);
        playSound(s, 4);
        if (rd8(s, RAM_gCurSubstateB) == 2) {
            wr8(s, RAM_gCurSubstateC, rd8(s, RAM_gCurSubstateA + 1));
        }
        wr8(s, RAM_gCurSubstateB, rd8(s, RAM_gCurSubstateB) - 1);
        return;
    case 2:
        if (!checkKeyStates(s, KEY_ANY)) {
            return;
        }
        if (!(rd8(s, RAM_RamCache_settingsByte) & 1)) {
            uint8_t slot = rd8(s, RAM_gCurSubstateE);
            if (slot == 3) {
                wr8(s, RAM_gCurSubstateA, 1);
                set_gCurSubstateZ_to_1(s);
                setCurrentlyActiveView(s, 7);
                return;
            }
            eepromWriteData(s, 0xCEBC + slot * 4, RAM_gCurSubstateF, 2);
            if (rd8(s, RAM_walker_status_flags) & 4) {
                uint16_t team = static_malloc(s, 0xBE);
                eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
                uint16_t log = static_malloc(s, 0x88);
                logEvent(s, team, log, 0xB, 0, rd16(s, RAM_gCurSubstateF), 0);
            }
        }
        break;
    case 3:
        if (!checkKeyStates(s, KEY_ANY)) {
            return;
        }
        if (rd8(s, RAM_gCurSubstateB)) {
            playSound(s, 0);
            wr8(s, RAM_gCurSubstateZ, 4);
            return;
        }
        break;
    case 4:
        if (checkKeyStates(s, KEY_ANY)) {
            playSound(s, 0);
            wr8(s, RAM_gCurSubstateZ, 0);
        }
        return;
    default: return;
    }
    playSound(s, 0);
    resetSubstates(s);
    setCurrentlyActiveView(s, 0);
}

#define ROM_BATTLE_ODDS 0xBB36

static void battle_choose_action(pw_state_t *s) {
    if (is_F7C4_nonzero(s)) {
        return;
    }
    uint8_t roll = (uint8_t)((rand32(s) >> 3) % 100);
    uint16_t odds = ROM_BATTLE_ODDS + (rd8(s, RAM_gCurSubstateF) >> 5) * 3;
    uint8_t f = rd8(s, RAM_gCurSubstateF) & 0xE7;
    if (roll < rom_rd8(odds + 2)) {
        f |= 0x10;
    } else if (roll < rom_rd8(odds + 2) + rom_rd8(odds + 1)) {
        f |= 8;
    }
    wr8(s, RAM_gCurSubstateF, f);
    uint8_t z, len;
    if (checkKeyStates(s, KEY_LEFT)) {
        f &= 0xF9;
        wr8(s, RAM_gCurSubstateF, f);
        if ((f >> 3 & 3) != 3) {
            z = 3;
            len = 8;
            goto start;
        }
    }
    if (checkKeyStates(s, KEY_RIGHT)) {
        f = (f & 0xF9) | 2;
        wr8(s, RAM_gCurSubstateF, f);
        switch (f >> 3 & 3) {
        case 2:
            playSound(s, 0xE);
            z = 7;
            len = 0xA;
            goto start;
        case 1:
            playSound(s, 0);
            z = 8;
            len = 6;
            goto start;
        case 0:
            z = 4;
            len = 8;
            goto start;
        }
    }
    if (!checkKeyStates(s, KEY_ENTER)) {
        return;
    }
    playSound(s, 0xF);
    z = 0xA;
    len = 6;
start:
    wr8(s, RAM_gCurSubstateZ, z);
    wr8(s, RAM_gCurSubstateB, 0);
    wr8(s, RAM_gCurSubstateB + 1, len);
}

#define ROM_BATTLE_APPEAR_Y  0xBB0E
#define ROM_BATTLE_ATTACK_XY 0xBB12
#define ROM_BATTLE_HIT_XY    0xBB24

static void battle_phase(pw_state_t *s, uint8_t z, uint8_t len) {
    wr8(s, RAM_gCurSubstateZ, z);
    wr8(s, RAM_gCurSubstateB, 0);
    wr8(s, RAM_gCurSubstateB + 1, len);
}

static void battle_log(pw_state_t *s, uint8_t type) {
    setHeapPointer(s);
    uint16_t team = static_malloc(s, 0xBE);
    eepromReadToRamAlso(s, 0x8F00, team, 0xBE);
    uint8_t y = rd8(s, RAM_gCurSubstateY);
    uint16_t log = static_malloc(s, 0x88);
    logEvent(s, team, log, type, y >= 4, 0, y);
}

static void battle_mood(pw_state_t *s, uint8_t mood) {
    wr8(s, RAM_gCurSubstateF, (rd8(s, RAM_gCurSubstateF) & 0x1F) | mood << 5);
}

void interactionHandlerPokeBattle(pw_state_t *s) {
    uint8_t z = rd8(s, RAM_gCurSubstateZ);
    if (z == 2) {
        battle_choose_action(s);
        return;
    }
    uint8_t frame = rd8(s, RAM_gCurSubstateB);
    uint8_t done = frame >= rd8(s, RAM_gCurSubstateB + 1);
    uint8_t f = rd8(s, RAM_gCurSubstateF);
    switch (z) {
    case 0:
        if (done)
            battle_phase(s, 1, 3);
        break;
    case 1:
        wr8(s, RAM_gCurSubstateD, rom_rd8(ROM_BATTLE_APPEAR_Y + frame));
        if (!done)
            break;
        wr8(s, RAM_gCurSubstateF, f | 1);
        if (is_F7C4_nonzero(s) || !checkKeyStates(s, KEY_ANY))
            break;
        wr8(s, RAM_gCurSubstateZ, 2);
        break;
    case 3: {
        wr8(s, RAM_gCurSubstateC, rom_rd8(ROM_BATTLE_ATTACK_XY + frame * 2));
        wr8(s, RAM_gCurSubstateD, rom_rd8(ROM_BATTLE_ATTACK_XY + 1 + frame * 2));
        if (is_F7C4_nonzero(s) || !done)
            break;
        wr8(s, RAM_gCurSubstateC, 0x38);
        wr8(s, RAM_gCurSubstateD, 8);
        uint8_t hp = rd8(s, RAM_gCurSubstateA + 1);
        switch (f >> 3 & 3) {
        case 0:
            if (hp <= 1)
                goto fled;
            wr8(s, RAM_gCurSubstateA + 1, hp - 1);
            switch (f >> 1 & 3) {
            case 0: battle_phase(s, 4, 8); break;
            case 1:
                wr8(s, RAM_gCurSubstateZ, 2);
                battle_mood(s, 1);
                break;
            }
            break;
        case 1: battle_phase(s, 4, 8); break;
        case 2:
            if (hp <= 2)
                goto fled;
            wr8(s, RAM_gCurSubstateA + 1, hp - 2);
            battle_phase(s, 4, 8);
            break;
        }
        break;
    fled:
        wr8(s, RAM_gCurSubstateA + 1, 0);
        playSound(s, 0xE);
        battle_phase(s, 7, 0xA);
        break;
    }
    case 4:
        wr8(s, RAM_gCurSubstateC, rom_rd8(ROM_BATTLE_HIT_XY + frame * 2));
        wr8(s, RAM_gCurSubstateD, rom_rd8(ROM_BATTLE_HIT_XY + 1 + frame * 2));
        if (is_F7C4_nonzero(s) || !done)
            break;
        wr8(s, RAM_gCurSubstateC, 0x38);
        wr8(s, RAM_gCurSubstateD, 8);
        switch (f >> 1 & 3) {
        case 0: {
            uint8_t hp = rd8(s, RAM_gCurSubstateA) - 1;
            wr8(s, RAM_gCurSubstateA, hp);
            if (!hp) {
                battle_phase(s, 5, 6);
                break;
            }
            switch (f >> 3 & 3) {
            case 0: battle_mood(s, 1); break;
            case 1: battle_mood(s, 3); break;
            case 2: battle_mood(s, 2); break;
            }
            wr8(s, RAM_gCurSubstateZ, 2);
            break;
        }
        case 1: battle_phase(s, 3, 8); break;
        }
        break;
    case 5: {
        if (!done)
            break;
        battle_log(s, 0x10);
        uint16_t watts = rd16(s, RAM_RamCache_curWatts);
        uint16_t lost = watts < 10 ? watts : 10;
        wr16(s, RAM_gCurSubstateE, lost);
        wr16(s, RAM_RamCache_curWatts, watts - lost);
        writeReliableDataToEeprom(s, 0x0156, 0x0256, RAM_RamCache_totalSteps, 0x18);
        battle_phase(s, 6, 8);
        break;
    }
    case 6:
        if (checkKeyStates(s, KEY_ANY)) {
            resetSubstates(s);
            setCurrentlyActiveView(s, 0);
        }
        break;
    case 7:
        wr8(s, RAM_gCurSubstateD, frame <= 3 ? rom_rd8(ROM_BATTLE_APPEAR_Y + 3 - frame) : 0xE0);
        if (!checkKeyStates(s, KEY_ANY))
            break;
        battle_log(s, 0xF);
        playSound(s, 4);
        resetSubstates(s);
        setCurrentlyActiveView(s, 0);
        break;
    case 8:
        if (!done)
            break;
        wr8(s, RAM_gCurSubstateZ, 2);
        battle_mood(s, 4);
        break;
    case 9:
        if (!done)
            break;
        playSound(s, 0xE);
        battle_phase(s, 7, 0xA);
        break;
    case 0xA:
        if (!done)
            break;
        wr8(s, RAM_gCurSubstateF, f & ~1);
        battle_phase(s, 0xB, 2);
        break;
    case 0xB:
        if (done)
            battle_phase(s, 0xC, 3);
        break;
    case 0xC:
        if (done)
            battle_phase(s, 0xD, 4);
        break;
    case 0xD:
        if (!done)
            break;
        if (rollBattleChance(s)) {
            wr8(s, RAM_gCurSubstateF + 1, rd8(s, RAM_gCurSubstateF + 1) + 1);
            battle_phase(s, 0xE, 4);
        } else {
            battle_phase(s, 0x11, 1);
        }
        break;
    case 0xE:
        if (!done)
            break;
        if (rd8(s, RAM_gCurSubstateF + 1) < 3) {
            battle_phase(s, 0xD, 4);
        } else {
            battle_phase(s, 0xF, 6);
            playSound(s, 0);
        }
        break;
    case 0xF:
        if (!done)
            break;
        battle_phase(s, 0x10, 6);
        playSound(s, 7);
        break;
    case 0x10:
        if (!done || !checkKeyStates(s, KEY_ANY))
            break;
        recordPokeCaughtNormal(s);
        if (rd8(s, RAM_currentlyActiveView) == 4) {
            resetSubstates(s);
            setCurrentlyActiveView(s, 0);
        }
        break;
    case 0x11:
        if (done)
            battle_phase(s, 9, 6);
        break;
    }
}
