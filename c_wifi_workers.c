#include <stdio.h>
// #include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
// #include "lwip/pbuf.h"
// #include "lwip/tcp.h"
#include "dhcpserver.h"
#include "dnsserver.h"
#include "httpserver.h"
#include "c_wifi_workers.h"

int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    cyw43_gpio_set(&cyw43_state, LED_GPIO, *((uint8_t *)user_data));
    return false; // Return false so it doesn't repeat
}
void blink_led(uint8_t n)
{
    if (n < 1)
    {
        return;
    }

    cyw43_gpio_set(&cyw43_state, LED_GPIO, true);
    for (uint8_t i = 1; i < 2 * n; i++)
    {
        uint8_t *next_bool = (uint8_t *)malloc(sizeof(uint8_t));
        *next_bool = !(i % 2);
        add_alarm_in_ms((LED_BLINK_DURATION_MS / ((2 * n) - 1))*i, alarm_callback, (void *)next_bool, false);
    }
}

constate_t conState = {.mode = MODE_OFF, .goal_mode = MODE_AP};
dhcp_server_t dhcp_server;
dns_server_t dns_server;
http_server_t http_server;
ip_addr_t ap_addr;

constate_t getMode()
{
    return conState;
}

void setMode(connection_mode_t newMode)
{
    if (newMode < MODE_OFF || newMode > MODE_WAN)
    {
        printf("Mode should be between %i and %i.\n", MODE_OFF, MODE_WAN);
        return;
    }
    if (newMode % 2)
    {
        printf("Mode should be an even number between %i and %i.\n", MODE_OFF, MODE_WAN);
        return;
    }
    conState.goal_mode = newMode;
    printf("Target mode set to %i. Current mode is still %i.\n", conState.goal_mode, conState.mode);
    blink_led(newMode);
}

void key_pressed_func(void *_)
{
    int key = getchar_timeout_us(0);
    DEBUG_printf(">%c\n", key);
    // if (key - '0' >= MODE_OFF && key - '0' <= (char)MODE_WAN)
    if (key >= MODE_OFF + '0' && key <= (char)MODE_WAN + '0')
    {
        setMode(key - '0');
    }
}

void check_and_update_mode()
{
    if (conState.goal_mode == conState.mode)
    {
        DEBUG_printf("Target mode achieved. Checking for failures...");
        printf("TODO\n");
        return;
    }

    DEBUG_printf("Target mode %i. Coming from mode %i.\n", conState.goal_mode, conState.mode);
    if (conState.mode == MODE_OFF && conState.goal_mode > MODE_OFF) // MODE_OFF -> MODE_OFF_AP_INT
    {
        // Turn on AP_mode
        // Turn on DHCP
        // Turn on DNS
        // Turn on HTTP

        const char *ap_name = "pico2w_workers";
        const char *password = "password";
        cyw43_arch_lwip_begin();
        cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);
        cyw43_arch_lwip_end();

        // Start the dhcp server
        ip4_addr_t mask;
        (ap_addr).addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
        (mask).addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);
        dhcp_server_init(&dhcp_server, &ap_addr, &mask);
        // Start the dns server
        dns_server_init(&dns_server, &ap_addr);

        // http server
        http_server.con_state_getter = getMode;
        http_server.mode_change_cb = setMode;
        http_server.ap_addr = ap_addr;
        http_server_init(&http_server);

        conState.mode = MODE_OFF_AP_INT;
        return;
    }
    else if (conState.mode == MODE_OFF_AP_INT && conState.goal_mode > MODE_OFF_AP_INT) // MODE_OFF_AP_INT -> MODE_AP
    {
        // Check AP_mode
        // Check DHCP
        // Check DNS
        // Check HTTP

        // TODO Check AP_mode
        // TODO Check DHCP
        // TODO Check DNS
        // TODO Check HTTP

        conState.mode = MODE_AP;
        return;
    }
    else if (conState.mode == MODE_AP && conState.goal_mode > MODE_AP) // MODE_AP -> MODE_AP_LAN_INT
    {
        // Turn on lwip STA_mode
        // Turn off DHCP
        // Turn off DNS
        // Update HTTP server if needed?

        cyw43_arch_lwip_begin();
        cyw43_arch_enable_sta_mode();
        // cyw43_wifi_join
        cyw43_arch_lwip_end();

        conState.mode = MODE_AP_LAN_INT;
        return;
    }
    else if (conState.mode == MODE_AP_LAN_INT && conState.goal_mode > MODE_AP_LAN_INT) // MODE_AP_LAN_INT -> MODE_LAN_VERIFY
    {
        // Turn on AP_mode
        // Turn on DHCP
        // Turn on DNS
        // Turn on HTTP

        dhcp_server_deinit(&dhcp_server);
        cyw43_arch_lwip_begin();
        cyw43_arch_disable_ap_mode();
        cyw43_arch_lwip_end();
        conState.mode = MODE_LAN_VERIFY;
        return;
    }
    else if (conState.mode == MODE_LAN_VERIFY && conState.goal_mode > MODE_LAN_VERIFY) // MODE_LAN_VERIFY -> MODE_LAN_CLEANUP_INT
    {
        // conState.mode = MODE_LAN_CLEANUP_INT;
        // return;
    }
    else if (conState.mode == MODE_LAN_CLEANUP_INT && conState.goal_mode > MODE_LAN_CLEANUP_INT) // MODE_LAN_CLEANUP_INT -> MODE_LAN
    {
        // conState.mode = MODE_LAN;
        // return;
    }
    else if (conState.mode == MODE_LAN && conState.goal_mode > MODE_LAN) // MODE_LAN -> MODE_LAN_WAN_INT
    {
        // conState.mode = MODE_LAN_WAN_INT;
        // return;
    }
    else if (conState.mode == MODE_LAN_WAN_INT && conState.goal_mode > MODE_LAN_WAN_INT) // MODE_LAN_WAN_INT -> MODE_WAN
    {
        // conState.mode = MODE_WAN;
        // return;
    }
    else if (conState.mode == MODE_WAN && conState.goal_mode < MODE_WAN) // MODE_WAN -> MODE_LAN_WAN_INT
    {
        // conState.mode = MODE_LAN_WAN_INT;
        // return;
    }
    else if (conState.mode == MODE_LAN_WAN_INT && conState.goal_mode < MODE_LAN_WAN_INT) // MODE_LAN_WAN_INT -> MODE_LAN
    {
        // conState.mode = MODE_LAN;
        // return;
    }
    else if (conState.mode == MODE_LAN && conState.goal_mode < MODE_LAN) // MODE_LAN -> MODE_LAN_CLEANUP_INT
    {
        // conState.mode = MODE_LAN_CLEANUP_INT;
        // return;
    }
    else if (conState.mode == MODE_LAN_CLEANUP_INT && conState.goal_mode < MODE_LAN_CLEANUP_INT) // MODE_LAN_CLEANUP_INT -> MODE_LAN_VERIFY
    {
        // conState.mode = MODE_LAN_VERIFY;
        // return;
    }
    else if (conState.mode == MODE_LAN_VERIFY && conState.goal_mode < MODE_LAN_VERIFY) // MODE_LAN_VERIFY -> MODE_AP_LAN_INT
    {
        conState.mode = MODE_AP_LAN_INT;
        return;
    }
    else if (conState.mode == MODE_AP_LAN_INT && conState.goal_mode < MODE_AP_LAN_INT) // MODE_AP_LAN_INT -> MODE_AP
    {
        conState.mode = MODE_AP;
        return;
    }
    else if (conState.mode == MODE_AP && conState.goal_mode < MODE_AP) // MODE_AP -> MODE_OFF_AP_INT
    {

        conState.mode = MODE_OFF_AP_INT;
        return;
    }
    else if (conState.mode == MODE_OFF_AP_INT && conState.goal_mode < MODE_OFF_AP_INT) // MODE_OFF_AP_INT -> MODE_OFF
    {
        http_server_deinit(&http_server);
        dns_server_deinit(&dns_server);
        dhcp_server_deinit(&dhcp_server);
        cyw43_arch_lwip_begin();
        cyw43_arch_disable_ap_mode();
        cyw43_arch_lwip_end();
        printf("Test complete\n");
        conState.mode = MODE_OFF;
        return;
    }
    else
    {
        printf("Invalid mode pair. {%i -> %i}", conState.mode, conState.goal_mode);
        return;
    }

    printf("TODO\n");
}

int main()
{
    stdio_init_all();

    printf("Initializing...");
    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
        return 1;
    }

    // Get notified if the user presses a key
    stdio_set_chars_available_callback(key_pressed_func, NULL);

    // while (conState.goal_mode != MODE_OFF || conState.mode != MODE_OFF)
    while (true)
    {
        printf("Hearthbeat ♥ %i -> %i\n", conState.mode, conState.goal_mode);
        check_and_update_mode();
        blink_led(conState.mode);
        sleep_ms(HEARTHBEAT_INTERVAL_MS);
    }
    cyw43_arch_deinit();
    printf("Shutting down...");
    return 0;
}
