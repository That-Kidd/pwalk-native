# Architecture

## Source layout

- `src/firmware/`: the decompiled firmware: the event loops and boot
  (`loop.c`), views and state (`state.c`), input handlers (`handlers.c`),
  drawing (`sprites.c`, `image.c`, `text.c`), the IR protocol (`ir.c`),
  the factory/debug views (`debug.c`) and arithmetic helpers (`util.c`)
- `src/hw/`: the hardware the firmware drives: timers and interrupts
  (`machine.c`), the LCD controller (`lcd.c`) and the SPI EEPROM (`eeprom.c`)
- `src/api/`: the public API around it: ROM loading (`rom.c`), save states
  (`save.c`) and the HAL (`core.c`)
- `src/pw_internal.h`: RAM access helpers and internal prototypes
- `src/ram_map.h`: names for the firmware's RAM variables
- `src/rom_data.h`: access to the firmware data loaded from the user's ROM

The walker's RAM is one array at its original addresses (0xF780-0xFF7F)
in its original big-endian byte order, so the decompiled code reads and
writes it exactly as the firmware did.

## How it runs

The firmware sleeps until an interrupt, does a little work, and sleeps
again. `src/machine.c` models the hardware that drives this: the RTC
(quarter-second, second, minute and hour interrupts), Timer B1 (16 Hz
wake-ups while counting steps), Timer W (the sound sequencer and the
beeper's tone) and the Enter key's IRQ0. Time is virtual, counted in ticks
of the 32768 Hz clock. `pw_update(s, ms)` jumps from one interrupt to the
next and runs the event loop after each one (`src/loop.c`), so an hour of
walker time takes a few milliseconds.

The clock resumes from the time last saved; it does not catch up on time
spent switched off.

IR goes through two optional HAL callbacks, `ir_send` and `ir_recv`, which
carry the bytes as they would travel over the air (already XORed with 0xAA).
The whole session protocol is decompiled (`src/ir.c`): advertising and
session set-up, walker-to-walker peer play, the game's commands (identity,
clock sync, starting and ending a walk, event gifts, EEPROM and RAM reads
and writes) and the factory commands. Without the callbacks a connection
attempt fails with "No trainer found", as with nobody nearby.
`tests/peertest.c` links two walkers and checks a full peer-play session.

The battery level comes from an optional HAL callback (always full
without it); the walker checks it hourly while counting steps and shows its
low-battery icon when the reading is at or below its calibrated reference. The factory self-test view works; the boot-time
factory test, which needs a Nintendo test tool on the SPI bus, is skipped.

## Decompiled routines

| Area | Functions |
|---|---|
| Arithmetic | mul32, divmod32, memoryCopy |
| Random numbers | setRandomSeed, rand |
| Step counters | clampTotalStepToLimit |
| EEPROM | eepromReadToRamAlso, eepromReadSingleByte, eepromWriteData, eepromWriteSingleByte, eepromFillWithByte, eepromFillPageWithByte, eepromWrite128Bytes |
| Redundant records | readReliableDataFromEeprom, writeReliableDataToEeprom |
| Scratch heap | setHeapPointer, static_malloc |
| LCD controller | model of the controller (`lcdc_*`), `pw_render` |
| LCD | lcdSendSingleByteCommand, lcdClearBackBuffer, lcdFlipBuffers, drawImageToScreen, lcdFillRectangle, drawImageToBuffer |
| LCD setup | lcdInit, lcdSetContrast, lcdEnter/ExitPowerSaveMode, lcdSetAddress, lcdShowBuffer, lcdFillBackBuffer |
| Box outlines | drawBoxOutlineRows32To47, drawBoxOutlineRows48To63 |
| Text | drawTinyChars, drawNumberOnscreen, drawNumberAndWattsymbolOnscreen |
| State helpers | view/sub-state setters, setEventLoopFunc, slot searches, saved-record helpers (special item, event log, signature), checkKeyStates, bitfieldInsert |
| Game logic | addWatts, clearWattsAndErasePokemon, applyDefaultSettings, getItemIdForNthItemOnRoute, rollBattleChance, updateCachedWalkerStatus, pushSeenTrainersLogDown, goToDowsingMicroApp |
| Sprites | drawItemSymbolOnScreen, drawLargeItemChest, drawLargePresentItem, drawPokemonFeelingBubble, drawCurPokemonSmallAnimatedFrame(+Flipped), drawCurrentAreaSmallImage, drawMusicNote, drawRewardReceivedBanner |
| Names | showRxedItemName, drawPeerPokemonName, showEventPokeName, drawNameOfEventPokemon, drawCurPokemonNameWithBorders, drawNameOfRouteAvailablePokemon, drawRouteAvailableItemName |
| More sprites | drawPeerPlayPokemonFrame, drawLowBatteryIcon, drawRouteAvailablePokemon |
| More logic | hasParticipatedInEvent, cursorToPrevOwnedSlotWrapping, wipeOutStepsAndSyncTime, setThatWeveParticipatedInAGivenEvent, copyTeamAndconfigDataFromStagingArea, end_walk_action, cursorToNextOwnedSlot(+Wrapping), cursorToFirstSlotOfA/B |
| Walking | accountForStepTaken, dayEnded, chooseWalkEvent, pokemonJoinsEmptyWalkerWalk, rtcSet, logEvent, takeItemAndPokeInventory |
| Step detection | processAccelerometerSamplesAxis (64-point FFT), findWalkingPeak, spectrumPeakCentroid, processAccelSamples |
| Screens | homeScreenSlidePokemon, homeScreenPacePokemon, homeScreenDrawLargeCurrentPokemonImage, drawCloudWithBarsOnTopAndBottomOfScreen(2), drawPokewalkerAndCompletedMessage, drawLargePokemonImageWithBars, pokemonArrivalScreen, pokemonDepartureScreen, pokeradarUserTooSlowScreen, drawCurSelectedPoke/ItemNameForSwitchScreen, drawMovingBlankIconWithBarsOnTopAndBottom, drawBallDropFromTopOfScreen, drawPokeballInMiddleOfScreen, drawHomeScreenMain, drawSad/HappyPokewalker, drawConnectingScreen, showCommsErrorMessage, showGoodJobBanner, drawPokemonArrivedAnimation, drawSwitchScreen, drawLineAtRow48, showPokemonFeelingsAndRandomFindings, drawPokewalkerInitialConnectionInstructionScreen |
| Menus | mainMenuDraw, drawSettingsScreen, drawStepStatisticsScreen, pokeRadarDraw, giftedItemsView, drawPokemonAndItemsScreen |
| Images in RAM | imageFlipHorizontal, moveImageVerticaly, textImageAddBorders, imageOverlayMasked |
| Activities | Poke Radar (pickPokemonToEncounter, goToPokeRadarMicroApp, startBattle), battles (interactionHandlerPokeBattle, battleScreenDraw), dowsing (dowsingFindItem, dowsingScreen, dowsingDraw), peer play (goToPeerPlayMicroApp, drawPeerPlay, calculateAndApplyPeerGift), random gifts (addRandomGift) |
| Input handlers | every view's interactionHandler*, including home, main menu and settings |
| Records | recordPokeCaught*, recordItemFound*, maybeFoundSomeEventItem, startWalkEepromActions, logPeerPlay, some_sort_of_eeprom_init, checkForAndInitEepromContentsIfNeeded |
| Sound | playSound, the Timer W sequencer (timer_w_irq), setBeeperFreq |
| Runtime | boot, sleepModeEventLoop, normalModeEventLoop, interactionHandler, maybe_viewDoDisplay, the RTC / Timer B / IRQ0 interrupt handlers, mode switches, key debouncing, accelerometer sampling and movement detection, rtcReadBcd |
| End of IR session | performActionAsRequestedByRemote |
