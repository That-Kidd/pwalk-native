#include <string.h>
#include "pw_internal.h"

#define SAVE_MAGIC   "PWS"
#define SAVE_VERSION 1

typedef struct cursor {
    uint8_t *w;
    const uint8_t *r;
    size_t pos;
} cursor_t;

static void bytes(cursor_t *c, void *p, size_t n) {
    if (c->w) {
        memcpy(c->w + c->pos, p, n);
    } else {
        memcpy(p, c->r + c->pos, n);
    }
    c->pos += n;
}

static void u8(cursor_t *c, uint8_t *v) {
    bytes(c, v, 1);
}

static void u16(cursor_t *c, uint16_t *v) {
    uint8_t b[2] = { (uint8_t)(*v >> 8), (uint8_t)*v };
    bytes(c, b, 2);
    *v = (uint16_t)(b[0] << 8 | b[1]);
}

static void u32(cursor_t *c, uint32_t *v) {
    uint16_t hi = (uint16_t)(*v >> 16), lo = (uint16_t)*v;
    u16(c, &hi);
    u16(c, &lo);
    *v = (uint32_t)hi << 16 | lo;
}

static void u64(cursor_t *c, uint64_t *v) {
    uint32_t hi = (uint32_t)(*v >> 32), lo = (uint32_t)*v;
    u32(c, &hi);
    u32(c, &lo);
    *v = (uint64_t)hi << 32 | lo;
}

static void fields(cursor_t *c, pw_state_t *s) {
    char magic[4] = SAVE_MAGIC;
    uint8_t version = SAVE_VERSION;
    bytes(c, magic, 3);
    u8(c, &version);
    bytes(c, s->ram, sizeof(s->ram));

    pw_lcd_t *l = &s->lcd;
    bytes(c, l->ram, sizeof(l->ram));
    u8(c, &l->page);
    u8(c, &l->column);
    u8(c, &l->start_line);
    u8(c, &l->contrast);
    u8(c, &l->display_on);
    u8(c, &l->power_save);
    bytes(c, l->cmd, sizeof(l->cmd));
    u8(c, &l->cmd_len);

    pw_hw_t *h = &s->hw;
    u64(c, &h->now);
    u8(c, &h->rtc_sec);
    u8(c, &h->rtc_min);
    u8(c, &h->rtc_hour);
    u8(c, &h->rtc_quarter);
    u64(c, &h->rtc_next);
    u8(c, &h->rtcflg);
    u8(c, &h->rtccr2);
    u8(c, &h->tb_on);
    u64(c, &h->tb_next);
    u8(c, &h->tw_clock);
    u8(c, &h->tmrw);
    u8(c, &h->tw_irq);
    u16(c, &h->gra);
    u16(c, &h->grb);
    u16(c, &h->grc);
    u64(c, &h->tw_start);
    u8(c, &h->irq0_flag);
    u8(c, &h->keys_last);
    u8(c, &h->masked);
    u64(c, &h->ir_base);
}

size_t pw_save_size(void) {
    static pw_state_t scratch;
    static uint8_t buf[sizeof(pw_state_t) + 64];
    cursor_t c = { buf, NULL, 0 };
    fields(&c, &scratch);
    return c.pos;
}

size_t pw_save(const pw_state_t *s, void *buf, size_t size) {
    size_t need = pw_save_size();
    if (size < need) {
        return 0;
    }
    cursor_t c = { buf, NULL, 0 };
    fields(&c, (pw_state_t *)s);
    return c.pos;
}

int pw_load(pw_state_t *s, const void *buf, size_t size, const pw_hal_t *hal) {
    const uint8_t *b = buf;
    if (size != pw_save_size() || memcmp(b, SAVE_MAGIC, 3) || b[3] != SAVE_VERSION) {
        return 0;
    }
    memset(s, 0, sizeof(*s));
    cursor_t c = { NULL, b, 0 };
    fields(&c, s);
    pw_set_hal(s, hal);
    hw_resync_beeper(s);
    return 1;
}
