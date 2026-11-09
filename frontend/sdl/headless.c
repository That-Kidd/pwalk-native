#include <stdlib.h>
#include <string.h>
#include "frontend.h"

static uint8_t parse_keys(const char *s, const char *end, int *walk) {
    uint8_t b = 0;
    *walk = 0;
    for (; s < end; s++) {
        switch (*s) {
        case 'L': b |= PW_BUTTON_LEFT; break;
        case 'E': b |= PW_BUTTON_ENTER; break;
        case 'R': b |= PW_BUTTON_RIGHT; break;
        case 'W': *walk = 1; break;
        case 'U': *walk = 2; break;
        default: break;
        }
    }
    return b;
}

void run_headless(frontend_t *f, double seconds, const char *script, const char *shot) {
    const char *next = script;
    double next_t = next ? atof(next) : 1e30;
    for (uint32_t ms = 0; ms < seconds * 1000; ms += 10) {
        while (next && ms >= next_t * 1000) {
            const char *colon = strchr(next, ':');
            const char *comma = strchr(next, ',');
            const char *end = comma ? comma : next + strlen(next);
            f->buttons = colon && colon < end ? parse_keys(colon + 1, end, &f->walking) : 0;
            next = comma ? comma + 1 : NULL;
            next_t = next ? atof(next) : 1e30;
        }
        pw_update(&f->walker, 10);
        if (f->ir_on) {
            ir_flush(f);
            SDL_Delay(10);
        }
    }
    save_pgm(f, shot);
}
