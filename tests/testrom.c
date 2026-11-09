#include <stdio.h>
#include "pokewalker.h"
#include "testrom.h"

int test_load_rom(const char *path) {
    static unsigned char rom[PW_ROM_SIZE + 1];
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "can't open ROM %s\n", path);
        return 0;
    }
    size_t n = fread(rom, 1, sizeof(rom), f);
    fclose(f);
    int r = pw_rom_load(rom, n);
    if (r != PW_ROM_OK) {
        fprintf(stderr, "%s: %s\n", path,
                r == PW_ROM_BAD_SIZE ? "not a 48 KB Pokewalker ROM" : "SHA-1 does not match the Pokewalker ROM");
        return 0;
    }
    return 1;
}
