
#include "httpserver.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

static const int MAX_RESPONSE_SIZE = 1024;
static mode_change_cb_t global_mode_setter = NULL;
static con_state_getter_t global_mode_getter = NULL;
static ip_addr_t ap_addr;
const char local_url[] = "tretaspt.pico";
const char *http_body =
    "<!doctype html>\r\n"
    "<html>\r\n"
    "   <head>"
    "       <style>"
    "           html{height:100%;}"
    "           body{height:100%;margin:0;background-color:cadetblue;color:white;display:flex;justify-content:center;align-items:center;}"
    "       </style>"
    "       <script></script>"
    "   </head>"
    "   <body>"
    "       <h1>This is not the site you're looking for...</h1>"
    "   </body>\r\n"
    "</html>\r\n";

static void http_routing(char *response, int *len, char *method, char *target, struct pbuf *p)
{
    int firstLineEnd = pbuf_strstr(p, "\r\n");
    int headersEnd = pbuf_strstr(p, "\r\n\r\n");
    int hostStart = 0xFFFF;
    int hostEnd = 0xFFFF;
    char host[50];
    host[0] = '\0';
    if (firstLineEnd != 0xFFFF && headersEnd != 0xFFFF && headersEnd > firstLineEnd)
    {
        hostStart = pbuf_memfind(p, "\r\nHost: ", 8, firstLineEnd - 1);
        if (hostStart != 0xFFFF)
        {
            hostStart += 8;
            hostEnd = pbuf_memfind(p, "\r\n", 2, hostStart);
            if (hostEnd != 0xFFFF && hostEnd - hostStart < 50)
            {
                pbuf_copy_partial(p, host, hostEnd - hostStart, hostStart);
                host[hostEnd - hostStart] = '\0';
            }
        }
    }

    if (strcasecmp("GET", method) == 0)
    {
        if (strcasecmp(local_url, host) != 0) // Redirect needed?
        {
            *len = snprintf(response, MAX_RESPONSE_SIZE * sizeof(char),
                            "HTTP/1.1 302 Found\r\n"
                            "Location: http://%s/\r\n"
                            "Connection: close\r\n"
                            "Content-Length: 0\r\n"
                            "\r\n",
                            local_url);
        }
        else
        {
            // const char *http_body =
            //     "<!doctype html>\r\n"
            //     "<html>\r\n"
            //     "<body><h1>This is not the site you're looking for...</h1></body>\r\n"
            //     "</html>\r\n";

            const char *http_header =
                "HTTP/1.1 200 Replaced\r\n"
                "Connection: close\r\n"
                "Content-Length: %d\r\n"
                "\r\n";

            *len = snprintf(response, MAX_RESPONSE_SIZE * sizeof(char),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            "Connection: close\r\n"
                            "Content-Length: %d\r\n"
                            "\r\n"
                            "%s",
                            (int)strlen(http_body), http_body);
        }
    }
    // else if (strcasecmp("POST", method) == 0)
    // {
    // }
    else
    {
        printf("Other method: \"%s\".\n", method);
        // 501 Not implemented.
        *len = snprintf(response, MAX_RESPONSE_SIZE * sizeof(char),
                        "HTTP/1.1 501 Not Implemented\r\n"
                        "Connection: close\r\n"
                        "Content-Length: 0\r\n"
                        "\r\n");
    }
}

static void http_response_handler(struct tcp_pcb *tpcb, struct pbuf *p)
{
    char response[MAX_RESPONSE_SIZE];
    int response_len;

    // printf("\n\n\n\nhttp request recieved with length %d \nHTTP_START\n\n", p->tot_len);
    // struct pbuf *partial;
    // for (partial = p; partial != NULL; partial = partial->next)
    // {
    //     for (uint16_t i = 0; i < partial->len; i++)
    //     {
    //         putchar(((char *)partial->payload)[i]);
    //     }
    // }
    // printf("\n\nHTTP_END\n");

    int methodEnd = pbuf_strstr(p, " ");
    int targetEnd = pbuf_memfind(p, " ", 1, methodEnd + 1);
    char method[20];
    char target[200];

    if (methodEnd == 0xFFFF || methodEnd + 1 >= 20)
    {
        goto onError;
    }
    else
    {
        pbuf_copy_partial(p, method, methodEnd, 0);
        method[methodEnd] = '\0';
    }

    if (targetEnd == 0xFFFF || targetEnd + 1 - methodEnd + 1 >= 200)
    {
        goto onError;
    }
    else
    {
        pbuf_copy_partial(p, target, targetEnd - methodEnd - 1, methodEnd + 1);
        target[targetEnd - methodEnd - 1] = '\0';
    }

    // printf("Method = \"%s\"\nTarget = \"%s\"\n", method, target);

    http_routing(response, &response_len, method, target, p);
    // printf("Sending response:\n%s\n", response);
    if (tcp_write(tpcb, response, response_len, TCP_WRITE_FLAG_COPY) == ERR_OK)
    {
        tcp_output(tpcb);
        // printf("Output sent.\n");
    }
    return;
onError:
    printf("Error handling http request.\n");
}

static err_t http_request_handler(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    // printf("http_recieve\n");
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
    // printf("http_accept\n");
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
