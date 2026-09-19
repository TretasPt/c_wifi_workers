#ifndef CWIFIWORKERS
#define CWIFIWORKERS

#define HEARTHBEAT_INTERVAL_MS 5000
#define LED_BLINK_DURATION_MS 800
#define LED_GPIO 0
// Function to use for prints
#define DEBUG_printf printf

typedef enum connection_mode_t
{
    // --- LEVEL 0: SHUTDOWN / INITIALIZATION ---
    MODE_OFF = 0,        // Goal: System stopped / safe state.
    MODE_OFF_AP_INT = 1, // Intermediary: Initializing cyw43, turning on AP hardware.

    // --- LEVEL 1: CAPTIVE PORTAL ---
    MODE_AP = 2, // Goal: Stable Captive Portal. Handing out IPs via DHCP, DNS redirecting.

    // --- LEVEL 2: JOINING HOME NETWORK ---
    MODE_AP_LAN_INT = 3,      // Intermediary: Scanning and attempting async connection to home router.
    MODE_LAN_VERIFY = 4,      // Goal: Dual-Mode active (STA + AP). Waiting for user to ping STA IP to verify access.
    MODE_LAN_CLEANUP_INT = 5, // Intermediary: User verified! Shutting down DNS server and disabling AP radio.
    MODE_LAN = 6,             // Goal: Pure local IoT device. Running HTTP server strictly on home WiFi.

    // --- LEVEL 3: OPENING TO THE INTERNET ---
    MODE_LAN_WAN_INT = 7, // Intermediary: Sending UPnP discovery and SOAP port-forwarding requests.
    MODE_WAN = 8          // Goal: Exposed to web. UPnP active with keep-alive heartbeats running.

} connection_mode_t;

typedef struct
{
    int mode;
    int goal_mode;
} constate_t;

#endif