#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

static const uint32_t palette[4] = { 0xFFB8C8A8, 0xFF889878, 0xFF586850, 0xFF283020 };

int main(int argc, char **argv) {
    const char *path = NULL, *rom = NULL, *script = NULL, *shot = "screen.pgm", *ir = NULL;
    double headless = 0;
    int scale = 6;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--scale") && i + 1 < argc) {
            scale = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--headless") && i + 1 < argc) {
            headless = atof(argv[++i]);
        } else if (!strcmp(argv[i], "--script") && i + 1 < argc) {
            script = argv[++i];
        } else if (!strcmp(argv[i], "--shot") && i + 1 < argc) {
            shot = argv[++i];
        } else if (!strcmp(argv[i], "--rom") && i + 1 < argc) {
            rom = argv[++i];
        } else if (!strcmp(argv[i], "--ir") && i + 1 < argc) {
            ir = argv[++i];
        } else {
            path = argv[i];
        }
    }
    if (!install_rom(rom)) {
        return 1;
    }
    if (!path && headless <= 0) {
        path = pick_file("Pokewalker EEPROM (*.bin;*.sav;*.eep)\0*.bin;*.sav;*.eep\0All files\0*.*\0",
                         "Open a Pokewalker EEPROM (64 KB); a new name starts a blank walker");
    }
    if (!path) {
        fprintf(stderr, "usage: %s [--rom PWROM.bin] [--scale N] [--headless S [--script ...] [--shot F]] EEPROM.bin\n",
                argv[0]);
        return 1;
    }

    static frontend_t f;
    load_eeprom(&f, path);
    static pw_hal_t hal;
    frontend_hal_init(&f, &hal);
    if (ir) {
        if (!ir_open(&f, ir)) {
            return 1;
        }
        hal.ir_send = hal_ir_send;
        hal.ir_recv = hal_ir_recv;
    }

    if (headless > 0) {
        start(&f, path, &hal);
        run_headless(&f, headless, script, shot);
        save_eeprom(&f, path);
        save_state(&f, path);
        return 0;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("Pokewalker", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       PW_SCREEN_WIDTH * scale, PW_SCREEN_HEIGHT * scale, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(ren, PW_SCREEN_WIDTH, PW_SCREEN_HEIGHT);
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, PW_SCREEN_WIDTH,
                                         PW_SCREEN_HEIGHT);

    SDL_AudioSpec want = { 0 }, have;
    want.freq = 48000;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = audio_callback;
    want.userdata = &f;
    f.audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (f.audio) {
        SDL_PauseAudioDevice(f.audio, 0);
    }

    start(&f, path, &hal);
    Uint32 last = SDL_GetTicks(), saved = last;
    for (int running = 1; running;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                int down = e.type == SDL_KEYDOWN;
                uint8_t bit = 0;
                switch (e.key.keysym.sym) {
                case SDLK_LEFT: bit = PW_BUTTON_LEFT; break;
                case SDLK_RIGHT: bit = PW_BUTTON_RIGHT; break;
                case SDLK_DOWN:
                case SDLK_SPACE:
                case SDLK_RETURN: bit = PW_BUTTON_ENTER; break;
                case SDLK_w:
                    if (down && !e.key.repeat)
                        f.walking = f.walking == 1 ? 0 : 1;
                    break;
                case SDLK_r:
                    if (down && !e.key.repeat)
                        f.walking = f.walking == 2 ? 0 : 2;
                    break;
                case SDLK_ESCAPE: running = 0; break;
                default: break;
                }
                f.buttons = down ? f.buttons | bit : f.buttons & ~bit;
            }
        }
        Uint32 now = SDL_GetTicks();
        pw_update(&f.walker, now - last);
        last = now;
        if (now - saved >= 60000) {
            save_eeprom(&f, path);
            save_state(&f, path);
            saved = now;
        }
        if (ir) {
            ir_flush(&f);
        }

        uint32_t *px;
        int pitch;
        SDL_LockTexture(tex, NULL, (void **)&px, &pitch);
        for (int y = 0; y < PW_SCREEN_HEIGHT; y++) {
            for (int x = 0; x < PW_SCREEN_WIDTH; x++) {
                px[y * pitch / 4 + x] = palette[f.screen[y][x] & 3];
            }
        }
        SDL_UnlockTexture(tex);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
        SDL_SetWindowTitle(win, f.walking == 2 ? "Pokewalker (running)"
                                : f.walking    ? "Pokewalker (walking)"
                                               : "Pokewalker");
    }
    save_eeprom(&f, path);
    save_state(&f, path);
    if (f.audio) {
        SDL_CloseAudioDevice(f.audio);
    }
    SDL_Quit();
    return 0;
}
