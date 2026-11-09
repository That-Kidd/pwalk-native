#include <stdio.h>
#include <string.h>
#include "frontend.h"
#ifndef _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#endif

void hal_ir_send(void *user, uint8_t byte) {
    frontend_t *f = user;
    if (f->ir_out_len < (int)sizeof(f->ir_out)) {
        f->ir_out[f->ir_out_len++] = byte;
    }
}

int hal_ir_recv(void *user) {
    frontend_t *f = user;
    if (f->ir_in_head == f->ir_in_len) {
        int n = (int)recv(f->ir_sock, (char *)f->ir_in, sizeof(f->ir_in), 0);
        if (n <= 0) {
            return -1;
        }
        f->ir_in_head = 0;
        f->ir_in_len = (int)n;
    }
    return f->ir_in[f->ir_in_head++];
}

void ir_flush(frontend_t *f) {
    if (f->ir_out_len) {
        sendto(f->ir_sock, (const char *)f->ir_out, f->ir_out_len, 0, (struct sockaddr *)&f->ir_peer, f->ir_peer_len);
        f->ir_out_len = 0;
    }
}

int ir_open(frontend_t *f, const char *spec) {
    char host[256];
    int lport, port;
    if (sscanf(spec, "%d:%255[^:]:%d", &lport, host, &port) != 3) {
        fprintf(stderr, "--ir wants LISTENPORT:HOST:PORT\n");
        return 0;
    }
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    struct addrinfo hints = { 0 }, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    char ps[16];
    snprintf(ps, sizeof(ps), "%d", port);
    if (getaddrinfo(host, ps, &hints, &res)) {
        fprintf(stderr, "can't resolve %s\n", host);
        return 0;
    }
    memcpy(&f->ir_peer, res->ai_addr, res->ai_addrlen);
    f->ir_peer_len = res->ai_addrlen;
    freeaddrinfo(res);
    f->ir_sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in me = { 0 };
    me.sin_family = AF_INET;
    me.sin_port = htons((uint16_t)lport);
    me.sin_addr.s_addr = htonl(INADDR_ANY);
#ifdef _WIN32
    if (f->ir_sock == INVALID_SOCKET || bind(f->ir_sock, (struct sockaddr *)&me, sizeof(me))) {
        fprintf(stderr, "can't listen on UDP port %d\n", lport);
        return 0;
    }
    u_long nb = 1;
    ioctlsocket(f->ir_sock, FIONBIO, &nb);
#else
    if (f->ir_sock < 0 || bind(f->ir_sock, (struct sockaddr *)&me, sizeof(me))) {
        fprintf(stderr, "can't listen on UDP port %d\n", lport);
        return 0;
    }
    fcntl(f->ir_sock, F_SETFL, O_NONBLOCK);
#endif
    f->ir_on = 1;
    return 1;
}
