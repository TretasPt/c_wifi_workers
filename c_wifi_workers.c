#include <stdio.h>
// #include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
// #include "lwip/pbuf.h"
// #include "lwip/tcp.h"
#include "dhcpserver.h"
// #include "dnsserver.h"

#define HEARTHBEAT_INTERVAL_MS 2000
#define LED_BLINK_DURATION_MS 100
#define LED_GPIO 0
// Function to use for prints
#define DEBUG_printf printf


// --- LEVEL 0: SHUTDOWN / INITIALIZATION ---
#define MODE_OFF              0  // Goal: System stopped / safe state.
#define MODE_OFF_AP_INT       1  // Intermediary: Initializing cyw43, turning on AP hardware.

// --- LEVEL 1: CAPTIVE PORTAL ---
#define MODE_AP               2  // Goal: Stable Captive Portal. Handing out IPs via DHCP, DNS redirecting.

// --- LEVEL 2: JOINING HOME NETWORK ---
#define MODE_AP_LAN_INT       3  // Intermediary: Scanning and attempting async connection to home router.
#define MODE_LAN_VERIFY       4  // Goal: Dual-Mode active (STA + AP). Waiting for user to ping STA IP to verify access.
#define MODE_LAN_CLEANUP_INT  5  // Intermediary: User verified! Shutting down DNS server and disabling AP radio.
#define MODE_LAN              6  // Goal: Pure local IoT device. Running HTTP server strictly on home WiFi.

// --- LEVEL 3: OPENING TO THE INTERNET ---
#define MODE_LAN_WAN_INT      7  // Intermediary: Sending UPnP discovery and SOAP port-forwarding requests.
#define MODE_WAN              8  // Goal: Exposed to web. UPnP active with keep-alive heartbeats running.

int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    cyw43_gpio_set(&cyw43_state, LED_GPIO, false);
    return false; // Return false so it doesn't repeat
}
void blink_led()
{
    cyw43_gpio_set(&cyw43_state, LED_GPIO, true);
    add_alarm_in_ms(LED_BLINK_DURATION_MS, alarm_callback, NULL, false);
}

struct
{
    int mode;
    int goal_mode;
} conState = {.mode = MODE_OFF, .goal_mode = MODE_AP};
dhcp_server_t dhcp_server;
ip_addr_t ap_addr;

void setMode(int newMode)
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
    blink_led();
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
    if (conState.mode == MODE_OFF && conState.goal_mode > MODE_OFF)//MODE_OFF -> MODE_OFF_AP_INT
    {
        //Turn on AP_mode
        //Turn on DHCP
        //Turn on DNS
        //Turn on HTTP

        const char *ap_name = "picow_test";
        const char *password = "password";
        cyw43_arch_lwip_begin();
        cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);
        cyw43_arch_lwip_end();

        // Start the dhcp server
        ip4_addr_t mask;
        (ap_addr).addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
        (mask).addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);
        dhcp_server_init(&dhcp_server, &ap_addr, &mask);

        conState.mode = MODE_OFF_AP_INT;
        return;
    }
    else if (conState.mode == MODE_OFF_AP_INT && conState.goal_mode > MODE_OFF_AP_INT)//MODE_OFF_AP_INT -> MODE_AP
    {
        //Check AP_mode
        //Check DHCP
        //Check DNS
        //Check HTTP

        conState.mode = MODE_AP;
        return;
    }
    else if (conState.mode == MODE_AP && conState.goal_mode > MODE_AP)//MODE_AP -> MODE_AP_LAN_INT
    {
        //Does nothing

        // conState.mode = MODE_AP_MIXED_INT;
        // return;
    }
    else if (conState.mode == MODE_AP_LAN_INT && conState.goal_mode > MODE_AP_LAN_INT)//MODE_AP_LAN_INT -> MODE_LAN_VERIFY
    {
        //Turn on AP_mode
        //Turn on DHCP
        //Turn on DNS
        //Turn on HTTP
        
        dhcp_server_deinit(&dhcp_server);
        cyw43_arch_lwip_begin();
        cyw43_arch_disable_ap_mode();
        cyw43_arch_lwip_end();
        // conState.mode = MODE_AP_MIXED_INT;
        // return;
    }
    else if (conState.mode == MODE_LAN_VERIFY && conState.goal_mode > MODE_LAN_VERIFY)//MODE_LAN_VERIFY -> MODE_LAN_CLEANUP_INT
    {
    }
    else if (conState.mode == MODE_LAN_CLEANUP_INT && conState.goal_mode > MODE_LAN_CLEANUP_INT)//MODE_LAN_CLEANUP_INT -> MODE_LAN
    {
    }
    else if (conState.mode == MODE_LAN && conState.goal_mode > MODE_LAN)//MODE_LAN -> MODE_LAN_WAN_INT
    {
    }
    else if (conState.mode == MODE_LAN_WAN_INT && conState.goal_mode > MODE_LAN_WAN_INT)//MODE_LAN_WAN_INT -> MODE_WAN
    {
    }
    else if (conState.mode == MODE_WAN && conState.goal_mode < MODE_WAN)//MODE_WAN -> MODE_LAN_WAN_INT
    {
    }
    else if (conState.mode == MODE_LAN_WAN_INT && conState.goal_mode < MODE_LAN_WAN_INT)//MODE_LAN_WAN_INT -> MODE_LAN
    {
    }
    else if (conState.mode == MODE_LAN && conState.goal_mode < MODE_LAN)//MODE_LAN -> MODE_LAN_CLEANUP_INT
    {
    }
    else if (conState.mode == MODE_LAN_CLEANUP_INT && conState.goal_mode < MODE_LAN_CLEANUP_INT)//MODE_LAN_CLEANUP_INT -> MODE_LAN_VERIFY
    {
    }
    else if (conState.mode == MODE_LAN_VERIFY && conState.goal_mode < MODE_LAN_VERIFY)//MODE_LAN_VERIFY -> MODE_AP_LAN_INT
    {
    }
    else if (conState.mode == MODE_AP_LAN_INT && conState.goal_mode < MODE_AP_LAN_INT)//MODE_AP_LAN_INT -> MODE_AP
    {
    }
    else if (conState.mode == MODE_AP && conState.goal_mode < MODE_AP)//MODE_AP -> MODE_OFF_AP_INT
    {
    }
    else if (conState.mode == MODE_OFF_AP_INT && conState.goal_mode < MODE_OFF_AP_INT)//MODE_OFF_AP_INT -> MODE_OFF
    {
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
        printf("Invalid mode.");
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

    while (conState.goal_mode != MODE_OFF || conState.mode != MODE_OFF)
    {
        printf("Hearthbeat ♥ %i -> %i\n", conState.mode, conState.goal_mode);
        check_and_update_mode();
        blink_led();
        sleep_ms(HEARTHBEAT_INTERVAL_MS);
    }
    cyw43_arch_deinit();
    printf("Shutting down...");
    return 0;
}

//     // Start the dns server
//     dns_server_t dns_server;
//     dns_server_init(&dns_server, &state->gw);
//     dns_server_deinit(&dns_server);
