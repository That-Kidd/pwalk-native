#pragma once
#include <stdint.h>
#include "pokewalker.h"
#include "ram_map.h"
#include "rom_data.h"

static inline uint8_t *ram_ptr(pw_state_t *s, uint16_t addr) {
    static uint8_t outside;
    uint16_t off = (uint16_t)(addr - PW_RAM_START);
    if (off >= PW_RAM_SIZE) {
        outside = 0xFF;
        return &outside;
    }
    return &s->ram[off];
}

static inline uint8_t rd8(pw_state_t *s, uint16_t addr) {
    return *ram_ptr(s, addr);
}

static inline uint16_t rd16(pw_state_t *s, uint16_t addr) {
    return (uint16_t)(rd8(s, addr) << 8 | rd8(s, addr + 1));
}

static inline uint32_t rd32(pw_state_t *s, uint16_t addr) {
    return (uint32_t)rd16(s, addr) << 16 | rd16(s, addr + 2);
}

static inline void wr8(pw_state_t *s, uint16_t addr, uint8_t v) {
    *ram_ptr(s, addr) = v;
}

static inline void wr16(pw_state_t *s, uint16_t addr, uint16_t v) {
    wr8(s, addr, v >> 8);
    wr8(s, addr + 1, v);
}

static inline void wr32(pw_state_t *s, uint16_t addr, uint32_t v) {
    wr16(s, addr, v >> 16);
    wr16(s, addr + 2, v);
}

static inline uint8_t shl8(uint8_t v, int n) {
    return n >= 8 ? 0 : (uint8_t)(v << n);
}

static inline uint8_t shr8(uint8_t v, int n) {
    return n >= 8 ? 0 : (uint8_t)(v >> n);
}

static inline uint8_t mem_rd8(pw_state_t *s, uint16_t addr) {
    return addr < 0xC000 ? rom_rd8(addr) : rd8(s, addr);
}

#define EEPROM_HEALTH0 0x0156
#define EEPROM_HEALTH1 0x0256
#define HEALTH_SIZE    0x18

void hw_reset(pw_state_t *s);
void hw_resync_beeper(pw_state_t *s);
void hw_rtc_set(pw_state_t *s, uint8_t sec, uint8_t min, uint8_t hour);
void hw_rtc_quarter_irq(pw_state_t *s, int on);
void hw_rtc_read(pw_state_t *s, uint8_t *sec, uint8_t *min, uint8_t *hour);
void hw_timer_b(pw_state_t *s, int on);
void hw_ir_start(pw_state_t *s);
void hw_ir_stop(pw_state_t *s);
uint16_t hw_ir_tcnt(pw_state_t *s);
void hw_mask_interrupts(pw_state_t *s, int masked);
int hw_wait(pw_state_t *s, uint64_t limit, int timer_b_wakes);
uint8_t hw_buttons(pw_state_t *s);
uint64_t hw_ms_to_ticks(uint32_t ms);
void set_timer_b_and_enable_irq(pw_state_t *s);
void sub_37D6(pw_state_t *s);
void sub_3810(pw_state_t *s);
void configureSomeIoAndTimerW(pw_state_t *s);
void setBeeperFreq(pw_state_t *s, uint8_t period);
void timer_w_irq(pw_state_t *s);

void rtcReadBcd(pw_state_t *s, uint16_t sec, uint16_t min, uint16_t hour);
void sub_A180(pw_state_t *s);
void sub_A29C(pw_state_t *s);
void sub_A2DA(pw_state_t *s);
void sub_A2F6(pw_state_t *s);
void maybeConfigureKeys(pw_state_t *s);
void likelyKeyDebounceRoutine(pw_state_t *s);
void accelReadSample(pw_state_t *s);
uint8_t accelProcessDataDetectStep(pw_state_t *s);
void interactionHandler(pw_state_t *s);
void maybe_viewDoDisplay(pw_state_t *s);
void goToConnectionMicroApp(pw_state_t *s);
void performActionAsRequestedByRemote(pw_state_t *s);
void maybe_shutDownIr_then_performActionAsRequested(pw_state_t *s);
void checkForAndInitEepromContentsIfNeeded(pw_state_t *s);
uint8_t checkBatteryForBelowGivenLevel(pw_state_t *s, uint16_t level);
void measureBatteryAndSetGlobalFalgWhetherItIsLow(pw_state_t *s);

uint8_t eepromFactoryTest(pw_state_t *s, uint16_t start);
uint8_t adcClibrationDataVerifySelfChecksum(uint16_t word);
void interactionHandlerDebugMode22(pw_state_t *s);
void drawSomethingWithTinyChars(pw_state_t *s);
void hex4(char out[5], uint16_t v);

uint16_t calcPacketCrc(pw_state_t *s, uint16_t data, uint32_t len);
void decompress(pw_state_t *s, uint16_t src, uint16_t dst);
uint8_t seeIfWeSawThisTrainerBefore(pw_state_t *s, uint16_t id);
void processSyncPacketFromGame(pw_state_t *s);
void irTxByte(pw_state_t *s, uint8_t byte);
void sendPacket(pw_state_t *s, uint8_t len, uint8_t cmd, uint8_t extra);
void irCommsEventLoop(pw_state_t *s);

uint32_t mul32(uint32_t a, uint32_t b);
uint32_t divmod32(uint32_t num, uint32_t den, uint32_t *rem);
void setRandomSeed(pw_state_t *s, uint32_t seed);
uint32_t rand32(pw_state_t *s);
void clampTotalStepToLimit(pw_state_t *s, uint32_t steps);

void eepromReadToRamAlso(pw_state_t *s, uint16_t eeprom_addr, uint16_t ram_dst, uint16_t len);
uint8_t eepromReadSingleByte(pw_state_t *s, uint16_t eeprom_addr);
void eepromWriteData(pw_state_t *s, uint16_t eeprom_addr, uint16_t ram_src, uint16_t len);
uint8_t eepromWriteSingleByte(pw_state_t *s, uint16_t eeprom_addr, uint8_t value);
void eepromFillWithByte(pw_state_t *s, uint16_t eeprom_addr, uint16_t len, uint8_t value);
void eepromFillPageWithByte(pw_state_t *s, uint16_t eeprom_addr, uint8_t value);
void eepromWrite128Bytes(pw_state_t *s, uint16_t eeprom_addr, uint16_t ram_src);
uint8_t readReliableDataFromEeprom(pw_state_t *s, uint16_t addr0, uint16_t addr1, uint16_t ram_addr, uint16_t len);
void writeReliableDataToEeprom(pw_state_t *s, uint16_t addr0, uint16_t addr1, uint16_t ram_addr, uint16_t len);
void setHeapPointer(pw_state_t *s);
uint16_t static_malloc(pw_state_t *s, uint16_t size);

void lcdc_command(pw_state_t *s, uint8_t byte);
void lcdc_data(pw_state_t *s, uint8_t byte);
void lcdc_reset(pw_state_t *s);

void lcd_set_address(pw_state_t *s, uint8_t x, uint8_t stripe);
void lcdSendSingleByteCommand(pw_state_t *s, uint8_t cmd);
void lcdClearBackBuffer(pw_state_t *s, uint8_t height);
void lcdFlipBuffers(pw_state_t *s);
void drawImageToScreen(pw_state_t *s, uint8_t x, int8_t y, uint16_t img, uint8_t w, uint8_t h);
void lcdSetAddress(pw_state_t *s, uint8_t x, uint8_t stripe);
void lcdShowBuffer(pw_state_t *s, uint8_t buffer);
void lcdFillBackBuffer(pw_state_t *s, uint8_t shade);
void drawBoxOutlineRows32To47(pw_state_t *s);
void drawBoxOutlineRows48To63(pw_state_t *s);
void lcdSetContrast(pw_state_t *s, uint8_t contrast);
void lcdExitPowerSaveMode(pw_state_t *s);
void lcdEnterPowerSaveMode(pw_state_t *s);
void lcdInit(pw_state_t *s);
void drawImageClipped(pw_state_t *s, int8_t x, int8_t y, uint16_t w, uint16_t h, uint16_t img);
void lcdFillRectangle(pw_state_t *s, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t shade);
void drawImageToBuffer(pw_state_t *s, uint8_t w, uint8_t h, uint16_t img, uint8_t x, uint8_t y, uint16_t buf,
                       uint8_t buf_w);

void drawTinyChars(pw_state_t *s, uint8_t x, uint8_t y, uint16_t str);
void drawNumberOnscreen(pw_state_t *s, uint8_t x, int8_t y, uint32_t number, uint8_t overline);
void drawNumberAndWattsymbolOnscreen(pw_state_t *s, uint8_t x, int8_t y, uint16_t watts);
void drawLargeMessage(pw_state_t *s, uint8_t y, uint8_t message, uint8_t borders, uint8_t cursor);

void imageFlipHorizontal(pw_state_t *s, uint8_t w, uint8_t h, uint16_t img);
void moveImageVerticaly(pw_state_t *s, uint8_t w, uint8_t h, uint16_t img, int8_t by);
void textImageAddBorders(pw_state_t *s, uint8_t w, uint8_t h, uint16_t img, uint8_t flags);
void imageOverlayMasked(pw_state_t *s, uint16_t dst, uint16_t sprite, uint8_t dst_w, uint8_t dst_h, uint16_t mask,
                        uint8_t x, uint8_t y, uint8_t w);

#define ROM_EEPROM_HEADER 0xBF98

void interactionHandlerNone(pw_state_t *s);
uint16_t get_packet_data_pointer(pw_state_t *s);
void setVolume(pw_state_t *s, uint8_t volume);
void setCurrentlyActiveView(pw_state_t *s, uint8_t view);
void set_gCurSubstateZ_to_1(pw_state_t *s);
void clear_gCurSubstateY(pw_state_t *s);
void clearSubstatesYZ(pw_state_t *s);
void resetSubstates(pw_state_t *s);
void setEventLoopFunc(pw_state_t *s, uint16_t loop);
uint8_t is_F7C4_nonzero(pw_state_t *s);
void write_F7C4_clear_F7C8_and_F7CA(pw_state_t *s, uint16_t value);
void incrementSaturating_F790(pw_state_t *s);
void clear_bytes_at_F8EE_F8EF_words_at_F8EA_F8E6(pw_state_t *s);
void lslByteAtPtr(pw_state_t *s, uint16_t ptr, uint8_t by);
void lsrByteAtPtr(pw_state_t *s, uint16_t ptr, uint8_t by);
uint16_t orr_0x0101_into_three_words(pw_state_t *s, uint16_t ptr);
uint8_t findFreeItemSlot(pw_state_t *s, uint16_t slots);
uint8_t findFirstEmptyCaughtSlot(pw_state_t *s, uint16_t slots);
void eraseCaughtPokemonAndFoundItems(pw_state_t *s);
void fill_eeprom_0xCEF0_to_CF0B_with_zero(pw_state_t *s);
void clearEventLogEntries(pw_state_t *s);
void readSpecialItemInfo(pw_state_t *s, uint16_t ram);
void writeSpecialItemInfo(pw_state_t *s, uint16_t ram);
void writeNintendoSignature(pw_state_t *s);
void drawNG6inTinyChars(pw_state_t *s);
void shortDelay(pw_state_t *s);
uint8_t checkKeyStates(pw_state_t *s, uint8_t mask);
void addWatts(pw_state_t *s, uint8_t watts);
void clearWattsAndErasePokemon(pw_state_t *s);
void applyDefaultSettings(pw_state_t *s);
uint8_t checkForNintendoSignature(pw_state_t *s);
void bitfieldInsert(pw_state_t *s, uint16_t ptr, uint8_t value, uint8_t width, uint8_t offset);
uint16_t getItemIdForNthItemOnRoute(pw_state_t *s, uint8_t n);
uint8_t rollBattleChance(pw_state_t *s);
void updateCachedWalkerStatus(pw_state_t *s);
void pushSeenTrainersLogDown(pw_state_t *s);
void goToDowsingMicroApp(pw_state_t *s);

void drawItemSymbolOnScreen(pw_state_t *s, uint8_t x, int8_t y);
void drawLargeItemChest(pw_state_t *s, uint8_t x, int8_t y);
void drawLargePresentItem(pw_state_t *s, uint8_t x, int8_t y);
void drawPokemonFeelingBubble(pw_state_t *s, uint8_t feeling);
void drawCurPokemonSmallAnimatedFrame(pw_state_t *s, uint8_t x, int8_t y);
void drawCurrentAreaSmallImage(pw_state_t *s);
void drawCurPokemonSmallAnimatedFrameFlipped(pw_state_t *s, uint8_t x, int8_t y);
void drawMusicNote(pw_state_t *s, uint8_t x, int8_t y, uint8_t shift);
void drawRewardReceivedBanner(pw_state_t *s);
void showRxedItemName(pw_state_t *s, uint8_t x, int8_t y, uint8_t borders);
void drawPeerPokemonName(pw_state_t *s, uint8_t x, int8_t y, uint8_t borders);
void showEventPokeName(pw_state_t *s, uint8_t x, int8_t y, uint8_t borders);
void enterDebugView22(pw_state_t *s);
uint8_t cursorToPrevOwnedSlot(pw_state_t *s, uint16_t owned);
void drawPeerPlayPokemonFrame(pw_state_t *s, int8_t x, int8_t y, uint8_t flip);
void drawLowBatteryIcon(pw_state_t *s, uint8_t x, int8_t y);
void drawRouteAvailablePokemon(pw_state_t *s, uint8_t x, int8_t y, uint8_t n);
void drawNameOfEventPokemon(pw_state_t *s, uint8_t x, int8_t y, uint8_t borders);
uint8_t hasParticipatedInEvent(pw_state_t *s, uint16_t buf, uint8_t idx);
void setThatWeveParticipatedInAGivenEvent(pw_state_t *s, uint16_t buf, uint8_t idx);
void homeScreenSlidePokemon(pw_state_t *s);
void homeScreenPacePokemon(pw_state_t *s);
void homeScreenDrawLargeCurrentPokemonImage(pw_state_t *s, uint8_t x, int8_t y);
void drawCloudWithBarsOnTopAndBottomOfScreen(pw_state_t *s);
void drawCloudWithBarsOnTopAndBottomOfScreen2(pw_state_t *s);
void drawPokewalkerAndCompletedMessage(pw_state_t *s);
void copyTeamAndconfigDataFromStagingArea(pw_state_t *s);
void drawLargePokemonImageWithBars(pw_state_t *s);
void pokemonDepartureScreen(pw_state_t *s);
void pokemonArrivalScreen(pw_state_t *s);
void pokeradarUserTooSlowScreen(pw_state_t *s);
void memoryCopy(pw_state_t *s, uint16_t src, uint16_t dst, uint16_t len);
void drawCurSelectedPokeNameForSwitchScreen(pw_state_t *s);
void drawCurSelectedItemNameForSwitchScreen(pw_state_t *s);
void drawMovingBlankIconWithBarsOnTopAndBottom(pw_state_t *s);
void drawBallDropFromTopOfScreen(pw_state_t *s);
void drawPokeballInMiddleOfScreen(pw_state_t *s);
void drawHomeScreenMain(pw_state_t *s);
void drawSadPokewalkerImage(pw_state_t *s);
void drawHappyPokewalker(pw_state_t *s, uint8_t ir_icon);
void drawConnectingScreen(pw_state_t *s, uint8_t ir_icon);
void showGoodJobBanner(pw_state_t *s);
void end_walk_action(pw_state_t *s);
void showCommsErrorMessage(pw_state_t *s);
void drawLineAtRow48(pw_state_t *s);
uint8_t cursorToNextOwnedSlot(pw_state_t *s, uint16_t owned);
void drawPokemonArrivedAnimation(pw_state_t *s);
void drawSwitchScreen(pw_state_t *s);
void cursorToNextOwnedSlotWrapping(pw_state_t *s, uint16_t owned);
void cursorToFirstSlotOfA(pw_state_t *s);
void cursorToFirstSlotOfB(pw_state_t *s);
void accountForStepTaken(pw_state_t *s);
void dayEnded(pw_state_t *s);
void rtcSet(pw_state_t *s, uint32_t seconds);
void showPokemonFeelingsAndRandomFindings(pw_state_t *s);
void pokemonJoinsEmptyWalkerWalk(pw_state_t *s);
void chooseWalkEvent(pw_state_t *s);
void drawPokewalkerInitialConnectionInstructionScreen(pw_state_t *s);
void takeItemAndPokeInventory(pw_state_t *s, uint16_t flags);
uint32_t spectrumPeakCentroid(pw_state_t *s, uint16_t mags, uint8_t idx);
void processAccelerometerSamplesAxis(pw_state_t *s, uint16_t samples);
uint32_t findWalkingPeak(pw_state_t *s, uint16_t spec);
void processAccelSamples(pw_state_t *s);
void mainMenuDraw(pw_state_t *s);
void drawSettingsScreen(pw_state_t *s);
void drawStepStatisticsScreen(pw_state_t *s);
void pokeRadarDraw(pw_state_t *s);
void giftedItemsView(pw_state_t *s);
void drawPokemonAndItemsScreen(pw_state_t *s);
void logEvent(pw_state_t *s, uint16_t team, uint16_t buf, uint8_t type, uint8_t special, uint16_t extra, uint8_t poke);
void drawCurPokemonNameWithBorders(pw_state_t *s, uint8_t x, int8_t y, uint8_t borders);
void drawNameOfRouteAvailablePokemon(pw_state_t *s, uint8_t x, int8_t y, uint8_t idx, uint8_t borders);
void drawRouteAvailableItemName(pw_state_t *s, uint8_t x, int8_t y, uint8_t idx, uint8_t borders);
uint8_t cursorToPrevOwnedSlotWrapping(pw_state_t *s, uint16_t owned);
void wipeOutStepsAndSyncTime(pw_state_t *s, uint8_t full);
void recordPokeCaughtNoEventConsideration(pw_state_t *s);
void recordItemFoundNoEventConsideration(pw_state_t *s);
void drawSomethingDepartingAndCompletedMessage(pw_state_t *s);
void drawItemMapStampOrRouteReceived(pw_state_t *s);
void dowsingScreen(pw_state_t *s);
void sub_A3AA(pw_state_t *s);
void addRandomGift(pw_state_t *s, uint8_t type);
void calculateAndApplyPeerGift(pw_state_t *s);
void recordPokeCaughtNormal(pw_state_t *s);
void maybeFoundSomeEventItem(pw_state_t *s);
void startWalkEepromActions(pw_state_t *s);
void logPeerPlay(pw_state_t *s);
void drawSomethingReceivedAnimation(pw_state_t *s);
void goToPeerPlayMicroApp(pw_state_t *s);
void sub_A34A(pw_state_t *s);
void drawHomeScreenSmallIconsOnBottom(pw_state_t *s);
void dowsingDraw(pw_state_t *s);
void some_sort_of_eeprom_init(pw_state_t *s, uint8_t forget_trainer, uint8_t full);
void draw_tiny_string(pw_state_t *s, uint8_t x, uint8_t y, const char *str);
void drawSomethingWithTinyChars2(pw_state_t *s);
void playSound(pw_state_t *s, uint8_t idx);

#define KEY_ENTER 2
#define KEY_LEFT  4
#define KEY_RIGHT 8
#define KEY_ANY   (KEY_ENTER | KEY_LEFT | KEY_RIGHT)
void interactionHandlerPokeRadarFailureUserTooSlow(pw_state_t *s);
void interactionHandlerRandomGiftFromBoredPokemon(pw_state_t *s);
void interactionHandlerPokemonArrivedAnimation(pw_state_t *s);
void interactionHandlerWalkEnd(pw_state_t *s);
void interactionHandlerSwitchScreen(pw_state_t *s);
void interactionHandlerGiftedItemsScreen(pw_state_t *s);
void interactionHandlerPokemonAndItemsScreen(pw_state_t *s);
void interactionHandlerSettings(pw_state_t *s);
void interactionHandlerTrainerCardAndStatsScreens(pw_state_t *s);
void interactionHandlerSomethingReceived(pw_state_t *s);
void pickPokemonToEncounter(pw_state_t *s);
void goToPokeRadarMicroApp(pw_state_t *s);
void startBattle(pw_state_t *s);
void interactionHandlerPokeRadar(pw_state_t *s);
void dowsingFindItem(pw_state_t *s);
void interactionHandlerDowsing(pw_state_t *s);
void drawTrainerCard(pw_state_t *s);
void drawTrainerCardAndStatsScreens(pw_state_t *s);
void interactionHandlerPokeBattle(pw_state_t *s);
void battleScreenDraw(pw_state_t *s);
void drawPeerPlay(pw_state_t *s);
