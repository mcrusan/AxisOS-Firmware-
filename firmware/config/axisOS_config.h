/*
 * ============================================================================
 * File:        firmware/config/axisOS_config.h
 * Project:     Crusan & Co. // AxisOS — Sovereign Pico 2 W Firmware
 * Author:      Marc F. Crusan / Crusan & Co. w/ AGY (AXEL)
 * Date:        2026-10-30
 * Version:     v1.7.0-Silicon
 * Description: AxisOS runtime configuration header for Raspberry Pi Pico 2 W
 *              (RP2350) with Sovereign Seven & COME-AND-GET-IT defaults.
 * Changelog:
 *   - 2026-10-30 (MFC/AGY): Replaced placeholder SSID with Sovereign uplink
 *     defaults and aligned watchdog/Wi-Fi timeouts.
 * ============================================================================
 */

#ifndef AXISOS_CONFIG_H
#define AXISOS_CONFIG_H

/* -----------------------------------------------------------------------
 *  Wi-Fi credentials
 *  Override via compiler flags or runtime NVS provisioning
 * --------------------------------------------------------------------- */
#ifndef AXISOS_WIFI_SSID
#define AXISOS_WIFI_SSID        "Starlink-WiFi-2.4"
#endif
#ifndef AXISOS_WIFI_PASSWORD
#define AXISOS_WIFI_PASSWORD    ""
#endif
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
 *  HTTP server & Sovereign Seven (:80, :8080-:8084)
 * --------------------------------------------------------------------- */
#define AXISOS_HTTP_ENABLE      1
#define AXISOS_HTTP_PORT        80
#define AXISOS_HTTP_MAX_CLIENTS 4

/* -----------------------------------------------------------------------
 *  COME-AND-GET-IT OTA update readiness
 * --------------------------------------------------------------------- */
#define AXISOS_OTA_ENABLE       1
#define AXISOS_OTA_PORT         50000u
#define AXISOS_OTA_HOST         "192.168.1.84"
#define AXISOS_OTA_PATH         "/firmware/pico2w/latest.uf2"

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
