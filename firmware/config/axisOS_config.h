/**
 * @file    axisOS_config.h
 * @brief   AxisOS Runtime Configuration — edit this file to customise behaviour
 *
 * Copyright (c) 2026 AxisOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef AXISOS_CONFIG_H
#define AXISOS_CONFIG_H

/* -----------------------------------------------------------------------
 *  Wi-Fi credentials
 *  Set these before building or provision at runtime via axisOS_prov.h
 * --------------------------------------------------------------------- */
#define AXISOS_WIFI_SSID        "YOUR_SSID"
#define AXISOS_WIFI_PASSWORD    "YOUR_PASSWORD"
#define AXISOS_WIFI_COUNTRY     "US"       /* ISO 3166-1 alpha-2 */

/* -----------------------------------------------------------------------
 *  Logging
 * --------------------------------------------------------------------- */
#define AXISOS_LOG_LEVEL        LOG_LEVEL_INFO   /* DEBUG / INFO / WARN / ERROR */
#define AXISOS_LOG_TIMESTAMP    1                /* 1 = include ms tick */

/* -----------------------------------------------------------------------
 *  Task scheduler
 * --------------------------------------------------------------------- */
#define AXISOS_HEARTBEAT_MS     1000u    /* LED heartbeat period */
#define AXISOS_TELEMETRY_MS     5000u    /* Telemetry push period */
#define AXISOS_WIFI_WATCHDOG_MS 30000u   /* Reconnect if disconnected > 30 s */

/* -----------------------------------------------------------------------
 *  HTTP server (optional — set to 0 to disable)
 * --------------------------------------------------------------------- */
#define AXISOS_HTTP_ENABLE      1
#define AXISOS_HTTP_PORT        80
#define AXISOS_HTTP_MAX_CLIENTS 4

/* -----------------------------------------------------------------------
 *  OTA update server
 * --------------------------------------------------------------------- */
#define AXISOS_OTA_ENABLE       0        /* disabled by default in this build */
#define AXISOS_OTA_HOST         "ota.axisOS.dev"
#define AXISOS_OTA_PATH         "/firmware/pico2w/latest.bin"

/* -----------------------------------------------------------------------
 *  Board pin assignments (Pico 2W)
 * --------------------------------------------------------------------- */
#define PIN_SDA                 4u
#define PIN_SCL                 5u
#define PIN_SPI_SCK             18u
#define PIN_SPI_MOSI            19u
#define PIN_SPI_MISO            16u
#define PIN_SPI_CS              17u
#define PIN_USER_LED            2u     /* Optional external LED */
#define PIN_USER_BTN            3u     /* Optional user button */

#define ADC_CHANNEL_VBAT        0u     /* Battery voltage divider on GPIO26 */

#endif /* AXISOS_CONFIG_H */
