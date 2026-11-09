# pwalk-native

A decompilation and native PC port of the Pokéwalker firmware: readable,
portable C99 that runs on Linux and Windows with no emulator. Every routine
was checked against the original ROM for identical behavior during
development, and the walker's RAM and EEPROM keep their original byte
layout, so saves stay compatible with real hardware. The core can also be
embedded in other programs through a small hardware layer.

**This repository contains no Nintendo code or data.** The firmware's data
(fonts, strings, images and tables) is read at startup from your own dump
of the Pokewalker ROM, which you must provide. This is an unofficial fan
project, not affiliated with or endorsed by Nintendo, Game Freak, Creatures
or The Pokémon Company. Pokémon and Pokéwalker are their trademarks.

## Layout

```
include/        public API: pokewalker.h, pw_hal.h
src/firmware/   the decompiled firmware
src/hw/         the hardware it drives (timers, LCD, EEPROM)
src/api/        ROM loading, save states, HAL glue
frontend/sdl/   SDL2 desktop frontend (pwpc)
tests/          peertest, savetest
docs/           INTEGRATION.md (embedding), ARCHITECTURE.md (internals)
scripts/        build helpers
```

## What you need

- **Your Pokewalker ROM dump**: 48 KB, SHA-1
  `19a4c4fd64c7087b2eb62a3a30cfbb88d2a1f744`. Only addresses
  0xBAC6-0xBFFF are used; the dump is checked in full so a wrong or
  corrupt file is refused.
- **An EEPROM** (64 KB): a dump from your walker, or a new file name to
  start a blank walker.
- CMake 3.13+, a C99 compiler, and SDL2 for the frontend.

## Building

```
cmake -B build
cmake --build build
```

Pass `-DPW_BUILD_FRONTEND=OFF` to build only the core library (no SDL2).

### Tests

The tests need your dumps. Point CMake at them (by default it looks for
`PWROM.bin` and `PWEEPROM.bin` in the source folder):

```
cmake -B build -DPW_ROM=/path/PWROM.bin -DPW_EEPROM=/path/PWEEPROM.bin
cmake --build build
ctest --test-dir build
```

### Windows exe (cross-compiled on Linux)

```
scripts/build-windows.sh
```

This builds `build-win/pokewalker.exe`, a single exe with SDL2 linked in
that runs on Windows 10 or later. The first run downloads
[zig](https://ziglang.org) 0.13.0 (used as the Windows C compiler) and
SDL2 2.30.9's MinGW development package into `~/.cache/pokewalker-build`,
each checked by SHA-256; it needs `curl`, `tar` and `sha256sum`. Set `ZIG`
or `SDL2` to use copies you already have.

The exe contains no ROM data. On Windows, run it and pick your ROM in the
dialog the first time (it is then installed to `%APPDATA%\pokewalker`),
then your EEPROM; or drag the EEPROM onto the exe.

## Running

```
./build/pwpc --rom PWROM.bin PWEEPROM.bin
```

`--rom` is only needed the first time: a valid ROM is copied to
`~/.local/share/pokewalker/PWROM.bin` (`%APPDATA%\pokewalker` on Windows)
and found there afterwards. Without `--rom` or an installed copy, `pwpc`
also tries `PWROM.bin` in the current folder.

The EEPROM file is read at start and written back on exit and every
minute. The rest of the walker (RAM, screen, clock) is saved next to it in
`PWEEPROM.bin.state`, so it resumes exactly; today's steps live only there
until the walker writes them to the EEPROM. A file name that doesn't exist
starts a blank walker.

As on the real walker, after a fresh start the screen stays off for a
minute; then hold the middle button for half a second to wake it.

| Key | Action |
|---|---|
| Left / Right | left and right buttons |
| Down, Enter or Space | middle button |
| W | toggle walking (two steps a second) |
| R | toggle running (2.5 steps a second) |
| Esc | quit (and save) |

### Options

| Option | |
|---|---|
| `--rom PATH` | your ROM dump (installs it) |
| `--scale N` | window scale (default 6) |
| `--headless SECONDS` | run without a window and write the screen to a PGM file |
| `--script "62:E,64:"` | with `--headless`: at each time in seconds, hold these keys (`L`, `E`, `R`; `W` walk, `U` run) |
| `--shot FILE` | with `--headless`: where to write the screen (default `screen.pgm`) |
| `--ir LISTEN:HOST:PORT` | connect the walker's IR to another program over UDP |

Two walkers on one PC can peer-play:

```
./build/pwpc --ir 7000:127.0.0.1:7001 a.bin
./build/pwpc --ir 7001:127.0.0.1:7000 b.bin
```

Each update's bytes go out as one datagram, so a packet always arrives
whole (the walker ends a packet at a 4-tick gap).

## Credits

Function and RAM names follow Dmitry Grinberg's annotated disassembly of
the Pokewalker firmware, which made this project possible.

## License

The code in this repository is under the MIT license (see `LICENSE`). It
does not cover the Pokewalker firmware or its data, which you supply
yourself.
