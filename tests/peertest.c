#include <stdio.h>
#include <string.h>
#include "pw_internal.h"
#include "testrom.h"

typedef struct walker {
    pw_state_t s;
    uint8_t eeprom[1 << 16];
    uint8_t rx[4096];
    unsigned rx_head, rx_tail;
    uint8_t buttons;
    struct walker *peer;
} walker_t;

static walker_t w[2];

static void ee_read(void *u, uint16_t a, void *d, uint16_t n) {
    walker_t *x = u;
    for (uint16_t i = 0; i < n; i++)
        ((uint8_t *)d)[i] = x->eeprom[(uint16_t)(a + i)];
}

static void ee_write(void *u, uint16_t a, const void *src, uint16_t n) {
    walker_t *x = u;
    for (uint16_t i = 0; i < n; i++)
        x->eeprom[(uint16_t)(a + i)] = ((const uint8_t *)src)[i];
}

static uint8_t buttons(void *u) {
    return ((walker_t *)u)->buttons;
}

static void ir_send(void *u, uint8_t b) {
    walker_t *o = ((walker_t *)u)->peer;
    if (rd16(&o->s, RAM_currentEventLoopFunc) == 0x08D6) {
        o->rx[o->rx_tail++ % sizeof(o->rx)] = b;
    }
}

static int ir_recv(void *u) {
    walker_t *x = u;
    return x->rx_head == x->rx_tail ? -1 : x->rx[x->rx_head++ % sizeof(x->rx)];
}

static void run(int ms) {
    for (int i = 0; i < ms; i++) {
        pw_update(&w[0].s, 1);
        pw_update(&w[1].s, 1);
    }
}

static void press(walker_t *x, uint8_t b) {
    x->buttons = b;
    run(60);
    x->buttons = 0;
    run(400);
}

int main(int argc, char **argv) {
    if (argc < 3 || !test_load_rom(argv[1])) {
        fprintf(stderr, "usage: %s ROM.bin EEPROM.bin\n", argv[0]);
        return 2;
    }
    static pw_hal_t hal[2];
    for (int i = 0; i < 2; i++) {
        FILE *f = fopen(argv[2], "rb");
        if (!f || fread(w[i].eeprom, 1, sizeof(w[i].eeprom), f) != sizeof(w[i].eeprom)) {
            fprintf(stderr, "can't read %s\n", argv[2]);
            return 2;
        }
        fclose(f);
        w[i].peer = &w[1 - i];
        hal[i] = (pw_hal_t){ .user = &w[i],
                             .buttons = buttons,
                             .eeprom_read = ee_read,
                             .eeprom_write = ee_write,
                             .ir_send = ir_send,
                             .ir_recv = ir_recv };
        pw_init(&w[i].s, &hal[i]);
    }
    {
        pw_state_t *x = &w[1].s;
        uint16_t buf = 0xFE00;
        readReliableDataFromEeprom(x, 0xED, 0x1ED, buf, 0x68);
        wr8(x, buf + 8, rd8(x, buf + 8) ^ 0x55);
        writeReliableDataToEeprom(x, 0xED, 0x1ED, buf, 0x68);
    }

    run(62000);
    for (int i = 0; i < 2; i++)
        w[i].buttons = PW_BUTTON_ENTER;
    run(1500);
    for (int i = 0; i < 2; i++)
        w[i].buttons = 0;
    run(500);
    for (int i = 0; i < 2; i++) {
        pw_state_t *x = &w[i].s;
        for (int tries = 0; tries < 40; tries++) {
            if (rd8(x, RAM_currentlyActiveView) == 1 && rd8(x, RAM_menuCurItem) == 2 && !rd8(x, RAM_gCurSubstateY)) {
                break;
            }
            press(&w[i], rd8(x, RAM_currentlyActiveView) == 1 ? PW_BUTTON_RIGHT : PW_BUTTON_ENTER);
        }
    }
    press(&w[0], PW_BUTTON_ENTER);
    press(&w[1], PW_BUTTON_ENTER);
    run(5000);

    int ok = 1;
    for (int i = 0; i < 2; i++) {
        uint8_t view = rd8(&w[i].s, RAM_currentlyActiveView);
        printf("walker %d: view %02X, error %d\n", i, view, rd8(&w[i].s, RAM_commsErrorMessageId));
        ok &= view == 0x0D;
    }
    static const struct {
        const char *what;
        uint16_t src, dst, n;
    } xfer[] = {
        { "sprite", 0x91BE, 0xF400, 0x180 },
        { "name image", 0x993E, 0xF580, 0x140 },
        { "trainer record", 0xCC00, 0xDC00, 0x224 },
    };
    for (int d = 0; d < 2; d++) {
        for (int k = 0; k < 3; k++) {
            int same = !memcmp(&w[1 - d].eeprom[xfer[k].src], &w[d].eeprom[xfer[k].dst], xfer[k].n);
            printf("walker %d has walker %d's %s: %s\n", d, 1 - d, xfer[k].what, same ? "yes" : "NO");
            ok &= same;
        }
    }
    printf(ok ? "peer play ok\n" : "peer play FAILED\n");
    return !ok;
}
