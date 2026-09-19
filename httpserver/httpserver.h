#ifndef HTTPSERVER
#define HTTPSERVER

#include "c_wifi_workers.h"
#include "lwip/ip_addr.h"

typedef void (*mode_change_cb_t)(connection_mode_t new_mode);
typedef constate_t (*con_state_getter_t)();

typedef struct http_server_t
{
    struct tcp_pcb* tcp_pcb;
    mode_change_cb_t mode_change_cb;
    con_state_getter_t con_state_getter;
    ip_addr_t ap_addr;
} http_server_t;

// Initialize HTTP server and register state machine callback
void http_server_init(http_server_t *s);
void http_server_deinit(http_server_t *s);

#endif
