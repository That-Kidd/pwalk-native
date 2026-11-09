#include <math.h>
#include <string.h>
#include "frontend.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void hal_present(void *user, const uint8_t pixels[PW_SCREEN_HEIGHT][PW_SCREEN_WIDTH]) {
    frontend_t *f = user;
    memcpy(f->screen, pixels, sizeof(f->screen));
}

static uint8_t hal_buttons(void *user) {
    return ((frontend_t *)user)->buttons;
}

static void hal_eeprom_read(void *user, uint16_t addr, void *dst, uint16_t len) {
    frontend_t *f = user;
    for (uint16_t i = 0; i < len; i++) {
        ((uint8_t *)dst)[i] = f->eeprom[(uint16_t)(addr + i)];
    }
}

static void hal_eeprom_write(void *user, uint16_t addr, const void *src, uint16_t len) {
    frontend_t *f = user;
    for (uint16_t i = 0; i < len; i++) {
        f->eeprom[(uint16_t)(addr + i)] = ((const uint8_t *)src)[i];
    }
}

static void hal_accel_sample(void *user, int8_t *x, int8_t *y, int8_t *z) {
    frontend_t *f = user;
    double t = f->walker.hw.now / 32768.0;

    double w = f->walking == 2 ? 1.5 : f->walking ? 1.0 : 0.0;
    double hz = f->walking == 2 ? 2.5 : 2.0;
    *x = (int8_t)lround(w * 12 * sin(2 * M_PI * hz / 2 * t));
    *y = (int8_t)lround(w * 6 * sin(2 * M_PI * hz * t + 1));
    *z = (int8_t)lround(64 + w * 40 * sin(2 * M_PI * hz * t));
}

static void hal_beep(void *user, uint32_t freq, uint8_t volume) {
    frontend_t *f = user;
    if (f->audio) {
        SDL_LockAudioDevice(f->audio);
    }
    f->freq = freq;
    f->volume = volume;
    if (f->audio) {
        SDL_UnlockAudioDevice(f->audio);
    }
}

void frontend_hal_init(frontend_t *f, pw_hal_t *hal) {
    hal->user = f;
    hal->present = hal_present;
    hal->buttons = hal_buttons;
    hal->eeprom_read = hal_eeprom_read;
    hal->eeprom_write = hal_eeprom_write;
    hal->accel_sample = hal_accel_sample;
    hal->beep = hal_beep;
}

void audio_callback(void *user, Uint8 *stream, int len) {
    frontend_t *f = user;
    int16_t *out = (int16_t *)stream;
    int n = len / 2;
    double step = f->freq / 48000.0;
    int16_t amp = f->volume == 2 ? 6000 : f->volume == 1 ? 2500 : 0;
    for (int i = 0; i < n; i++) {
        out[i] = f->freq && amp ? (f->phase < 0.5 ? amp : -amp) : 0;
        f->phase += step;
        f->phase -= floor(f->phase);
    }
}
