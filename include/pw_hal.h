#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PW_SCREEN_WIDTH  96
#define PW_SCREEN_HEIGHT 64

#define PW_BUTTON_ENTER (1 << 0)
#define PW_BUTTON_LEFT  (1 << 2)
#define PW_BUTTON_RIGHT (1 << 4)

typedef struct pw_hal {
    void *user;

    void (*present)(void *user, const uint8_t pixels[PW_SCREEN_HEIGHT][PW_SCREEN_WIDTH]);

    uint8_t (*buttons)(void *user);

    void (*eeprom_read)(void *user, uint16_t addr, void *dst, uint16_t len);
    void (*eeprom_write)(void *user, uint16_t addr, const void *src, uint16_t len);

    void (*accel_sample)(void *user, int8_t *x, int8_t *y, int8_t *z);

    void (*beep)(void *user, uint32_t freq_hz, uint8_t volume);

    void (*ir_send)(void *user, uint8_t byte);

    int (*ir_recv)(void *user);

    uint8_t (*battery)(void *user);
} pw_hal_t;

#ifdef __cplusplus
}
#endif
