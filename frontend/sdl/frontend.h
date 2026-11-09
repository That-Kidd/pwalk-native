#pragma once
#include <stddef.h>
#include <stdint.h>
#include <SDL.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET sock_t;
#else
#include <sys/socket.h>
typedef int sock_t;
#endif
#include "pokewalker.h"

#define EEPROM_SIZE (1 << 16)

typedef struct frontend {
    pw_state_t walker;
    uint8_t eeprom[EEPROM_SIZE];
    uint8_t buttons;
    int walking;
    uint8_t screen[PW_SCREEN_HEIGHT][PW_SCREEN_WIDTH];

    SDL_AudioDeviceID audio;
    volatile uint32_t freq;
    volatile uint8_t volume;
    double phase;

    int ir_on;
    sock_t ir_sock;
    struct sockaddr_storage ir_peer;
    socklen_t ir_peer_len;
    uint8_t ir_out[1024], ir_in[4096];
    int ir_out_len, ir_in_head, ir_in_len;
} frontend_t;

/* hal.c */
void frontend_hal_init(frontend_t *f, pw_hal_t *hal);
void audio_callback(void *user, Uint8 *stream, int len);

/* ir_udp.c: the walker's IR link over UDP */
int ir_open(frontend_t *f, const char *spec);
void ir_flush(frontend_t *f);
void hal_ir_send(void *user, uint8_t byte);
int hal_ir_recv(void *user);

/* files.c: EEPROM, save states, the ROM and file dialogs */
void load_eeprom(frontend_t *f, const char *path);
void save_eeprom(frontend_t *f, const char *path);
void save_pgm(frontend_t *f, const char *path);
int load_state(frontend_t *f, const char *path, const pw_hal_t *hal);
void save_state(frontend_t *f, const char *path);
void start(frontend_t *f, const char *path, const pw_hal_t *hal);
const char *pick_file(const char *filter, const char *title);
void show_error(const char *msg);
int install_rom(const char *given);

/* headless.c: windowless runs driven by a key script, for testing */
void run_headless(frontend_t *f, double seconds, const char *script, const char *shot);
