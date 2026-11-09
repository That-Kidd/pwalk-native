#include <string.h>
#include "pokewalker.h"
#include "rom_data.h"

uint8_t pw_rom_data[PW_ROM_DATA_END - PW_ROM_DATA_START + 1];
static int rom_loaded;

static const uint8_t expected_sha1[20] = {
    0x19, 0xa4, 0xc4, 0xfd, 0x64, 0xc7, 0x08, 0x7b, 0x2e, 0xb6,
    0x2a, 0x3a, 0x30, 0xcf, 0xbb, 0x88, 0xd2, 0xa1, 0xf7, 0x44,
};

static uint32_t rol(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

static void sha1_block(uint32_t h[5], const uint8_t *p) {
    uint32_t w[80];
    for (int i = 0; i < 16; i++) {
        w[i] = (uint32_t)p[i * 4] << 24 | (uint32_t)p[i * 4 + 1] << 16 | (uint32_t)p[i * 4 + 2] << 8 | p[i * 4 + 3];
    }
    for (int i = 16; i < 80; i++) {
        w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        uint32_t t = rol(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rol(b, 30);
        b = a;
        a = t;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
}

static void sha1(const uint8_t *data, size_t len, uint8_t out[20]) {
    uint32_t h[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
    size_t i = 0;
    for (; i + 64 <= len; i += 64) {
        sha1_block(h, data + i);
    }
    uint8_t tail[128] = { 0 };
    size_t rest = len - i;
    memcpy(tail, data + i, rest);
    tail[rest] = 0x80;
    size_t n = rest + 9 <= 64 ? 64 : 128;
    uint64_t bits = (uint64_t)len * 8;
    for (int j = 0; j < 8; j++) {
        tail[n - 1 - j] = (uint8_t)(bits >> (j * 8));
    }
    for (size_t j = 0; j < n; j += 64) {
        sha1_block(h, tail + j);
    }
    for (int j = 0; j < 20; j++) {
        out[j] = (uint8_t)(h[j / 4] >> (24 - (j % 4) * 8));
    }
}

int pw_rom_load(const void *rom, size_t size) {
    if (size != PW_ROM_SIZE) {
        return PW_ROM_BAD_SIZE;
    }
    uint8_t digest[20];
    sha1(rom, size, digest);
    if (memcmp(digest, expected_sha1, sizeof(digest)) != 0) {
        return PW_ROM_BAD_HASH;
    }
    memcpy(pw_rom_data, (const uint8_t *)rom + PW_ROM_DATA_START, sizeof(pw_rom_data));
    rom_loaded = 1;
    return PW_ROM_OK;
}

int pw_rom_loaded(void) {
    return rom_loaded;
}
