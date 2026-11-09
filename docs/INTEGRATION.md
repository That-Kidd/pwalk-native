# Embedding the walker

How to put the native core in another program, such as a HeartGold/SoulSilver
port. Everything here is in `include/pokewalker.h` and `include/pw_hal.h`;
link `pwcore` (plain C99, no dependencies; usable from C++).

## Lifecycle

The core reads the firmware's data (fonts, strings, tables) from the
user's own ROM dump: call `pw_rom_load` once before anything else. It
returns `PW_ROM_OK`, or `PW_ROM_BAD_SIZE` / `PW_ROM_BAD_HASH` for a wrong
file. The buffer can be freed afterwards.

```c
static pw_state_t walker;
static pw_hal_t hal = { .user = my_ctx, .present = ..., .buttons = ...,
                        .eeprom_read = ..., .eeprom_write = ...,
                        .accel_sample = ..., .beep = ...,
                        .ir_send = ..., .ir_recv = ... };

pw_rom_load(rom, rom_len);         // the user's 48 KB ROM dump, once per process

if (!have_save_state || !pw_load(&walker, state, state_len, &hal)) {
    pw_init(&walker, &hal);          // boots from the EEPROM
}
for (;;) {                           // e.g. once per frame
    pw_update(&walker, elapsed_ms);
}
```

`pw_update` runs the walker for that much of its own time and then calls
`present` once with the current screen. It is cheap: idle, an hour of
walker time takes a few milliseconds, so it is fine to call every frame or
to catch up in one large call.

The walker has no clock of its own. It counts time only through
`pw_update`, and when it is loaded it resumes from the last saved time. If
you want it to catch up on time spent closed, call `pw_update` with that
time (in chunks of a few seconds is fine).

## Saving

Two things make up a walker:

- **The EEPROM** (64 KB), through `eeprom_read` / `eeprom_write`. It has
  the real walker's layout, so dumps from a real walker or from emulators
  work as they are. The firmware writes it as things happen.
- **The save state** from `pw_save` (`pw_save_size()` bytes, about 6 KB):
  RAM, screen and timers. The firmware keeps some things only in RAM, such
  as today's steps and the current screen, so save this too. It has a
  fixed, versioned, endian-independent layout; `pw_load` returns 0 for data
  it can't read, and then `pw_init` boots from the EEPROM alone.

Save both together. A save state without its matching EEPROM will not make
sense to the walker.

## Inputs and outputs

- `buttons`: the held buttons as `PW_BUTTON_ENTER | PW_BUTTON_LEFT |
  PW_BUTTON_RIGHT`. It's sampled once per `pw_update`, so a press must last
  at least one call. After a fresh boot the screen stays off for a minute,
  as on the real walker; holding Enter for half a second then wakes it.
- `accel_sample`: one reading in the firmware's units, signed, 64 per g
  (flat at rest: 0, 0, 64). It is sampled up to 16 times a second. Steps
  count at about 1 to 2.6 steps a second of up-and-down motion; the walker
  counts in 4-second windows. Leave it NULL and the walker sits still.
- `present`: 96×64 pixels, 0 (white) to 3 (black).
- `battery`: optional; the reading in twentieths of the walker's calibrated
  low-battery reference, so above 20 is healthy (255 is the maximum). It's
  checked hourly while counting steps; at 20 or less the low-battery icon
  appears. NULL means always healthy.
- `beep`: called when the tone changes, with a frequency in Hz and volume
  0 (off), 1 or 2.

## IR

The walker talks to the game through `ir_send(byte)` and `ir_recv()`
(return -1 when nothing has arrived). The bytes are the ones on the air,
already XORed with 0xAA by the sender, so pass them straight through to and
from whatever plays the game's side.

Timing is in walker time (`pw_update`'s milliseconds):

- A packet ends when no byte arrives for about 0.12 ms. The walker reads
  one byte per tick (1/32768 s), so deliver a whole packet's bytes in the
  same update, and its reply arrives during that update or the next.
- Advertisement: the walker sends 0xFC (0x56 on the air) roughly every
  0.1 s and gives up after 20 tries ("No trainer found"). The other side
  answers within 0.1 s.
- While the walker is not in an IR session, nothing reads `ir_recv`; drop
  bytes sent then, as the real receiver would, rather than queueing them.

The game's commands (walk start 0x5A / 0x38, end 0x4E, clock sync 0x32 /
0x40 / 0x52 / 0x60, identity 0x20, EEPROM writes 0x00-0x0A, event gifts
0xA0-0xD6, ...) are handled in `src/ir.c`; what the walker does at the end
of a session is `performActionAsRequestedByRemote` in `src/loop.c`.

`frontend/sdl/ir_udp.c` (`--ir`) and `tests/peertest.c` are working examples of the IR
callbacks.
