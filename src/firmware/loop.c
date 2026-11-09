#include <string.h>
#include "pw_internal.h"

#define LOOP_SLEEP  0x7882
#define LOOP_NORMAL 0x7998
#define LOOP_IR     0x08D6

#define MODE_MASK  0x18
#define MODE_IDLE  0x00
#define MODE_STEPS 0x08
#define MODE_AWAKE 0x10

#define ROM_ACTIVITY_WATT_COSTS 0xBF0E

#define STACK_SCRATCH 0xFF70

static uint8_t mode(pw_state_t *s) {
    return rd8(s, RAM_walker_status_flags) & MODE_MASK;
}

static void set_mode(pw_state_t *s, uint8_t m) {
    wr8(s, RAM_walker_status_flags, (rd8(s, RAM_walker_status_flags) & ~MODE_MASK) | m);
}

static void set_bits(pw_state_t *s, uint16_t addr, uint8_t bits) {
    wr8(s, addr, rd8(s, addr) | bits);
}

static void clear_bits(pw_state_t *s, uint16_t addr, uint8_t bits) {
    wr8(s, addr, rd8(s, addr) & ~bits);
}

void rtcReadBcd(pw_state_t *s, uint16_t sec, uint16_t min, uint16_t hour) {
    uint8_t sc, mn, hr;
    hw_rtc_read(s, &sc, &mn, &hr);
    wr8(s, sec, sc);
    wr8(s, min, mn);
    wr8(s, hour, hr);
}

void sub_A180(pw_state_t *s) {
    hw_timer_b(s, 1);
    set_mode(s, MODE_STEPS);
    hw_rtc_quarter_irq(s, 1);
    wr8(s, RAM_byte_FFF7B0, 0x1E);
    clear_bits(s, RAM_common_bit_flags, 0x80);
}

void sub_A29C(pw_state_t *s) {
    wr8(s, RAM_byte_FFF7AF, 0x3C);
    wr8(s, RAM_byte_FFF7B0, 0x5A);
    if (mode(s) == MODE_AWAKE) {
        return;
    }
    if (mode(s) == MODE_IDLE) {
        wr8(s, RAM_ACCEL_SAMPLE_NEXT_WRITE_IDX, 0);
    }
    set_mode(s, MODE_AWAKE);
    hw_rtc_quarter_irq(s, 1);
    lcdExitPowerSaveMode(s);
}

void sub_A2DA(pw_state_t *s) {
    sub_3810(s);
    hw_timer_b(s, 0);
    hw_rtc_quarter_irq(s, 0);
    set_mode(s, MODE_IDLE);
}

void sub_A2F6(pw_state_t *s) {
    if (!rd8(s, RAM_byte_FFF7B0)) {
        sub_A2DA(s);
    }
}

uint8_t checkBatteryForBelowGivenLevel(pw_state_t *s, uint16_t level) {
    uint16_t cal = STACK_SCRATCH;
    readReliableDataFromEeprom(s, 0x80, 0x180, cal, 2);
    if (!adcClibrationDataVerifySelfChecksum(rd16(s, cal))) {
        wr16(s, cal, 0);
        writeReliableDataToEeprom(s, 0x80, 0x180, cal, 2);
    }

    uint8_t charge = s->hal && s->hal->battery ? s->hal->battery(s->hal->user) : 0xFF;
    return charge <= level;
}

void measureBatteryAndSetGlobalFalgWhetherItIsLow(pw_state_t *s) {
    if (!(rd8(s, RAM_common_bit_flags) & 4)) {
        return;
    }
    if (checkBatteryForBelowGivenLevel(s, 0x14)) {
        set_bits(s, RAM_common_bit_flags, 2);
    } else {
        clear_bits(s, RAM_common_bit_flags, 2);
    }
    clear_bits(s, RAM_common_bit_flags, 4);
}

void maybeConfigureKeys(pw_state_t *s) {
    wr8(s, RAM_byte_FFF798, 0);
    wr8(s, RAM_byte_FFF799, 0);
    wr8(s, RAM_CUR_KEYS, 0);
    wr8(s, RAM_byte_FFF79B, 0);
}

void likelyKeyDebounceRoutine(pw_state_t *s) {
    uint8_t b = s->hw.keys_last;
    uint8_t held = 0;
    if (b & PW_BUTTON_ENTER) {
        held |= 2;
        if (rd8(s, RAM_byte_FFF7BB)) {
            wr8(s, RAM_byte_FFF79B, rd8(s, RAM_byte_FFF79B) + 1);
        }
    } else {
        wr8(s, RAM_byte_FFF79B, 0);
    }
    if (rd8(s, RAM_common_bit_flags) & 8) {
        held |= 2;
        clear_bits(s, RAM_common_bit_flags, 8);
    }
    if (b & PW_BUTTON_LEFT) {
        held |= 4;
    }
    if (b & PW_BUTTON_RIGHT) {
        held |= 8;
    }
    wr8(s, RAM_byte_FFF798, held);
    wr8(s, RAM_CUR_KEYS, held & (held ^ rd8(s, RAM_byte_FFF799)));
    wr8(s, RAM_byte_FFF799, held);
    if (rd8(s, RAM_CUR_KEYS)) {
        wr8(s, RAM_byte_FFF7AF, 0x5A);
        wr8(s, RAM_ACCEL_SAMPLE_NEXT_WRITE_IDX, 0);
        if (mode(s) != MODE_AWAKE) {
            wr8(s, RAM_CUR_KEYS, 0);
        }
    }
    if (rd8(s, RAM_byte_FFF79B) >= 8 && mode(s) != MODE_AWAKE) {
        sub_A29C(s);
        set_bits(s, RAM_walker_status_flags, 1);
    }
}

void accelReadSample(pw_state_t *s) {
    int8_t x = 0, y = 0, z = 64;
    if (s->hal && s->hal->accel_sample) {
        s->hal->accel_sample(s->hal->user, &x, &y, &z);
    }
    uint8_t i = rd8(s, RAM_ACCEL_SAMPLE_NEXT_WRITE_IDX);
    wr8(s, RAM_ACCEL_SAMPLES_X + i, (uint8_t)x);
    wr8(s, RAM_ACCEL_SAMPLES_Y + i, (uint8_t)y);
    wr8(s, RAM_ACCEL_SAMPLES_Z + i, (uint8_t)z);
    if (rd8(s, RAM_currentlyActiveView) != 0x17) {
        return;
    }
    uint8_t prev = (i + 0x3F) & 0x3F;
    if (!i) {
        wr16(s, RAM_gCurSubstateB, 0);
        wr16(s, RAM_gCurSubstateC, 0);
        wr16(s, RAM_gCurSubstateE, 0);
    }
    static const uint16_t axes[3] = { RAM_ACCEL_SAMPLES_X, RAM_ACCEL_SAMPLES_Y, RAM_ACCEL_SAMPLES_Z };
    static const uint16_t sums[3] = { RAM_gCurSubstateB, RAM_gCurSubstateC, RAM_gCurSubstateE };
    for (int a = 0; a < 3; a++) {
        int d = (int8_t)rd8(s, axes[a] + i) - (int8_t)rd8(s, axes[a] + prev);
        wr16(s, sums[a], (uint16_t)(rd16(s, sums[a]) + (d < 0 ? -d : d)));
    }
}

uint8_t accelProcessDataDetectStep(pw_state_t *s) {
    uint8_t i = rd8(s, RAM_ACCEL_SAMPLE_NEXT_WRITE_IDX);
    uint8_t prev = (i + 0x3F) & 0x3F;
    uint16_t total = 0;
    static const uint16_t axes[3] = { RAM_ACCEL_SAMPLES_X, RAM_ACCEL_SAMPLES_Y, RAM_ACCEL_SAMPLES_Z };
    for (int a = 0; a < 3; a++) {
        int d = (int8_t)rd8(s, axes[a] + i) - (int8_t)rd8(s, axes[a] + prev);
        total = (uint16_t)(total + (d < 0 ? -d : d));
    }
    return total > 0x1E;
}

static void interactionHandlerHomeScreen(pw_state_t *s) {
    if (!(rd8(s, RAM_walker_status_flags) & 2)) {
        if (checkKeyStates(s, KEY_ENTER) || (rd8(s, RAM_common_bit_flags) & 0x20)) {
            goToConnectionMicroApp(s);
        }
        return;
    }
    if (rd8(s, RAM_gCurSubstateZ)) {
        if (checkKeyStates(s, KEY_ANY)) {
            wr8(s, RAM_gCurSubstateZ, 0);
            addRandomGift(s, rd8(s, RAM_gCurSubstateY));
            return;
        }
        wr8(s, RAM_gCurSubstateZ, rd8(s, RAM_gCurSubstateZ) - 1);
    }
    if (checkKeyStates(s, KEY_ENTER)) {
        playSound(s, 0);
        clear_gCurSubstateY(s);
        wr8(s, RAM_menuCurItem, 2);
    } else if (checkKeyStates(s, KEY_LEFT)) {
        wr8(s, RAM_menuCurItem, 5);
        playSound(s, 0);
        clear_gCurSubstateY(s);
    } else if (checkKeyStates(s, KEY_RIGHT)) {
        wr8(s, RAM_menuCurItem, 0);
        playSound(s, 0);
        clear_gCurSubstateY(s);
    } else {
        return;
    }
    setCurrentlyActiveView(s, 1);
}

static void spend_watts(pw_state_t *s, uint8_t cost) {
    uint16_t w = rd16(s, RAM_RamCache_curWatts);
    wr16(s, RAM_RamCache_curWatts, w < cost ? 0 : w - cost);
    writeReliableDataToEeprom(s, EEPROM_HEALTH0, EEPROM_HEALTH1, RAM_RamCache_totalSteps, HEALTH_SIZE);
}

static void interactionHAndlerMainMenu(pw_state_t *s) {
    if (rd8(s, RAM_gCurSubstateY)) {
        if (!checkKeyStates(s, KEY_ANY)) {
            return;
        }
        wr8(s, RAM_gCurSubstateY, 0);
        playSound(s, 0);
        return;
    }
    if (checkKeyStates(s, KEY_ENTER)) {
        uint8_t item = rd8(s, RAM_menuCurItem);
        uint8_t cost = rom_rd8(ROM_ACTIVITY_WATT_COSTS + item);
        uint8_t msg = 0;
        if (rd16(s, RAM_RamCache_curWatts) < cost) {
            msg = 1;
        } else {
            switch (item) {
            case 0:
                if (!(rd8(s, RAM_walker_status_flags) & 4)) {
                    msg = 2;
                    break;
                }
                spend_watts(s, cost);
                setCurrentlyActiveView(s, 3);
                goToPokeRadarMicroApp(s);
                playSound(s, 0);
                return;
            case 1:
                spend_watts(s, cost);
                goToDowsingMicroApp(s);
                setCurrentlyActiveView(s, 2);
                playSound(s, 0);
                return;
            case 2: goToConnectionMicroApp(s); return;
            case 3:
                setCurrentlyActiveView(s, 8);
                clearSubstatesYZ(s);
                playSound(s, 0);
                return;
            case 4: {
                uint16_t inv = STACK_SCRATCH;
                takeItemAndPokeInventory(s, inv);
                if (rd16(s, inv)) {
                    wr16(s, RAM_gCurSubstateA, rd16(s, inv));
                    wr16(s, RAM_gCurSubstateB, rd16(s, inv + 2));
                    cursorToFirstSlotOfA(s);
                    setCurrentlyActiveView(s, 0xA);
                } else if (rd16(s, inv + 2)) {
                    wr16(s, RAM_gCurSubstateA, rd16(s, inv));
                    wr16(s, RAM_gCurSubstateB, rd16(s, inv + 2));
                    cursorToFirstSlotOfB(s);
                    setCurrentlyActiveView(s, 0xB);
                } else {
                    msg = 3;
                    break;
                }
                playSound(s, 0);
                return;
            }
            case 5:
                clearSubstatesYZ(s);
                setCurrentlyActiveView(s, 9);
                playSound(s, 0);
                return;
            default: break;
            }
        }
        if (msg) {
            wr8(s, RAM_gCurSubstateY, msg);
            playSound(s, 4);
            return;
        }
    }
    if (checkKeyStates(s, KEY_LEFT)) {
        if (!rd8(s, RAM_menuCurItem)) {
            goto home;
        }
        wr8(s, RAM_menuCurItem, (rd8(s, RAM_menuCurItem) + 5) % 6);
        playSound(s, 2);
    }
    if (!checkKeyStates(s, KEY_RIGHT)) {
        return;
    }
    if (rd8(s, RAM_menuCurItem) == 5) {
        goto home;
    }
    wr8(s, RAM_menuCurItem, (rd8(s, RAM_menuCurItem) + 1) % 6);
    playSound(s, 2);
    return;
home:
    resetSubstates(s);
    setCurrentlyActiveView(s, 0);
    playSound(s, 1);
}

void interactionHandler(pw_state_t *s) {
    rand32(s);
    switch (rd8(s, RAM_currentlyActiveView)) {
    case 0x00: interactionHandlerHomeScreen(s); break;
    case 0x01: interactionHAndlerMainMenu(s); break;
    case 0x02: interactionHandlerDowsing(s); break;
    case 0x03: interactionHandlerPokeRadar(s); break;
    case 0x04: interactionHandlerPokeBattle(s); break;
    case 0x06: interactionHandlerPokeRadarFailureUserTooSlow(s); break;
    case 0x07: interactionHandlerSwitchScreen(s); break;
    case 0x08: interactionHandlerTrainerCardAndStatsScreens(s); break;
    case 0x09: interactionHandlerSettings(s); break;
    case 0x0A: interactionHandlerPokemonAndItemsScreen(s); break;
    case 0x0B: interactionHandlerGiftedItemsScreen(s); break;
    case 0x0C: interactionHandlerRandomGiftFromBoredPokemon(s); break;
    case 0x0D: interactionHandlerNone(s); break;
    case 0x0E:

        if ((rd8(s, RAM_walker_status_flags) & 2) && checkKeyStates(s, KEY_ANY)) {
            resetSubstates(s);
            setCurrentlyActiveView(s, 0);
            playSound(s, 0);
        }
        break;
    case 0x0F: interactionHandlerPokemonArrivedAnimation(s); break;
    case 0x10: interactionHandlerWalkEnd(s); break;
    case 0x11: interactionHandlerSomethingReceived(s); break;
    case 0x16: interactionHandlerDebugMode22(s); break;
    case 0x17: interactionHandlerNone(s); break;
    default: break;
    }
    wr8(s, RAM_byte_FFF7AB, rd8(s, RAM_byte_FFF7AB) + 1);
}

void maybe_viewDoDisplay(pw_state_t *s) {
    uint8_t inited = rd8(s, RAM_walker_status_flags) & 2;
    switch (rd8(s, RAM_currentlyActiveView)) {
    case 0x00:
        if (!inited) {
            drawPokewalkerInitialConnectionInstructionScreen(s);
        } else {
            drawHomeScreenMain(s);
            drawHomeScreenSmallIconsOnBottom(s);
        }
        break;
    case 0x01: mainMenuDraw(s); break;
    case 0x02: dowsingDraw(s); break;
    case 0x03: pokeRadarDraw(s); break;
    case 0x04: battleScreenDraw(s); break;
    case 0x06: pokeradarUserTooSlowScreen(s); break;
    case 0x07: drawSwitchScreen(s); break;
    case 0x08: drawTrainerCardAndStatsScreens(s); break;
    case 0x09: drawSettingsScreen(s); break;
    case 0x0A: drawPokemonAndItemsScreen(s); break;
    case 0x0B: giftedItemsView(s); break;
    case 0x0C: showPokemonFeelingsAndRandomFindings(s); break;
    case 0x0D: drawPeerPlay(s); break;
    case 0x0E:
        if (!inited) {
            drawSadPokewalkerImage(s);
        } else {
            showCommsErrorMessage(s);
        }
        break;
    case 0x0F: drawPokemonArrivedAnimation(s); break;
    case 0x10: drawSomethingDepartingAndCompletedMessage(s); break;
    case 0x11: drawSomethingReceivedAnimation(s); break;
    case 0x16: drawSomethingWithTinyChars(s); break;
    case 0x17: drawSomethingWithTinyChars2(s); break;
    case 0x18: drawNG6inTinyChars(s); break;
    default: break;
    }
}

static void redraw(pw_state_t *s) {
    lcdClearBackBuffer(s, 0x40);
    maybe_viewDoDisplay(s);
    lcdFlipBuffers(s);
    wr8(s, RAM_curUiFlags, rd8(s, RAM_curUiFlags) + 1);
}

static void configureSciAndSendPingByte(pw_state_t *s) {
    wr8(s, RAM_commsErrorMessageId, 0);
    wr32(s, RAM_SuggestedOurSideSessionId, rd32(s, RAM_RandomSeed));
    wr32(s, RAM_CurrentSessionID, rd32(s, RAM_SuggestedOurSideSessionId));
    wr8(s, RAM_ourPeerRole, 1);
    wr8(s, RAM_numTimesWeSentIrAdv, 0);
    wr8(s, RAM_NumPingsSent, 0);
    wr8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE, 0xFF);
    clear_bits(s, RAM_bool_got_at_least_one_packet, 1);
    wr8(s, RAM_NumBytesRxedsoFar, 0);
    wr8(s, RAM_byte_FFF8C1, 0);
    wr8(s, RAM_PeerPlayStage, 0);
    hw_ir_start(s);
    wr16(s, RAM_TimeWhenLAstByteRxed, hw_ir_tcnt(s));
    irTxByte(s, 0xFC);
}

void goToConnectionMicroApp(pw_state_t *s) {
    if (rd16(s, RAM_currentEventLoopFunc) != LOOP_SLEEP) {
        return;
    }
    for (uint8_t frame = 0; frame < 2; frame++) {
        if (frame) {
            lcdFlipBuffers(s);
        }
        lcdClearBackBuffer(s, 0x40);
        if (!(rd8(s, RAM_walker_status_flags) & 2)) {
            drawHappyPokewalker(s, frame);
        } else {
            drawConnectingScreen(s, frame);
            drawLowBatteryIcon(s, 0, 0);
        }
    }
    lcdFlipBuffers(s);
    hw_mask_interrupts(s, 1);
    configureSciAndSendPingByte(s);
    setEventLoopFunc(s, LOOP_IR);
}

void performActionAsRequestedByRemote(pw_state_t *s) {
    uint8_t z_and_home = 0;
    uint8_t a = 0xFF;
    switch (rd8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE)) {
    case 0xF0:
        lcdInit(s);
        wr8(s, RAM_currentlyActiveView, 0x16);
        applyDefaultSettings(s);
        break;
    case 0xFE:
        wr8(s, RAM_currentlyActiveView, 0x17);
        enterDebugView22(s);
        break;
    case 0xE0:
    case 0x2A:
    case 0x2C: {
        uint8_t act = rd8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE);
        if (act == 0xE0) {
            lcdInit(s);
        }
        wr16(s, RAM_word_FFF7A2, 0xE10);
        some_sort_of_eeprom_init(s, act == 0xE0, act != 0x2C);
        uint8_t set = rd8(s, RAM_RamCache_settingsByte);
        setVolume(s, set >> 1 & 3);
        lcdSetContrast(s, set >> 3 & 0xF);
        z_and_home = 2;
        break;
    }
    case 0x38:
        wr32(s, RAM_RamCache_STEP_COUNT_maybe, 0);
        startWalkEepromActions(s);
        setCurrentlyActiveView(s, 0xF);
        wr8(s, RAM_gCurSubstateY, 0);
        z_and_home = 1;
        break;
    case 0x4E:
        end_walk_action(s);
        setCurrentlyActiveView(s, 0x10);
        wr8(s, RAM_gCurSubstateY, 5);
        z_and_home = 1;
        break;
    case 0x5A:
        startWalkEepromActions(s);

        eepromFillWithByte(s, 0xB800, 0x6C8, 0);
        setCurrentlyActiveView(s, 0xF);
        wr8(s, RAM_gCurSubstateY, 0);
        z_and_home = 1;
        break;
    case 0x66:
        clearWattsAndErasePokemon(s);
        setCurrentlyActiveView(s, 0x10);
        wr8(s, RAM_gCurSubstateY, 6);
        z_and_home = 1;
        break;
    case 0x16:
        setCurrentlyActiveView(s, 0xD);
        goToPeerPlayMicroApp(s);
        break;
    case 0xC2:
        setCurrentlyActiveView(s, 0x11);
        wr8(s, RAM_gCurSubstateY, 0);
        wr8(s, RAM_gCurSubstateZ, 0);
        wr8(s, RAM_gCurSubstateA, 0);
        break;
    case 0xC0: a = 3; break;
    case 0xC4: a = 2; break;
    case 0xC6: a = 1; break;
    case 0xB8: a = 4; break;
    case 0xBA: a = 5; break;
    case 0xBC: a = 6; break;
    case 0xBE: a = 7; break;
    default:
        if (rd8(s, RAM_commsErrorMessageId)) {
            setCurrentlyActiveView(s, 0xE);
            z_and_home = 1;
        } else {
            z_and_home = 2;
        }
        break;
    }
    if (a != 0xFF) {
        setCurrentlyActiveView(s, 0x11);
        wr8(s, RAM_gCurSubstateY, 0);
        wr8(s, RAM_gCurSubstateZ, 0);
        wr8(s, RAM_gCurSubstateA, a);
    }
    if (z_and_home == 1) {
        wr8(s, RAM_gCurSubstateZ, 0);
    } else if (z_and_home == 2) {
        resetSubstates(s);
        setCurrentlyActiveView(s, 0);
    }
    wr8(s, RAM_ACCEL_SAMPLE_NEXT_WRITE_IDX, 0);
    clear_bytes_at_F8EE_F8EF_words_at_F8EA_F8E6(s);
    setEventLoopFunc(s, LOOP_SLEEP);
    hw_mask_interrupts(s, 0);
    rtcReadBcd(s, RAM_currentSecond, RAM_currentMinute, RAM_currentHour);
}

void maybe_shutDownIr_then_performActionAsRequested(pw_state_t *s) {
    hw_ir_stop(s);
    performActionAsRequestedByRemote(s);
}

#define IR_TIMEOUT 3200

static uint32_t ir_ticks_to_next(pw_state_t *s) {
    if (s->hal && s->hal->ir_recv) {
        return 1;
    }
    uint16_t since = (uint16_t)(hw_ir_tcnt(s) - rd16(s, RAM_TimeWhenLAstByteRxed));
    return since > IR_TIMEOUT ? 0 : IR_TIMEOUT + 1 - since;
}

static void start_sound_if_needed(pw_state_t *s) {
    if (is_F7C4_nonzero(s)) {
        setEventLoopFunc(s, LOOP_NORMAL);
        sub_37D6(s);
    }
}

static void sleepModeEventLoop(pw_state_t *s) {
    accelReadSample(s);
    likelyKeyDebounceRoutine(s);
    if (mode(s) == MODE_IDLE) {
        sub_A34A(s);
        if (accelProcessDataDetectStep(s) || (rd8(s, RAM_common_bit_flags) & 8)) {
            sub_A180(s);
        }
        goto next_sample;
    }
    if (mode(s) == MODE_AWAKE) {
        chooseWalkEvent(s);
        interactionHandler(s);
        if (rd16(s, RAM_currentEventLoopFunc) == LOOP_IR) {
            goto next_sample;
        }
    }
    if ((rd8(s, RAM_walker_status_flags) & 2) && rd8(s, RAM_ACCEL_SAMPLE_NEXT_WRITE_IDX) == 0x3F) {
        processAccelSamples(s);
    } else if (rd8(s, RAM_common_bit_flags) & 1) {
        if (mode(s) == MODE_AWAKE) {
            redraw(s);
        }
        clear_bits(s, RAM_common_bit_flags, 1);
    } else {
        sub_A34A(s);
        if (mode(s) == MODE_AWAKE) {
            if (!rd8(s, RAM_byte_FFF7AF)) {
                lcdEnterPowerSaveMode(s);
                set_mode(s, MODE_STEPS);
                wr8(s, RAM_byte_FFF7BB, 0);
                wr8(s, RAM_byte_FFF79B, 0);
            }
        } else {
            measureBatteryAndSetGlobalFalgWhetherItIsLow(s);
            sub_A2F6(s);
        }
    }
    accountForStepTaken(s);
    start_sound_if_needed(s);
next_sample:
    wr8(s, RAM_ACCEL_SAMPLE_NEXT_WRITE_IDX, (rd8(s, RAM_ACCEL_SAMPLE_NEXT_WRITE_IDX) + 1) & 0x3F);
}

static void normalModeEventLoop(pw_state_t *s) {
    likelyKeyDebounceRoutine(s);
    interactionHandler(s);
    if (rd8(s, RAM_common_bit_flags) & 1) {
        if (mode(s) == MODE_AWAKE) {
            redraw(s);
        }
        clear_bits(s, RAM_common_bit_flags, 1);
    }
    if (!is_F7C4_nonzero(s)) {
        sub_3810(s);
        setEventLoopFunc(s, LOOP_SLEEP);
        wr8(s, RAM_ACCEL_SAMPLE_NEXT_WRITE_IDX, 0);
    }
}

void checkForAndInitEepromContentsIfNeeded(pw_state_t *s) {
    if (checkForNintendoSignature(s)) {
        readReliableDataFromEeprom(s, EEPROM_HEALTH0, EEPROM_HEALTH1, RAM_RamCache_totalSteps, HEALTH_SIZE);
        uint8_t set = rd8(s, RAM_RamCache_settingsByte);
        if ((set & 0x78) > 0x48) {
            wr8(s, RAM_RamCache_settingsByte, (set & 0x87) | 0x20);
        }
        updateCachedWalkerStatus(s);
    } else {
        some_sort_of_eeprom_init(s, 1, 1);
        uint8_t set = rd8(s, RAM_RamCache_settingsByte);
        setVolume(s, set >> 1 & 3);
        lcdSetContrast(s, set >> 3 & 0xF);
        writeNintendoSignature(s);
    }
    uint16_t flag = STACK_SCRATCH;
    readReliableDataFromEeprom(s, 0x16F, 0x26F, flag, 1);
    if (rd8(s, flag) == 0xA5) {
        copyTeamAndconfigDataFromStagingArea(s);
        wr8(s, flag, 0);
        writeReliableDataToEeprom(s, 0x16F, 0x26F, flag, 1);
    }
}

static void boot(pw_state_t *s) {
    wr8(s, RAM_common_bit_flags, rd8(s, RAM_common_bit_flags) | 0x10);
    set_mode(s, MODE_AWAKE);
    wr8(s, RAM_byte_FFF7AF, 0x3C);
    wr8(s, RAM_byte_FFF7B0, 0x5A);
    wr16(s, RAM_word_FFF7A2, 0xE10);
    clear_bytes_at_F8EE_F8EF_words_at_F8EA_F8E6(s);
    checkForAndInitEepromContentsIfNeeded(s);
    configureSomeIoAndTimerW(s);
    setVolume(s, rd8(s, RAM_RamCache_settingsByte) >> 1 & 3);
    lcdInit(s);

    rtcSet(s, rd32(s, RAM_RamCache_lastSyncTime));
    uint16_t seed = STACK_SCRATCH;
    eepromReadToRamAlso(s, 0x153, seed, 4);
    setRandomSeed(s, rd32(s, seed));
    maybeConfigureKeys(s);
    setEventLoopFunc(s, LOOP_SLEEP);
    resetSubstates(s);
    wr8(s, RAM_currentlyActiveView, 0);
    set_timer_b_and_enable_irq(s);
}

void pw_init(pw_state_t *s, const pw_hal_t *hal) {
    memset(s, 0, sizeof(*s));
    pw_set_hal(s, hal);
    hw_reset(s);
    boot(s);
}

void pw_update(pw_state_t *s, uint32_t ms) {
    uint64_t target = s->hw.now + hw_ms_to_ticks(ms);
    hw_buttons(s);
    for (;;) {
        uint16_t loop = rd16(s, RAM_currentEventLoopFunc);
        if (loop == LOOP_IR) {
            uint64_t due = s->hw.now + ir_ticks_to_next(s);
            if (due <= target) {
                while (hw_wait(s, due, 0)) {
                }
                irCommsEventLoop(s);
                continue;
            }
            while (hw_wait(s, target, 0)) {
            }
            break;
        }
        if (!hw_wait(s, target, loop == LOOP_SLEEP)) {
            break;
        }
        if (loop == LOOP_NORMAL) {
            normalModeEventLoop(s);
        } else {
            sleepModeEventLoop(s);
        }
    }
    if (s->hal && s->hal->present) {
        uint8_t pixels[PW_SCREEN_HEIGHT][PW_SCREEN_WIDTH];
        pw_render(s, pixels);
        s->hal->present(s->hal->user, pixels);
    }
}
