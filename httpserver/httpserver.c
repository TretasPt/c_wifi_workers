
#include "httpserver.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

static mode_change_cb_t global_mode_setter = NULL;
static con_state_getter_t global_mode_getter = NULL;
static ip_addr_t ap_addr;

static void http_response_handler(struct tcp_pcb *tpcb, struct pbuf *p)
{
    printf("http request recieved with length %d \nHTTP_START\n\n", p->tot_len);
    struct pbuf *partial;
    for (partial = p; partial != NULL; partial = partial->next)
    {
        for (uint16_t i = 0; i < partial->len; i++)
        {
            putchar(((char *)partial->payload)[i]);
        }
    }
    printf("\n\nHTTP_END\n");

    char response[256];
    int len;
    if (false)
    {
        // TODO: Address redirect loop...
        len = snprintf(response, sizeof(response),
                       "HTTP/1.1 302 Found\r\n"
                       "Location: http://%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "/\r\n"
                       "Connection: close\r\n"
                       "Content-Length: 0\r\n"
                       "\r\n",
                       ip4_addr1_val(ap_addr), ip4_addr2_val(ap_addr), ip4_addr3_val(ap_addr), ip4_addr4_val(ap_addr));
    }
    else
    {
        const char *http_body =
            "<!doctype html>\r\n"
            "<html>\r\n"
            "<body><h1>This is not the site you're looking for...</h1></body>\r\n"
            "</html>\r\n";

        const char *http_header =
            "HTTP/1.1 200 Replaced\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "\r\n";

        len = snprintf(response, sizeof(response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n"
                           "Connection: close\r\n"
                           "Content-Length: %d\r\n"
                           "\r\n"
                           "%s",
                           (int)strlen(http_body), http_body);
    }

    if (tcp_write(tpcb, response, len, TCP_WRITE_FLAG_COPY) == ERR_OK)
    {
        tcp_output(tpcb);
    }
}

static err_t http_request_handler(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    printf("http_recieve\n");
    if (p == NULL) // Client closed connection
    {
        tcp_close(tpcb);
        return ERR_OK;
    }

    http_response_handler(tpcb, p);

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    return ERR_OK;
}

err_t http_accept_handler(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    printf("http_accept\n");
    if (err != ERR_OK || newpcb == NULL)
    {
        return ERR_VAL;
    }
    tcp_recv(newpcb, http_request_handler);
    return ERR_OK;
}

void http_server_init(http_server_t *s)
{
    printf("http_innit\n");
    global_mode_setter = s->mode_change_cb;
    global_mode_getter = s->con_state_getter;
    ap_addr = s->ap_addr;

    cyw43_arch_lwip_begin();
    struct tcp_pcb *tcp_pcb = tcp_new();
    cyw43_arch_lwip_end();
    if (!tcp_pcb) // If out of memory it doesn't create the tcp_pcb
    {
        return;
    }

    cyw43_arch_lwip_begin();
    if (tcp_bind(tcp_pcb, IP4_ADDR_ANY, 80) == ERR_OK)
    {
        s->tcp_pcb = tcp_listen(tcp_pcb);
        printf("http_bind\n");
        tcp_accept(s->tcp_pcb, http_accept_handler);
    }
    else
    {
        printf("http_bind-----------------------FAIL\n");
        tcp_abort(tcp_pcb);
    }
    cyw43_arch_lwip_end();
}

void http_server_deinit(http_server_t *s)
{
    if (s && s->tcp_pcb)
    {
        tcp_close(s->tcp_pcb);
        s->tcp_pcb = NULL; // close frees the memory.
    }
}
