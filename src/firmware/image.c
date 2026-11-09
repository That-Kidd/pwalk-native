#include "pw_internal.h"

static int shifts_bge(uint8_t n) {
    return n <= 0x7F ? n : 0;
}

static int shifts_bmi(uint8_t n) {
    return n <= 0x80 ? n : 0;
}

void imageFlipHorizontal(pw_state_t *s, uint8_t w, uint8_t h, uint16_t img) {
    for (uint8_t stripe = 0; stripe < (h >> 3); stripe++) {
        uint8_t right = w - 1;
        for (uint8_t left = 0; left < (w >> 1); left++, right--) {
            uint16_t a = img + left * 2, b = img + right * 2;
            uint16_t t = rd16(s, a);
            wr16(s, a, rd16(s, b));
            wr16(s, b, t);
        }
        img += w * 2;
    }
}

void moveImageVerticaly(pw_state_t *s, uint8_t w, uint8_t h, uint16_t img, int8_t by) {
    uint16_t stride = w * 2;
    int stripes = h >> 3;
    if (by >= 0) {
        uint16_t row = img + (stripes - 1) * stride;
        int carry = shifts_bmi((uint8_t)(8 - by));
        for (uint8_t left = h >> 3; left != 0; left--) {
            for (uint16_t b = 0; b < stride; b++) {
                wr8(s, row + b, shl8(rd8(s, row + b), shifts_bge((uint8_t)by)));
                if (left != 1) {
                    uint8_t above = rd8(s, row + b - stride);
                    wr8(s, row + b, rd8(s, row + b) | shr8(above, carry));
                }
            }
            row -= stride;
        }
    } else {
        uint16_t row = img;
        int carry = shifts_bmi((uint8_t)(by + 8));
        for (int stripe = 0; stripe < stripes; stripe++) {
            for (uint16_t b = 0; b < stride; b++) {
                wr8(s, row + b, shr8(rd8(s, row + b), shifts_bge((uint8_t)-by)));
                if (stripe != stripes - 1) {
                    uint8_t below = rd8(s, row + b + stride);
                    wr8(s, row + b, rd8(s, row + b) | shl8(below, carry));
                }
            }
            row += stride;
        }
    }
}

#define BORDER_TOP    0x01
#define BORDER_BOTTOM 0x02
#define BORDER_LEFT   0x04
#define BORDER_RIGHT  0x08

void textImageAddBorders(pw_state_t *s, uint8_t w, uint8_t h, uint16_t img, uint8_t flags) {
    (void)h;
    uint16_t lower = img + w * 2;
    if (flags & BORDER_TOP) {
        for (uint8_t col = 0; col < w; col++) {
            wr16(s, img + col * 2, rd16(s, img + col * 2) | 0x0101);
        }
    }
    if (flags & BORDER_LEFT) {
        wr16(s, img, 0xFFFF);
        wr16(s, lower, 0xFFFF);
    }
    if (flags & BORDER_RIGHT) {
        wr16(s, lower - 2, 0xFFFF);
        wr16(s, img + w * 4 - 2, 0xFFFF);
    }
    if (flags & BORDER_BOTTOM) {
        for (uint8_t col = 0; col < w; col++) {
            wr16(s, lower + col * 2, rd16(s, lower + col * 2) | 0x8080);
        }
    }
}

void imageOverlayMasked(pw_state_t *s, uint16_t dst, uint16_t sprite, uint8_t dst_w, uint8_t dst_h, uint16_t mask,
                        uint8_t x, uint8_t y, uint8_t w) {
    uint8_t cols = (x + w > dst_w) ? (uint8_t)(dst_w - x) : w;
    uint16_t base = dst + (((y >> 3) * dst_w) + x) * 2;
    int yl = y & 7;
    int has_lower = y + 8 < dst_h;

    for (uint8_t c = 0; c < cols; c++) {
        uint16_t p = base + c * 2;
        uint8_t m = rd8(s, mask + c);
        wr8(s, p, rd8(s, p) & (shl8(m, 8 - yl) | shr8(0xFF, yl)));
        wr8(s, p + 1, rd8(s, p + 1) & (shl8(m, yl) | shr8(0xFF, 8 - yl)));
        if (has_lower) {
            uint16_t q = base + (c + dst_w) * 2;
            uint8_t keep = shr8(m, 8 - yl) | (uint8_t)(0xFF << yl);
            wr8(s, q, rd8(s, q) & keep);
            wr8(s, q + 1, rd8(s, q + 1) & keep);
        }
    }
    for (uint8_t c = 0; c < cols; c++) {
        uint16_t p = base + c * 2, src = sprite + c * 2;
        wr8(s, p, rd8(s, p) | shl8(rd8(s, src), yl));
        wr8(s, p + 1, rd8(s, p + 1) | shl8(rd8(s, src + 1), yl));
        if (has_lower) {
            uint16_t q = base + (c + dst_w) * 2;
            wr8(s, q, rd8(s, q) | shr8(rd8(s, src), 8 - yl));
            wr8(s, q + 1, rd8(s, q + 1) | shr8(rd8(s, src + 1), 8 - yl));
        }
    }
}
