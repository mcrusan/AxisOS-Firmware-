// -----------------------------------------------------------------------------
// File:        firmware/src/axisOS_wifi.c
// Description: AxisOS Wi-Fi Manager implementation (CYW43 / lwIP) with watchdog-safe connection handling.
// Author:      Marc F. Crusan (MFC) / Crusan & Co. w/ PP & AGY (AXEL)
// Date:        2026112302
// Version:     v1.8.3-Silicon
// -----------------------------------------------------------------------------
// 2026112302 (MFC/AGY) Eliminated <stdio.h>; replaced IPv4 snprintf with pure freestanding axis_fmt_ipv4().
// 2026112301 (MFC/AGY) Replaced <string.h> with Marc's freestanding axis-string.h (axis_strlen / axis_memset).
// 2026110901 (MFC/PP/AGY) Upgraded to canonical Crusan & Co. 5-Field SOP Header standard.

#include "axis-string.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"
#include "axisOS_wifi.h"
#include "axisOS_core.h"

#define TAG "WIFI"

/* -----------------------------------------------------------------------
 *  Private state
 * --------------------------------------------------------------------- */
static wifi_state_t    s_state     = WIFI_STATE_IDLE;
static wifi_event_cb_t s_event_cb  = NULL;
static void           *s_event_ctx = NULL;
static char            s_ip_buf[16] = {0};

/* -----------------------------------------------------------------------
 *  Helpers
 * --------------------------------------------------------------------- */
static void axis_fmt_ipv4(char *out, uint32_t ip) {
    char *p = out;
    for (int oct = 0; oct < 4; oct++) {
        uint8_t v = (uint8_t)((ip >> (oct * 8)) & 0xFFu);
        if (v >= 100u) *p++ = (char)('0' + (v / 100u));
        if (v >= 10u)  *p++ = (char)('0' + ((v / 10u) % 10u));
        *p++ = (char)('0' + (v % 10u));
        if (oct < 3) *p++ = '.';
    }
    *p = '\0';
}

static void set_state(wifi_state_t new_state) {
    if (s_state == new_state) return;
    s_state = new_state;
    if (s_event_cb) s_event_cb(new_state, s_event_ctx);
}

/* -----------------------------------------------------------------------
 *  Public API
 * --------------------------------------------------------------------- */
axisOS_status_t wifi_init(void) {
    cyw43_arch_enable_sta_mode();
    LOG_I(TAG, "Wi-Fi STA mode enabled.");
    set_state(WIFI_STATE_IDLE);
    return AXISOS_OK;
}

axisOS_status_t wifi_connect(const char *ssid, const char *password) {
    if (!ssid || axis_strlen(ssid) == 0) return AXISOS_ERR_PARAM;

    LOG_I(TAG, "Connecting to SSID: %s ...", ssid);
    set_state(WIFI_STATE_CONNECTING);

    watchdog_update();
    uint32_t auth = (password && axis_strlen(password) > 0) ? CYW43_AUTH_WPA2_AES_PSK : CYW43_AUTH_OPEN;
    int rc = cyw43_arch_wifi_connect_timeout_ms(ssid, password, auth, WIFI_CONNECT_TIMEOUT_MS);
    watchdog_update();

    if (rc != 0) {
        LOG_E(TAG, "Connection failed (err=%d).", rc);
        set_state(WIFI_STATE_ERROR);
        return AXISOS_ERR_IO;
    }

    /* Cache IP */
    uint32_t ip = cyw43_state.netif[0].ip_addr.addr;
    axis_fmt_ipv4(s_ip_buf, ip);

    LOG_I(TAG, "Connected. IP: %s  RSSI: %d dBm", s_ip_buf, wifi_get_rssi());
    set_state(WIFI_STATE_CONNECTED);
    return AXISOS_OK;
}

axisOS_status_t wifi_disconnect(void) {
    cyw43_arch_disable_sta_mode();
    axis_memset(s_ip_buf, 0, sizeof(s_ip_buf));
    set_state(WIFI_STATE_DISCONNECTED);
    LOG_I(TAG, "Disconnected.");
    return AXISOS_OK;
}

wifi_state_t wifi_get_state(void) { return s_state; }

int8_t wifi_get_rssi(void) {
    int32_t rssi = 0;
    cyw43_wifi_get_rssi(&cyw43_state, &rssi);
    return (int8_t)rssi;
}

const char *wifi_get_ip_str(void) {
    return (s_state == WIFI_STATE_CONNECTED) ? s_ip_buf : NULL;
}

void wifi_poll(void) {
    cyw43_arch_poll();
}

void wifi_register_event_cb(wifi_event_cb_t cb, void *user_ctx) {
    s_event_cb  = cb;
    s_event_ctx = user_ctx;
}
