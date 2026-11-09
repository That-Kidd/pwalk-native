#include "pw_internal.h"

uint32_t mul32(uint32_t a, uint32_t b) {
    return a * b;
}

uint32_t divmod32(uint32_t num, uint32_t den, uint32_t *rem) {
    if (den <= 0xFFFF) {
        uint32_t q = num / den;
        *rem = num % den;
        return q;
    }
    uint32_t r = num >> 16;
    uint16_t q = num & 0xFFFF;
    for (int i = 0; i < 16; i++) {
        r = (r << 1) | (q >> 15);
        q <<= 1;
        if (r >= den) {
            r -= den;
            q |= 1;
        }
    }
    *rem = r;
    return q;
}

void setRandomSeed(pw_state_t *s, uint32_t seed) {
    wr32(s, RAM_RandomSeed, seed);
}

uint32_t rand32(pw_state_t *s) {
    uint32_t next = mul32(rd32(s, RAM_RandomSeed), 0x19660D) + 0x3C6EF35F;
    wr32(s, RAM_RandomSeed, next);
    return next;
}

void clampTotalStepToLimit(pw_state_t *s, uint32_t steps) {
    if (steps >= 9999999) {
        steps = 9999999;
    }
    wr32(s, RAM_RamCache_totalSteps, steps);
}

void memoryCopy(pw_state_t *s, uint16_t src, uint16_t dst, uint16_t len) {
    while (len--) {
        wr8(s, dst++, mem_rd8(s, src++));
    }
}

static uint32_t bin_weight(unsigned k) {
    return (uint32_t)(int32_t)(int16_t)(uint16_t)((k & 0xFF) << 9);
}

uint32_t spectrumPeakCentroid(pw_state_t *s, uint16_t mags, uint8_t idx) {
    uint32_t num;
    uint16_t den;
    if (idx == 0) {
        uint16_t a = rd16(s, mags), b = rd16(s, mags + 2);
        den = a + b;
        num = a * bin_weight(5) + b * bin_weight(6);
    } else if (idx == 9) {
        uint16_t a = rd16(s, mags + 0x10), b = rd16(s, mags + 0x12);
        den = a + b;
        num = (uint32_t)a * 0x1A00 + (uint32_t)b * 0x1C00;
    } else {
        uint16_t p = mags + idx * 2;
        uint16_t lo = rd16(s, p - 2), hi = rd16(s, p + 2), mid = rd16(s, p);
        den = lo + hi + mid;
        num = lo * bin_weight(idx + 4) + hi * bin_weight(idx + 6) + mid * bin_weight(idx + 5);
    }

    uint32_t rem;
    return den ? divmod32(num, den, &rem) : 0;
}

#define ROM_SINE_TABLE 0xBDD0

static int16_t rom_rd16s(uint16_t addr) {
    return (int16_t)(rom_rd8(addr) << 8 | rom_rd8(addr + 1));
}

static int16_t q11(int32_t a) {
    return (int16_t)((uint32_t)a << 5 >> 16);
}

void processAccelerometerSamplesAxis(pw_state_t *s, uint16_t samples) {
    setHeapPointer(s);
    uint16_t re = static_malloc(s, 0x80);
    uint16_t im = static_malloc(s, 0x80);
#define RE(i) (re + (uint16_t)(i) * 2)
#define IM(i) (im + (uint16_t)(i) * 2)
    for (int i = 0; i < 64; i++) {
        wr16(s, RE(i), (uint16_t)(int16_t)(int8_t)rd8(s, samples + i));
        wr16(s, IM(i), 0);
    }

    uint8_t j = 0;
    for (uint8_t i = 1; i < 0x3F; i++) {
        uint8_t m = 0x20;
        j ^= m;
        while (m > j) {
            m >>= 1;
            j ^= m;
        }
        if (i < j) {
            uint16_t t = rd16(s, RE(i));
            wr16(s, RE(i), rd16(s, RE(j)));
            wr16(s, RE(j), t);
        }
    }
    uint8_t stride = 0x40;
    for (uint8_t half = 1; (uint8_t)(half << 1) <= 0x40; half <<= 1) {
        uint8_t span = half << 1;
        stride >>= 1;
        uint8_t tw = 0;
        for (uint8_t k = 0; k < half; k++, tw += stride) {
            int16_t c = rom_rd16s(ROM_SINE_TABLE + (tw + 0x10) * 2);
            int16_t sn = rom_rd16s(ROM_SINE_TABLE + tw * 2);
            for (uint8_t a = k; a < 0x40; a += span) {
                uint8_t b = half + a;
                int16_t br = (int16_t)rd16(s, RE(b)), bi = (int16_t)rd16(s, IM(b));
                int16_t tr, ti;
                if (tw == 0) {
                    tr = br;
                    ti = bi;
                } else if (tw == 0x10) {
                    tr = (int16_t)-bi;
                    ti = br;
                } else if (bi == 0) {
                    tr = q11((int32_t)br * c);
                    ti = q11((int32_t)br * sn);
                } else {
                    tr = q11((int32_t)br * c - (int32_t)bi * sn);
                    ti = q11((int32_t)bi * c + (int32_t)br * sn);
                }
                wr16(s, RE(b), rd16(s, RE(a)) - tr);
                wr16(s, IM(b), rd16(s, IM(a)) - ti);
                wr16(s, RE(a), rd16(s, RE(a)) + tr);
                wr16(s, IM(a), rd16(s, IM(a)) + ti);
            }
        }
    }
    for (int i = 0; i < 32; i++) {
        int16_t r = (int16_t)rd16(s, RE(i)), m = (int16_t)rd16(s, IM(i));
        uint16_t mag = (uint16_t)(r < 0 ? -r : r) + (uint16_t)(m < 0 ? -m : m);
        uint16_t acc = RAM_sometimes_new_rxed_data_for_0xED + i * 2;
        wr16(s, acc, rd16(s, acc) + mag);
    }
#undef RE
#undef IM
}
