/*
 * ============================================================================
 * File:        firmware/src/axisOS_wifi.c
 * Project:     Crusan & Co. // AxisOS — Sovereign Pico 2 W Firmware
 * Author:      Marc F. Crusan / Crusan & Co. w/ AGY (AXEL)
 * Date:        2026-10-30
 * Version:     v1.7.0-Silicon
 * Description: AxisOS Wi-Fi Manager implementation (CYW43 / lwIP) with
 *              watchdog-safe connection handling.
 * Changelog:
 *   - 2026-10-30 (MFC/AGY): Added hardware watchdog refresh around CYW43
 *     Wi-Fi connect attempts so slow handshakes never trigger WDT resets.
 * ============================================================================
 */

#include <string.h>
#include <stdio.h>
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
    if (!ssid || strlen(ssid) == 0) return AXISOS_ERR_PARAM;

    LOG_I(TAG, "Connecting to SSID: %s ...", ssid);
    set_state(WIFI_STATE_CONNECTING);

    watchdog_update();
    uint32_t auth = (password && strlen(password) > 0) ? CYW43_AUTH_WPA2_AES_PSK : CYW43_AUTH_OPEN;
    int rc = cyw43_arch_wifi_connect_timeout_ms(ssid, password, auth, WIFI_CONNECT_TIMEOUT_MS);
    watchdog_update();

    if (rc != 0) {
        LOG_E(TAG, "Connection failed (err=%d).", rc);
        set_state(WIFI_STATE_ERROR);
        return AXISOS_ERR_IO;
    }

    /* Cache IP */
    uint32_t ip = cyw43_state.netif[0].ip_addr.addr;
    snprintf(s_ip_buf, sizeof(s_ip_buf), "%lu.%lu.%lu.%lu",
             (unsigned long)((ip)       & 0xFF),
             (unsigned long)((ip >> 8)  & 0xFF),
             (unsigned long)((ip >> 16) & 0xFF),
             (unsigned long)((ip >> 24) & 0xFF));

    LOG_I(TAG, "Connected. IP: %s  RSSI: %d dBm", s_ip_buf, wifi_get_rssi());
    set_state(WIFI_STATE_CONNECTED);
    return AXISOS_OK;
}

axisOS_status_t wifi_disconnect(void) {
    cyw43_arch_disable_sta_mode();
    memset(s_ip_buf, 0, sizeof(s_ip_buf));
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
