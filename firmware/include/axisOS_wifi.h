/**
 * @file    axisOS_wifi.h
 * @brief   AxisOS Wi-Fi Manager API (CYW43 / lwIP)
 *
 * Copyright (c) 2026 AxisOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef AXISOS_WIFI_H
#define AXISOS_WIFI_H

#include <stdint.h>
#include <stdbool.h>
#include "axisOS_core.h"

/* -----------------------------------------------------------------------
 *  Configuration maximums
 * --------------------------------------------------------------------- */
#define WIFI_SSID_MAX_LEN       33u
#define WIFI_PASS_MAX_LEN       65u
#define WIFI_CONNECT_TIMEOUT_MS 15000u
#define WIFI_RETRY_COUNT        3u

/* -----------------------------------------------------------------------
 *  State machine
 * --------------------------------------------------------------------- */
typedef enum {
    WIFI_STATE_IDLE        = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_ERROR,
} wifi_state_t;

/* -----------------------------------------------------------------------
 *  Event callback
 * --------------------------------------------------------------------- */
typedef void (*wifi_event_cb_t)(wifi_state_t new_state, void *user_ctx);

/* -----------------------------------------------------------------------
 *  API
 * --------------------------------------------------------------------- */
axisOS_status_t wifi_init(void);
axisOS_status_t wifi_connect(const char *ssid, const char *password);
axisOS_status_t wifi_disconnect(void);
wifi_state_t    wifi_get_state(void);
int8_t          wifi_get_rssi(void);
void            wifi_poll(void);
void            wifi_register_event_cb(wifi_event_cb_t cb, void *user_ctx);

/* Returns heap-stable pointer to current IP string, NULL if not connected */
const char     *wifi_get_ip_str(void);

#endif /* AXISOS_WIFI_H */
