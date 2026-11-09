#include <math.h>
#include <stdio.h>
#include <string.h>
#include "pw_internal.h"
#include "testrom.h"

static uint8_t eeprom[2][1 << 16];
static pw_state_t walker[2];

static void ee_read(void *u, uint16_t a, void *d, uint16_t n) {
    uint8_t *e = u;
    for (uint16_t i = 0; i < n; i++)
        ((uint8_t *)d)[i] = e[(uint16_t)(a + i)];
}

static void ee_write(void *u, uint16_t a, const void *src, uint16_t n) {
    uint8_t *e = u;
    for (uint16_t i = 0; i < n; i++)
        e[(uint16_t)(a + i)] = ((const uint8_t *)src)[i];
}

static void accel(void *u, int8_t *x, int8_t *y, int8_t *z) {
    const pw_state_t *s = &walker[u == eeprom[0] ? 0 : 1];
    *x = *y = 0;
    *z = (int8_t)lround(64 + 40 * sin(4 * M_PI * s->hw.now / 32768.0));
}

int main(int argc, char **argv) {
    if (argc < 3 || !test_load_rom(argv[1])) {
        fprintf(stderr, "usage: %s ROM.bin EEPROM.bin\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[2], "rb");
    if (!f || fread(eeprom[0], 1, sizeof(eeprom[0]), f) != sizeof(eeprom[0])) {
        fprintf(stderr, "usage: %s ROM.bin EEPROM.bin\n", argv[0]);
        return 2;
    }
    fclose(f);
    static pw_hal_t hal[2];
    for (int i = 0; i < 2; i++) {
        hal[i] =
            (pw_hal_t){ .user = eeprom[i], .eeprom_read = ee_read, .eeprom_write = ee_write, .accel_sample = accel };
    }
    pw_init(&walker[0], &hal[0]);
    for (int i = 0; i < 90000; i++)
        pw_update(&walker[0], 1);

    static uint8_t buf[1 << 16];
    size_t n = pw_save(&walker[0], buf, sizeof(buf));
    memcpy(eeprom[1], eeprom[0], sizeof(eeprom[0]));
    if (n != pw_save_size() || !pw_load(&walker[1], buf, n, &hal[1])) {
        printf("save/load FAILED\n");
        return 1;
    }
    for (int i = 0; i < 120000; i++) {
        pw_update(&walker[0], 1);
        pw_update(&walker[1], 1);
    }
    int ok = !memcmp(walker[0].ram, walker[1].ram, sizeof(walker[0].ram)) &&
             !memcmp(&walker[0].lcd, &walker[1].lcd, sizeof(walker[0].lcd)) &&
             !memcmp(eeprom[0], eeprom[1], sizeof(eeprom[0])) && rd32(&walker[0], RAM_stepCountTodaySoFar) > 0;
    printf("%zu-byte state, %u steps today: %s\n", n, rd32(&walker[0], RAM_stepCountTodaySoFar),
           ok ? "save state ok" : "save state FAILED");
    return !ok;
}
