#include <stdio.h>
#include <string.h>
#include "frontend.h"
#ifdef _WIN32
#include <commdlg.h>
#endif

void load_eeprom(frontend_t *f, const char *path) {
    memset(f->eeprom, 0xFF, sizeof(f->eeprom));
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "%s not found: starting a blank walker\n", path);
        return;
    }
    size_t n = fread(f->eeprom, 1, sizeof(f->eeprom), fp);
    fclose(fp);
    if (n != sizeof(f->eeprom)) {
        fprintf(stderr, "%s: only %zu bytes\n", path, n);
    }
}

void save_eeprom(frontend_t *f, const char *path) {
    FILE *fp = fopen(path, "wb");
    if (!fp || fwrite(f->eeprom, 1, sizeof(f->eeprom), fp) != sizeof(f->eeprom)) {
        fprintf(stderr, "couldn't save %s\n", path);
    }
    if (fp) {
        fclose(fp);
    }
}

void save_pgm(frontend_t *f, const char *path) {
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        return;
    }
    fprintf(fp, "P5 %d %d 255\n", PW_SCREEN_WIDTH, PW_SCREEN_HEIGHT);
    for (int y = 0; y < PW_SCREEN_HEIGHT; y++) {
        for (int x = 0; x < PW_SCREEN_WIDTH; x++) {
            fputc(255 - f->screen[y][x] * 85, fp);
        }
    }
    fclose(fp);
}

static void state_path(char *out, size_t n, const char *path) {
    snprintf(out, n, "%s.state", path);
}

int load_state(frontend_t *f, const char *path, const pw_hal_t *hal) {
    char sp[4096];
    state_path(sp, sizeof(sp), path);
    FILE *fp = fopen(sp, "rb");
    if (!fp) {
        return 0;
    }
    static uint8_t buf[1 << 16];
    size_t n = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    if (!pw_load(&f->walker, buf, n, hal)) {
        fprintf(stderr, "%s isn't a save state this version reads: booting fresh\n", sp);
        return 0;
    }
    return 1;
}

void save_state(frontend_t *f, const char *path) {
    char sp[4096];
    state_path(sp, sizeof(sp), path);
    static uint8_t buf[1 << 16];
    size_t n = pw_save(&f->walker, buf, sizeof(buf));
    FILE *fp = fopen(sp, "wb");
    if (!fp || fwrite(buf, 1, n, fp) != n) {
        fprintf(stderr, "couldn't save %s\n", sp);
    }
    if (fp) {
        fclose(fp);
    }
}

void start(frontend_t *f, const char *path, const pw_hal_t *hal) {
    if (!load_state(f, path, hal)) {
        pw_init(&f->walker, hal);
    }
}

const char *pick_file(const char *filter, const char *title) {
#ifdef _WIN32
    static char buf[MAX_PATH];
    OPENFILENAMEA ofn = { 0 };
    buf[0] = 0;
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = sizeof(buf);
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    return GetOpenFileNameA(&ofn) ? buf : NULL;
#else
    (void)filter;
    (void)title;
    return NULL;
#endif
}

void show_error(const char *msg) {
    fprintf(stderr, "%s\n", msg);
#ifdef _WIN32
    MessageBoxA(NULL, msg, "Pokewalker", MB_OK | MB_ICONERROR);
#endif
}

static int read_rom(const char *path, uint8_t *rom, size_t *size) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return 0;
    }
    *size = fread(rom, 1, PW_ROM_SIZE + 1, fp);
    fclose(fp);
    return 1;
}

/* Finds the user's ROM dump and loads the firmware data from it. The first
   time, the ROM is given with --rom or picked in a dialog; it is then
   copied into the per-user data folder so later runs find it there. */
int install_rom(const char *given) {
    static uint8_t rom[PW_ROM_SIZE + 1];
    size_t size = 0;
    char *pref = SDL_GetPrefPath(NULL, "pokewalker");
    char installed[1024] = "";
    if (pref) {
        snprintf(installed, sizeof(installed), "%sPWROM.bin", pref);
        SDL_free(pref);
    }

    const char *path = given;
    if (!path && installed[0] && read_rom(installed, rom, &size) && pw_rom_load(rom, size) == PW_ROM_OK) {
        return 1;
    }
    if (!path && read_rom("PWROM.bin", rom, &size)) {
        path = "PWROM.bin";
    }
    if (!path) {
        path = pick_file("Pokewalker ROM (*.bin)\0*.bin\0All files\0*.*\0",
                         "First run: select your Pokewalker ROM dump (48 KB)");
    }
    if (!path) {
        fprintf(stderr,
                "No Pokewalker ROM found. Run once with --rom PWROM.bin (your own 48 KB dump);\n"
                "it is then copied to %s.\n",
                installed[0] ? installed : "the user data folder");
        return 0;
    }
    if (!read_rom(path, rom, &size)) {
        char msg[1200];
        snprintf(msg, sizeof(msg), "Can't open the ROM %s", path);
        show_error(msg);
        return 0;
    }
    int r = pw_rom_load(rom, size);
    if (r != PW_ROM_OK) {
        char msg[1200];
        snprintf(msg, sizeof(msg), "%s is not the Pokewalker ROM (%s).", path,
                 r == PW_ROM_BAD_SIZE ? "it should be exactly 48 KB" : "its SHA-1 does not match");
        show_error(msg);
        return 0;
    }
    if (installed[0] && strcmp(path, installed) != 0) {
        FILE *fp = fopen(installed, "wb");
        if (fp && fwrite(rom, 1, PW_ROM_SIZE, fp) == PW_ROM_SIZE) {
            fprintf(stderr, "ROM installed to %s\n", installed);
        }
        if (fp) {
            fclose(fp);
        }
    }
    return 1;
}
