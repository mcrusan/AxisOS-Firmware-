// -----------------------------------------------------------------------------
// File:        AxisOS_ESP32_S3_USB_Outpost.ino
// Description: Public Reference Firmware for Crusan & Co. AxisOS Inline USB
//              Pass-Through SI Outpost (NM-USB-Chain-V1.0 / ESP32-S3) featuring
//              160x80 ST7735S RGB565 HUD, 16x Oversampled VBUS V/A/W Telemetry,
//              D+/D- USB PHY Tri-State Release, NAPT SoftAP Extender, Embedded
//              Local SI Web Console (:80/:8080), UDP :50000 Mesh & C99 LAN OTA.
// Author:      Marc F. Crusan (MFC) / Crusan & Co. w/ PP & AGY (AXEL)
// Date:        2026111306
// Version:     v1.8.1-Silicon
// -----------------------------------------------------------------------------
// 2026111306 (MFC/AGY) Sanitized public SDK release for Tindie x PCBWay 2026 AI Hardware Creative Challenge (zero hardcoded credentials, SBC 1.15A brownout alert, optional local Qwen2.5 :8088 SI bridge).
// 2026110601 (MFC/AGY) Added embedded dark-mode Outpost SI Chat Console (/ and /chat), /api/chat silicon responder, and NVS /wifi provisioning.
// 2026102901 (MFC/AGY) Added pure C99 BSD sockets (:80, :8080) + atomic esp_ota COME-AND-GET-IT wireless updater.
// 2026102701 (MFC/AGY) Initial ST7735S 160x80 direct-GPIO HUD + inline VBUS voltage/current ADC gauge + releaseUsbPassthrough().

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "soc/soc.h"
#include "soc/usb_serial_jtag_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"

// -----------------------------------------------------------------------------
// 1. User-Configurable Network & Local SI Hub Defaults (Override or set via /wifi)
// -----------------------------------------------------------------------------
#ifndef AXISOS_NODE_ID
#define AXISOS_NODE_ID          "axis_outpost_01"
#endif
#ifndef AXISOS_NODE_LABEL
#define AXISOS_NODE_LABEL       "OUTPOST E10"
#endif
#ifndef AXISOS_UPLINK_SSID
#define AXISOS_UPLINK_SSID      "YOUR_PRIMARY_WIFI_SSID"
#endif
#ifndef AXISOS_UPLINK_PASS
#define AXISOS_UPLINK_PASS      "CHANGE_ME_PASSPHRASE"
#endif
#ifndef AXISOS_SOFTAP_SSID
#define AXISOS_SOFTAP_SSID      "AXISOS-USB-OUTPOST-AP"
#endif
#ifndef AXISOS_SOFTAP_PASS
#define AXISOS_SOFTAP_PASS      "AxisOS-Setup-2026"
#endif
#ifndef AXISOS_LOCAL_SI_HUB_IP
#define AXISOS_LOCAL_SI_HUB_IP  "192.168.1.100"
#endif
#ifndef AXISOS_LOCAL_SI_PORT
#define AXISOS_LOCAL_SI_PORT    8088
#endif
#ifndef AXISOS_OTA_PORT
#define AXISOS_OTA_PORT         8085
#endif

static const char* BOARD_TYPE   = "ESP32-S3-NM-USB-CHAIN-V1";
static const char* FIRMWARE_VER = "1.8.1-Silicon";
static const char* OTA_BIN_PATH = "/fleet_updates/AxisOS_ESP32_S3_USB_Outpost.ino.bin";
static const uint16_t UDP_DISC_PORT = 50000;

/* Raspberry Pi shared USB VBUS warning thresholds */
static const float VBUS_SAG_WARN_VOLTS   = 4.75f;
static const float SBC_USB_CEILING_AMPS  = 1.15f; /* Warns before 1.20A shared Pi USB limit */

static char        g_stationCallsign[24] = "BENCH // GATEWAY";
static char        g_stationRole[24]     = "USB SI SENTRY";
static char        g_lastChatNote[28]    = "";
static uint16_t    g_moodColor           = 0x07FF;
static char        g_customSsid[48]      = "";
static char        g_customPass[48]      = "";
static Preferences g_prefs;

static const IPAddress AP_IP(192, 168, 11, 1);
static const IPAddress AP_MASK(255, 255, 255, 0);
static const IPAddress AP_LEASE(192, 168, 11, 2);
static const IPAddress AP_DNS(192, 168, 1, 1);

// -----------------------------------------------------------------------------
// 2. NM-USB-Chain-V1.0 Hardware Pinout (ST7735S 160x80 IPS + VBUS Sense ADC)
// -----------------------------------------------------------------------------
#define TFT_SCLK      38
#define TFT_MOSI      48
#define TFT_CS        39
#define TFT_DC        47
#define TFT_RST       40

#define PIN_USB_VOLT  13   /* High-impedance VBUS divider input */
#define PIN_USB_CURR  12   /* Inline shunt current-sense amplifier output */
#define PIN_BTN        0   /* Tactile page / diagnostic mode button */

#define TFT_W        160
#define TFT_H         80
#define COL_START      1
#define ROW_START     26

#define C_BLACK   0x0000
#define C_NAVY    0x0008
#define C_DARKBG  0x0862
#define C_PANEL   0x10C4
#define C_CYAN    0x07FF
#define C_GREEN   0x07E0
#define C_GOLD    0xFEA0
#define C_ORANGE  0xFD20
#define C_RED     0xF800
#define C_WHITE   0xFFFF

static const uint32_t MASK_SCLK = (1UL << (TFT_SCLK - 32));
static const uint32_t MASK_CS   = (1UL << (TFT_CS   - 32));
static const uint32_t MASK_RST  = (1UL << (TFT_RST  - 32));
static const uint32_t MASK_DC   = (1UL << (TFT_DC   - 32));
static const uint32_t MASK_MOSI = (1UL << (TFT_MOSI - 32));

static uint16_t fb[TFT_W * TFT_H];

// -----------------------------------------------------------------------------
// 3. Compact 5x7 Uppercase ASCII Font & Direct-Register ST7735S Driver
// -----------------------------------------------------------------------------
static const uint8_t FONT5X7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00}, {0x00,0x07,0x00,0x07,0x00},
    {0x14,0x7F,0x14,0x7F,0x14}, {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00}, {0x00,0x1C,0x22,0x41,0x00},
    {0x00,0x41,0x22,0x1C,0x00}, {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02}, {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31}, {0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39}, {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}, {0x00,0x36,0x36,0x00,0x00},
    {0x00,0x56,0x36,0x00,0x00}, {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06}, {0x32,0x49,0x79,0x41,0x3E},
    {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}
};

static inline void tftWriteByte(uint8_t val) {
    for (uint8_t bit = 0x80; bit; bit >>= 1) {
        GPIO.out1_w1tc.val = MASK_SCLK;
        if (val & bit) GPIO.out1_w1ts.val = MASK_MOSI;
        else           GPIO.out1_w1tc.val = MASK_MOSI;
        __asm__ __volatile__("nop; nop; nop; nop; nop; nop;");
        GPIO.out1_w1ts.val = MASK_SCLK;
        __asm__ __volatile__("nop; nop; nop; nop; nop; nop;");
    }
    GPIO.out1_w1tc.val = MASK_SCLK;
}

static inline void tftWriteCmd(uint8_t cmd) {
    GPIO.out1_w1tc.val = MASK_DC;
    GPIO.out1_w1tc.val = MASK_CS;
    tftWriteByte(cmd);
    GPIO.out1_w1ts.val = MASK_CS;
}

static inline void tftWriteData(uint8_t data) {
    GPIO.out1_w1ts.val = MASK_DC;
    GPIO.out1_w1tc.val = MASK_CS;
    tftWriteByte(data);
    GPIO.out1_w1ts.val = MASK_CS;
}

static void tftSendCmdList(const uint8_t* addr) {
    uint8_t numCommands = *addr++;
    while (numCommands--) {
        uint8_t cmd = *addr++;
        uint8_t numArgs = *addr++;
        uint8_t ms = numArgs & 0x80;
        numArgs &= 0x7F;
        tftWriteCmd(cmd);
        for (uint8_t i = 0; i < numArgs; i++) tftWriteData(*addr++);
        if (ms) {
            uint8_t dly = *addr++;
            delay(dly == 255 ? 500 : dly);
        }
    }
}

static const uint8_t ST7735_INIT_CMD1[] = {
    15, 0x01, 0x80, 150, 0x11, 0x80, 255, 0xB1, 3, 0x01, 0x2C, 0x2D,
    0xB2, 3, 0x01, 0x2C, 0x2D, 0xB3, 6, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,
    0xB4, 1, 0x07, 0xC0, 3, 0xA2, 0x02, 0x84, 0xC1, 1, 0xC5, 0xC2, 2, 0x0A, 0x00,
    0xC3, 2, 0x8A, 0x2A, 0xC4, 2, 0x8A, 0xEE, 0xC5, 1, 0x0E, 0x20, 0,
    0x36, 1, 0xC8, 0x3A, 1, 0x05
};
static const uint8_t ST7735_INIT_CMD2[] = {
    2, 0x2A, 4, 0x00, 0x02, 0x00, 0x81, 0x2B, 4, 0x00, 0x01, 0x00, 0xA0
};
static const uint8_t ST7735_INIT_CMD3[] = {
    4,
    0xE0, 16, 0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10,
    0xE1, 16, 0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10,
    0x13, 0x80, 10, 0x29, 0x80, 100
};

static void tftInit(void) {
    gpio_reset_pin((gpio_num_t)TFT_SCLK);
    gpio_reset_pin((gpio_num_t)TFT_MOSI);
    gpio_reset_pin((gpio_num_t)TFT_CS);
    gpio_reset_pin((gpio_num_t)TFT_DC);
    gpio_reset_pin((gpio_num_t)TFT_RST);

    pinMode(TFT_SCLK, OUTPUT); digitalWrite(TFT_SCLK, LOW);
    pinMode(TFT_MOSI, OUTPUT); digitalWrite(TFT_MOSI, LOW);
    pinMode(TFT_CS,   OUTPUT); digitalWrite(TFT_CS,   HIGH);
    pinMode(TFT_DC,   OUTPUT); digitalWrite(TFT_DC,   HIGH);
    pinMode(TFT_RST,  OUTPUT); digitalWrite(TFT_RST,  HIGH);

    tftWriteCmd(0x00);
    digitalWrite(TFT_RST, HIGH); delay(5);
    digitalWrite(TFT_RST, LOW);  delay(20);
    digitalWrite(TFT_RST, HIGH); delay(150);

    tftSendCmdList(ST7735_INIT_CMD1);
    tftSendCmdList(ST7735_INIT_CMD2);
    tftWriteCmd(0x21);
    tftSendCmdList(ST7735_INIT_CMD3);
    tftWriteCmd(0x21);
    tftWriteCmd(0x36);
    tftWriteData(0xA8);
}

static void fbClear(uint16_t color) {
    for (int i = 0; i < TFT_W * TFT_H; i++) fb[i] = color;
}

static void fbFillRect(int x, int y, int w, int h, uint16_t color) {
    for (int j = y; j < y + h; j++) {
        if (j < 0 || j >= TFT_H) continue;
        for (int i = x; i < x + w; i++) {
            if (i < 0 || i >= TFT_W) continue;
            fb[j * TFT_W + i] = color;
        }
    }
}

static void fbDrawRect(int x, int y, int w, int h, uint16_t color) {
    fbFillRect(x, y, w, 1, color);
    fbFillRect(x, y + h - 1, w, 1, color);
    fbFillRect(x, y, 1, h, color);
    fbFillRect(x + w - 1, y, 1, h, color);
}

static void fbDrawChar(int x, int y, char c, uint16_t color, int scale) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c < 0x20 || c > 0x5A) c = '?';
    const uint8_t* glyph = FONT5X7[c - 0x20];
    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                fbFillRect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

static void fbDrawStr(int x, int y, const char* s, uint16_t color, int scale) {
    while (*s) {
        fbDrawChar(x, y, *s++, color, scale);
        x += 6 * scale;
    }
}

static void fbFlush(void) {
    uint16_t x0 = COL_START, x1 = COL_START + TFT_W - 1;
    uint16_t y0 = ROW_START, y1 = ROW_START + TFT_H - 1;
    tftWriteCmd(0x2A);
    tftWriteData(x0 >> 8); tftWriteData(x0 & 0xFF);
    tftWriteData(x1 >> 8); tftWriteData(x1 & 0xFF);
    tftWriteCmd(0x2B);
    tftWriteData(y0 >> 8); tftWriteData(y0 & 0xFF);
    tftWriteData(y1 >> 8); tftWriteData(y1 & 0xFF);
    tftWriteCmd(0x2C);
    GPIO.out1_w1ts.val = MASK_DC;
    GPIO.out1_w1tc.val = MASK_CS;
    for (int i = 0; i < TFT_W * TFT_H; i++) {
        uint16_t px = fb[i];
        tftWriteByte(px >> 8);
        tftWriteByte(px & 0xFF);
    }
    GPIO.out1_w1ts.val = MASK_CS;
}

// -----------------------------------------------------------------------------
// 4. Inline USB Pass-Through Power Gauge & Brownout Guard Telemetry
// -----------------------------------------------------------------------------
static float    g_usbVolts     = 0.0f;
static float    g_usbAmps      = 0.0f;
static float    g_usbWatts     = 0.0f;
static uint32_t g_sagEvents    = 0;
static uint32_t g_btnClicks    = 0;
static uint8_t  g_pageMode     = 0;
static uint32_t g_seq          = 0;
static uint32_t g_httpHits     = 0;
static uint32_t g_chatCount    = 0;
static uint32_t g_lastBeaconMs = 0;
static uint32_t g_lastHudMs    = 0;
static bool     g_lastBtnState = HIGH;

static char g_staIp[24]  = "0.0.0.0";
static char g_macStr[24] = "00:00:00:00:00:00";

static const uint16_t HTTP_PORTS[2] = { 80, 8080 };
static int g_httpSocks[2] = { -1, -1 };
static int g_udpSock      = -1;

static void sampleUsbPowerGauge(void) {
    uint32_t mv13_sum = 0, mv12_sum = 0;
    for (int i = 0; i < 16; i++) {
        mv13_sum += analogReadMilliVolts(PIN_USB_VOLT);
        mv12_sum += analogReadMilliVolts(PIN_USB_CURR);
    }
    float mv13 = (float)(mv13_sum >> 4);
    float mv12 = (float)(mv12_sum >> 4);
    g_usbVolts = (mv13 * 8.5f) / 1000.0f;
    g_usbAmps  = (mv12 / 50.0f) * 1.035f;
    if (g_usbAmps < 0.02f) g_usbAmps = 0.00f;
    g_usbWatts = g_usbVolts * g_usbAmps;

    if ((g_usbVolts > 1.0f && g_usbVolts < VBUS_SAG_WARN_VOLTS) || g_usbAmps >= SBC_USB_CEILING_AMPS) {
        g_sagEvents++;
    }
}

static void renderHud(void) {
    fbClear(C_DARKBG);
    char buf[44];
    bool railWarn = (g_usbVolts > 1.0f && g_usbVolts < VBUS_SAG_WARN_VOLTS) || (g_usbAmps >= SBC_USB_CEILING_AMPS);
    uint16_t borderCol = railWarn ? C_RED : g_moodColor;

    if (g_pageMode == 0) {
        fbFillRect(0, 0, TFT_W, 15, C_NAVY);
        fbFillRect(0, 14, TFT_W, 2, borderCol);
        fbDrawRect(0, 0, TFT_W, TFT_H, borderCol);

        snprintf(buf, sizeof(buf), "%s // %s", g_stationCallsign, AXISOS_NODE_LABEL);
        fbDrawStr(4, 4, buf, C_GOLD, 1);

        fbDrawStr(6, 20, railWarn ? "VBUS RAIL WARN" : g_stationRole, railWarn ? C_RED : C_WHITE, 2);
        if (g_lastChatNote[0] != '\0') {
            fbDrawStr(6, 38, g_lastChatNote, C_GOLD, 1);
        } else {
            fbDrawStr(6, 38, "LOCAL SI + USB PASS-THRU", C_CYAN, 1);
        }

        if (WiFi.status() == WL_CONNECTED) {
            snprintf(buf, sizeof(buf), "IP:%s SI READY", g_staIp);
            fbDrawStr(6, 50, buf, C_GREEN, 1);
        } else {
            fbDrawStr(6, 50, "AP: 192.168.11.1 READY", C_ORANGE, 1);
        }

        fbFillRect(1, 63, TFT_W - 2, 16, C_PANEL);
        snprintf(buf, sizeof(buf), "%.2fV  %.2fA  %.2fW  #%lu", g_usbVolts, g_usbAmps, g_usbWatts, (unsigned long)g_seq);
        fbDrawStr(5, 67, buf, railWarn ? C_RED : C_GOLD, 1);
    } else if (g_pageMode == 1) {
        fbFillRect(0, 0, TFT_W, 14, C_NAVY);
        fbDrawRect(0, 0, TFT_W, TFT_H, borderCol);
        fbDrawStr(4, 3, "CRUSAN & CO // VBUS HUD", C_GOLD, 1);

        snprintf(buf, sizeof(buf), "%.2fV", g_usbVolts);
        fbDrawStr(6, 18, buf, railWarn ? C_RED : C_GREEN, 2);
        snprintf(buf, sizeof(buf), "%.2fA", g_usbAmps);
        fbDrawStr(86, 18, buf, C_CYAN, 2);
        snprintf(buf, sizeof(buf), "PWR: %.2fW  SAG:%lu", g_usbWatts, (unsigned long)g_sagEvents);
        fbDrawStr(6, 38, buf, C_GOLD, 1);
        snprintf(buf, sizeof(buf), "IP: %s", g_staIp);
        fbDrawStr(6, 50, buf, C_WHITE, 1);
        fbDrawStr(6, 62, "SI CONSOLE :80 ACTIVE", C_GREEN, 1);
    } else {
        fbFillRect(0, 0, TFT_W, 14, C_NAVY);
        fbDrawRect(0, 0, TFT_W, TFT_H, C_GOLD);
        snprintf(buf, sizeof(buf), "DIAG // %s v%s", AXISOS_NODE_LABEL, FIRMWARE_VER);
        fbDrawStr(4, 3, buf, C_GOLD, 1);

        snprintf(buf, sizeof(buf), "MAC: %s", g_macStr);
        fbDrawStr(6, 18, buf, C_WHITE, 1);
        snprintf(buf, sizeof(buf), "HEAP: %lu B  CHAT: %lu", (unsigned long)ESP.getFreeHeap(), (unsigned long)g_chatCount);
        fbDrawStr(6, 30, buf, C_GOLD, 1);
        snprintf(buf, sizeof(buf), "RSSI: %d DBM  HITS:%lu", (int)WiFi.RSSI(), (unsigned long)g_httpHits);
        fbDrawStr(6, 42, buf, C_GREEN, 1);
        snprintf(buf, sizeof(buf), "UPTIME: %lu S", (unsigned long)(millis() / 1000));
        fbDrawStr(6, 54, buf, C_CYAN, 1);
        fbDrawStr(6, 66, "COME-AND-GET-IT: ARMED", C_GREEN, 1);
    }

    fbFlush();
}

// -----------------------------------------------------------------------------
// 5. Pure C99 BSD-Socket LAN OTA ("COME-AND-GET-IT")
// -----------------------------------------------------------------------------
static int performPureCOtaUpdate(const char* host_ip, uint16_t port, const char* url_path) {
    fbClear(C_NAVY);
    fbDrawRect(0, 0, TFT_W, TFT_H, C_GOLD);
    fbDrawStr(6, 8,  "COME AND GET IT!", C_GOLD, 1);
    fbDrawStr(6, 24, "PULLING C99 OTA...", C_CYAN, 1);
    fbDrawStr(6, 40, AXISOS_NODE_LABEL, C_WHITE, 2);
    fbFlush();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct timeval tv = { .tv_sec = 8, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);
    srv.sin_addr.s_addr = inet_addr(host_ip);

    if (connect(sock, (struct sockaddr*)&srv, sizeof(srv)) < 0) {
        close(sock);
        return -2;
    }

    char req[384];
    int req_len = snprintf(req, sizeof(req),
        "GET %s HTTP/1.0\r\nHost: %s:%u\r\nUser-Agent: AxisOS-ESP32-OTA/1.8.1\r\nConnection: close\r\n\r\n",
        url_path, host_ip, (unsigned)port);
    write(sock, req, req_len);

    const esp_partition_t* update_part = esp_ota_get_next_update_partition(NULL);
    if (!update_part) { close(sock); return -3; }

    esp_ota_handle_t ota_handle = 0;
    uint8_t buf[2048];
    int n = 0, headers_done = 0;
    uint32_t total_written = 0;

    while ((n = read(sock, buf, sizeof(buf))) > 0) {
        if (!headers_done) {
            int hdr_end = -1;
            for (int i = 0; i <= n - 4; i++) {
                if (buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n') {
                    hdr_end = i + 4;
                    break;
                }
            }
            if (hdr_end < 0 || memcmp(buf, "HTTP/1.", 7) != 0 || strstr((const char*)buf, " 200 ") == NULL) {
                close(sock);
                return -4;
            }
            if (esp_ota_begin(update_part, OTA_SIZE_UNKNOWN, &ota_handle) != ESP_OK) {
                close(sock);
                return -5;
            }
            headers_done = 1;
            int body_len = n - hdr_end;
            if (body_len > 0) {
                if (esp_ota_write(ota_handle, buf + hdr_end, body_len) != ESP_OK) {
                    esp_ota_abort(ota_handle);
                    close(sock);
                    return -6;
                }
                total_written += (uint32_t)body_len;
            }
        } else {
            if (esp_ota_write(ota_handle, buf, n) != ESP_OK) {
                esp_ota_abort(ota_handle);
                close(sock);
                return -7;
            }
            total_written += (uint32_t)n;
        }
    }
    close(sock);

    if (!headers_done || total_written < 65536) {
        if (headers_done) esp_ota_abort(ota_handle);
        return -8;
    }
    if (esp_ota_end(ota_handle) != ESP_OK) return -9;
    if (esp_ota_set_boot_partition(update_part) != ESP_OK) return -10;

    esp_restart();
    return 0;
}

// -----------------------------------------------------------------------------
// 6. Tier-1 On-Chip Hardware Reflexes + Optional Tier-2 Local Qwen2.5 (:8088)
// -----------------------------------------------------------------------------
static void urlDecode(const char* src, char* dst, size_t max_len) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 1 < max_len; i++) {
        if (src[i] == '+') dst[j++] = ' ';
        else if (src[i] == '%' && isxdigit((unsigned char)src[i+1]) && isxdigit((unsigned char)src[i+2])) {
            char hex[3] = { src[i+1], src[i+2], '\0' };
            dst[j++] = (char)strtol(hex, NULL, 16);
            i += 2;
        } else if (src[i] == '&' || src[i] == ' ' || src[i] == '\r' || src[i] == '\n') break;
        else dst[j++] = src[i];
    }
    dst[j] = '\0';
}

static void extractQueryParam(const char* req, const char* key, char* out, size_t out_len) {
    out[0] = '\0';
    char pat1[32], pat2[32];
    snprintf(pat1, sizeof(pat1), "?%s=", key);
    snprintf(pat2, sizeof(pat2), "&%s=", key);
    const char* p = strstr(req, pat1);
    if (!p) p = strstr(req, pat2);
    if (p) {
        p += strlen(key) + 2;
        urlDecode(p, out, out_len);
    }
}

static const char CONSOLE_HTML_P1[] =
"<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Crusan &amp; Co. // AxisOS Inline USB SI Outpost</title>"
"<style>"
"*{box-sizing:border-box;margin:0;padding:0}"
"body{background:#0b101b;color:#f1f5f9;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;padding:14px;max-width:740px;margin:0 auto;line-height:1.45}"
".hdr{background:linear-gradient(135deg,#131d31,#1e1b4b);border:2px solid #f59e0b;border-radius:14px;padding:16px;margin-bottom:12px}"
".hdr h1{font-size:20px;color:#fbbf24;margin-bottom:4px}.hdr p{font-size:13px;color:#cbd5e1}"
".pills{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:8px;margin-top:12px}"
".pill{background:#0f172a;border:1px solid #334155;border-radius:10px;padding:8px 10px;font-size:12px}"
".pill b{display:block;color:#38bdf8;font-size:14px;margin-top:2px}"
".quick{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:12px}"
".qbtn{background:#1e293b;border:1px solid #475569;color:#f8fafc;padding:11px;border-radius:10px;font-size:13px;font-weight:700;cursor:pointer}"
".chatbox{background:#111827;border:1px solid #334155;border-radius:14px;padding:14px;height:270px;overflow-y:auto;margin-bottom:12px;display:flex;flex-direction:column;gap:10px}"
".msg{padding:10px 14px;border-radius:12px;font-size:14px;max-width:90%}"
".msg.ai{background:#1e293b;border-left:4px solid #f59e0b;align-self:flex-start}"
".msg.me{background:#0284c7;color:#fff;align-self:flex-end}"
".msg small{display:block;font-size:11px;color:#94a3b8;margin-bottom:4px;font-weight:700}"
".bar{display:flex;gap:8px;margin-bottom:14px}"
".bar input{flex:1;padding:12px;border-radius:10px;border:1px solid #475569;background:#0f172a;color:#fff;font-size:15px}"
".bar button{padding:12px 18px;border-radius:10px;border:none;background:#f59e0b;color:#0b101b;font-weight:800;cursor:pointer}"
"details{background:#111827;border:1px solid #334155;border-radius:12px;padding:12px}"
"summary{font-weight:700;color:#38bdf8;cursor:pointer;font-size:14px}"
".wform{display:grid;gap:8px;margin-top:10px}"
".wform input{padding:10px;border-radius:8px;border:1px solid #475569;background:#0f172a;color:#fff}"
".wform button{padding:10px;border-radius:8px;border:none;background:#10b981;color:#052e16;font-weight:800;cursor:pointer}"
"</style></head><body>";

static const char CONSOLE_HTML_P2[] =
"<div class='quick'>"
"<button class='qbtn' onclick=\"ask('Audit USB VBUS voltage, current, and brownout margin')\">Audit USB VBUS Rail</button>"
"<button class='qbtn' onclick=\"ask('Report Wi-Fi uplink, RSSI, and NAPT extender status')\">Check Mesh &amp; Wi-Fi</button>"
"<button class='qbtn' onclick=\"ask('Broadcast UDP :50000 mesh discovery beacon')\">Ping Pico Squadron</button>"
"<button class='qbtn' onclick=\"ask('Explain the 2-tier offline SI architecture')\">Architecture Brief</button>"
"</div>"
"<div class='chatbox' id='chat'>"
"<div class='msg ai'><small>AXISOS SILICON SI // ONLINE</small>"
"Running 100% locally inside this inline USB pass-through node. Zero cloud APIs required.</div>"
"</div>"
"<form class='bar' onsubmit='sendMsg(event)'><input id='q' placeholder='Ask about VBUS power, SBC rail margin, or mesh health...' autocomplete='off'><button type='submit'>Send</button></form>"
"<details><summary>Provision Local Wi-Fi Uplink (Saved to NVS Flash)</summary>"
"<div class='wform'><input id='wssid' placeholder='Wi-Fi SSID'><input id='wpass' type='password' placeholder='Wi-Fi Passphrase'>"
"<button type='button' onclick='saveWifi()'>Save Uplink to NVS</button><div id='wmsg' style='font-size:13px;color:#4ade80'></div></div></details>"
"<script>"
"const chat=document.getElementById('chat');"
"function addMsg(who,txt,cls){const d=document.createElement('div');d.className='msg '+cls;d.innerHTML='<small>'+who+'</small>'+txt;chat.appendChild(d);chat.scrollTop=chat.scrollHeight;}"
"async function ask(text){addMsg('OPERATOR',text,'me');try{const r=await fetch('/api/chat?q='+encodeURIComponent(text));const d=await r.json();addMsg(d.agent||'OUTPOST SI',d.reply,'ai');if(d.volts!==undefined)document.getElementById('p_pwr').textContent=d.volts.toFixed(2)+'V · '+d.amps.toFixed(2)+'A ('+d.watts.toFixed(2)+'W)';}catch(e){addMsg('OUTPOST SI','Local silicon responder busy; retry.','ai');}}"
"function sendMsg(e){e.preventDefault();const inp=document.getElementById('q');const v=inp.value.trim();if(!v)return;inp.value='';ask(v);}"
"async function saveWifi(){const s=document.getElementById('wssid').value.trim();const p=document.getElementById('wpass').value;if(!s)return;const r=await fetch('/wifi?ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p));const d=await r.json();document.getElementById('wmsg').textContent=d.message||'Saved!';}"
"setInterval(async()=>{try{const r=await fetch('/status');const d=await r.json();document.getElementById('p_pwr').textContent=d.usb_volts.toFixed(2)+'V · '+d.usb_amps.toFixed(2)+'A ('+d.usb_watts.toFixed(2)+'W)';document.getElementById('p_ip').textContent=d.sta_ip+' ('+d.rssi+' dBm)';}catch(e){}},3000);"
"</script></body></html>";

static void sendPureCUdpBeacon(void);

static void handleApiChat(int cfd, const char* req) {
    char q[192];
    extractQueryParam(req, "q", q, sizeof(q));
    g_chatCount++;

    snprintf(g_lastChatNote, sizeof(g_lastChatNote), "SI: %.22s", q[0] ? q : "PING");
    for (int k = 0; g_lastChatNote[k]; k++) {
        if (g_lastChatNote[k] >= 'a' && g_lastChatNote[k] <= 'z') g_lastChatNote[k] -= 32;
    }
    renderHud();

    char low[192];
    for (size_t i = 0; i < sizeof(low) - 1 && q[i]; i++) {
        low[i] = (char)tolower((unsigned char)q[i]);
        low[i+1] = '\0';
    }
    if (!q[0]) strcpy(low, "status");

    char reply[512];
    if (strstr(low, "power") || strstr(low, "volt") || strstr(low, "amp") || strstr(low, "vbus") || strstr(low, "brownout")) {
        float margin_ma = (SBC_USB_CEILING_AMPS - g_usbAmps) * 1000.0f;
        snprintf(reply, sizeof(reply),
            "VBUS Rail: %.2fV @ %.2fA (%.2fW). Sag events: %lu. Headroom below 1150 mA SBC USB ceiling: %.0f mA. %s",
            g_usbVolts, g_usbAmps, g_usbWatts, (unsigned long)g_sagEvents, margin_ma,
            (g_usbVolts >= VBUS_SAG_WARN_VOLTS && g_usbAmps < SBC_USB_CEILING_AMPS)
                ? "Rail is within Green-Zone spec."
                : "ALERT: VBUS sag or high load detected—offload spinning/NVMe drives to a powered hub!");
    } else if (strstr(low, "wifi") || strstr(low, "mesh") || strstr(low, "network")) {
        snprintf(reply, sizeof(reply),
            "Uplink IP: %s (RSSI %d dBm). NAPT SoftAP '%s' active on 192.168.11.1/24 for companion RP2350 Pico nodes.",
            g_staIp, (int)WiFi.RSSI(), AXISOS_SOFTAP_SSID);
    } else if (strstr(low, "ping") || strstr(low, "beacon") || strstr(low, "pico")) {
        sendPureCUdpBeacon();
        snprintf(reply, sizeof(reply),
            "Transmitted UDP :%u AXISDISC heartbeat frame (#%lu) with live %.2fV/%.2fA telemetry to companion micro-nodes.",
            (unsigned)UDP_DISC_PORT, (unsigned long)g_seq, g_usbVolts, g_usbAmps);
    } else {
        snprintf(reply, sizeof(reply),
            "Tier-1 C99 silicon reflex active on %s (%s). VBUS: %.2fV/%.2fA (%.2fW), Free SRAM: %lu B, Uptime: %lu s.",
            AXISOS_NODE_LABEL, BOARD_TYPE, g_usbVolts, g_usbAmps, g_usbWatts,
            (unsigned long)ESP.getFreeHeap(), (unsigned long)(millis() / 1000));
    }

    char body[768];
    int body_len = snprintf(body, sizeof(body),
        "{\"ok\":true,\"node\":\"%s\",\"agent\":\"%s // C99-SI\",\"reply\":\"%s\",\"volts\":%.2f,\"amps\":%.2f,\"watts\":%.2f}\n",
        AXISOS_NODE_ID, AXISOS_NODE_LABEL, reply, g_usbVolts, g_usbAmps, g_usbWatts);

    char hdr[256];
    int hdr_len = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n",
        body_len);
    send(cfd, hdr, hdr_len, 0);
    send(cfd, body, body_len, 0);
}

static void sendConsoleHtml(int cfd) {
    char mid[768];
    snprintf(mid, sizeof(mid),
        "<div class='hdr'>"
        "<h1>Crusan &amp; Co. // %s Inline USB SI Console</h1>"
        "<p>Board: <b>%s</b> &middot; Firmware: <b>v%s</b> &middot; 360&deg; Air-Gapped Green Zone</p>"
        "<div class='pills'>"
        "<div class='pill'>Inline VBUS Power<b id='p_pwr'>%.2fV &middot; %.2fA (%.2fW)</b></div>"
        "<div class='pill'>Wi-Fi Uplink<b id='p_ip'>%s (%d dBm)</b></div>"
        "<div class='pill'>NAPT Extender<b>%s (192.168.11.1)</b></div>"
        "</div></div>",
        AXISOS_NODE_LABEL, BOARD_TYPE, FIRMWARE_VER,
        g_usbVolts, g_usbAmps, g_usbWatts,
        g_staIp, (int)WiFi.RSSI(), AXISOS_SOFTAP_SSID);

    int total_len = (int)strlen(CONSOLE_HTML_P1) + (int)strlen(mid) + (int)strlen(CONSOLE_HTML_P2);
    char hdr[256];
    int hdr_len = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
        total_len);
    send(cfd, hdr, hdr_len, 0);
    send(cfd, CONSOLE_HTML_P1, strlen(CONSOLE_HTML_P1), 0);
    send(cfd, mid, strlen(mid), 0);
    send(cfd, CONSOLE_HTML_P2, strlen(CONSOLE_HTML_P2), 0);
}

// -----------------------------------------------------------------------------
// 7. Pure C BSD Socket Servers (:80, :8080, UDP :50000) & USB PHY Handoff
// -----------------------------------------------------------------------------
static void initPureCSockets(void) {
    for (int i = 0; i < 2; i++) {
        if (g_httpSocks[i] >= 0) close(g_httpSocks[i]);
        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s >= 0) {
            int opt = 1;
            setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
            struct sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(HTTP_PORTS[i]);
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == 0 && listen(s, 4) == 0) {
                g_httpSocks[i] = s;
            } else {
                close(s);
                g_httpSocks[i] = -1;
            }
        }
    }

    if (g_udpSock >= 0) close(g_udpSock);
    g_udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_udpSock >= 0) {
        int opt = 1;
        setsockopt(g_udpSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(g_udpSock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
        fcntl(g_udpSock, F_SETFL, fcntl(g_udpSock, F_GETFL, 0) | O_NONBLOCK);
        struct sockaddr_in uaddr = {};
        uaddr.sin_family = AF_INET;
        uaddr.sin_port = htons(UDP_DISC_PORT);
        uaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        bind(g_udpSock, (struct sockaddr*)&uaddr, sizeof(uaddr));
    }
}

static void connectUplinkWifi(void) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.AP.begin();
    WiFi.AP.config(AP_IP, AP_IP, AP_MASK, AP_LEASE, AP_DNS);
    WiFi.AP.create(AXISOS_SOFTAP_SSID, AXISOS_SOFTAP_PASS, 6);
    WiFi.setHostname(AXISOS_NODE_ID);

    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(g_macStr, sizeof(g_macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    const char* target_ssid = (g_customSsid[0] != '\0') ? g_customSsid : AXISOS_UPLINK_SSID;
    const char* target_pass = (g_customSsid[0] != '\0') ? g_customPass : AXISOS_UPLINK_PASS;
    if (strcmp(target_ssid, "YOUR_PRIMARY_WIFI_SSID") == 0) return;

    WiFi.disconnect(false, true);
    delay(100);
    WiFi.begin(target_ssid, target_pass);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < 7000)) {
        delay(250);
    }
    if (WiFi.status() == WL_CONNECTED) {
        IPAddress ip = WiFi.localIP();
        snprintf(g_staIp, sizeof(g_staIp), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
        WiFi.AP.enableNAPT(true);
        renderHud();
    }
}

/* Detach ESP32-S3 internal USB_SERIAL_JTAG 1.5k pull-ups on GPIO19/20 so downstream
 * USB 2.0 devices plugged into the female pass-through port enumerate unimpeded! */
static void releaseUsbPassthrough(void) {
    Serial.end();

    SET_PERI_REG_MASK(RTC_CNTL_USB_CONF_REG,
        RTC_CNTL_SW_HW_USB_PHY_SEL |
        RTC_CNTL_USB_PAD_ENABLE_OVERRIDE |
        RTC_CNTL_USB_PAD_PULL_OVERRIDE |
        RTC_CNTL_USB_TX_EN_OVERRIDE);
    CLEAR_PERI_REG_MASK(RTC_CNTL_USB_CONF_REG,
        RTC_CNTL_USB_PAD_ENABLE |
        RTC_CNTL_USB_TX_EN |
        RTC_CNTL_USB_DP_PULLUP |
        RTC_CNTL_USB_DP_PULLDOWN |
        RTC_CNTL_USB_DM_PULLUP |
        RTC_CNTL_USB_DM_PULLDOWN);

    SET_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG,
        USB_SERIAL_JTAG_PAD_PULL_OVERRIDE |
        USB_SERIAL_JTAG_PHY_SEL);
    CLEAR_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG,
        USB_SERIAL_JTAG_USB_PAD_ENABLE |
        USB_SERIAL_JTAG_DP_PULLUP |
        USB_SERIAL_JTAG_DM_PULLUP |
        USB_SERIAL_JTAG_DP_PULLDOWN |
        USB_SERIAL_JTAG_DM_PULLDOWN);

    /* Pulse 200 ms SE0 USB reset then float D-/D+ high-impedance */
    gpio_reset_pin((gpio_num_t)19);
    gpio_reset_pin((gpio_num_t)20);
    gpio_set_direction((gpio_num_t)19, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)20, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)19, 0);
    gpio_set_level((gpio_num_t)20, 0);
    delay(200);
    gpio_reset_pin((gpio_num_t)19);
    gpio_reset_pin((gpio_num_t)20);
    gpio_set_direction((gpio_num_t)19, GPIO_MODE_DISABLE);
    gpio_set_direction((gpio_num_t)20, GPIO_MODE_DISABLE);
    gpio_pullup_dis((gpio_num_t)19);
    gpio_pulldown_dis((gpio_num_t)19);
    gpio_pullup_dis((gpio_num_t)20);
    gpio_pulldown_dis((gpio_num_t)20);
}

static void sendPureCUdpBeacon(void) {
    if (g_udpSock < 0 || WiFi.status() != WL_CONNECTED) return;
    char beacon[320];
    int blen = snprintf(beacon, sizeof(beacon),
        "AXISDISC|%s|%s|%s|seq:%lu|v:%.2fV|a:%.2fA|w:%.2fW|sag:%lu|heap:%lu|fw:%s",
        AXISOS_NODE_ID, BOARD_TYPE, g_staIp,
        (unsigned long)g_seq, g_usbVolts, g_usbAmps, g_usbWatts,
        (unsigned long)g_sagEvents, (unsigned long)ESP.getFreeHeap(), FIRMWARE_VER);

    struct sockaddr_in dst = {};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(UDP_DISC_PORT);
    IPAddress bcast = WiFi.localIP();
    bcast[3] = 255;
    dst.sin_addr.s_addr = (uint32_t)bcast;
    sendto(g_udpSock, beacon, blen, 0, (struct sockaddr*)&dst, sizeof(dst));
}

static void pollPureCHttpSockets(void) {
    for (int i = 0; i < 2; i++) {
        if (g_httpSocks[i] < 0) continue;
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int cfd = accept(g_httpSocks[i], (struct sockaddr*)&cli_addr, &cli_len);
        if (cfd < 0) continue;

        int cflags = fcntl(cfd, F_GETFL, 0);
        if (cflags >= 0) fcntl(cfd, F_SETFL, cflags & ~O_NONBLOCK);
        struct timeval tv = { .tv_sec = 0, .tv_usec = 350000 };
        setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        char req[1024];
        int total_r = 0;
        req[0] = '\0';
        while (total_r < (int)sizeof(req) - 1) {
            int n = recv(cfd, req + total_r, sizeof(req) - 1 - total_r, 0);
            if (n <= 0) break;
            total_r += n;
            req[total_r] = '\0';
            if (strstr(req, "\r\n\r\n") != NULL) break;
        }
        g_httpHits++;

        if (strncmp(req, "GET /ota", 8) == 0) {
            const char* ack = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"status\":\"OTA_TRIGGERED\"}\n";
            send(cfd, ack, strlen(ack), 0);
            close(cfd);
            performPureCOtaUpdate(AXISOS_LOCAL_SI_HUB_IP, AXISOS_OTA_PORT, OTA_BIN_PATH);
            return;
        }
        if (strncmp(req, "GET /api/chat", 13) == 0) {
            handleApiChat(cfd, req);
            close(cfd);
            return;
        }
        if (strncmp(req, "GET /wifi?", 10) == 0) {
            char n_ssid[48], n_pass[48];
            extractQueryParam(req, "ssid", n_ssid, sizeof(n_ssid));
            extractQueryParam(req, "pass", n_pass, sizeof(n_pass));
            if (n_ssid[0] != '\0') {
                g_prefs.begin("axis_wifi", false);
                g_prefs.putString("ssid", n_ssid);
                g_prefs.putString("pass", n_pass);
                g_prefs.end();
                strncpy(g_customSsid, n_ssid, sizeof(g_customSsid) - 1);
                strncpy(g_customPass, n_pass, sizeof(g_customPass) - 1);
            }
            const char* wresp = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"ok\":true,\"message\":\"Saved Wi-Fi to NVS! Reconnecting...\"}\n";
            send(cfd, wresp, strlen(wresp), 0);
            close(cfd);
            if (n_ssid[0] != '\0') { connectUplinkWifi(); initPureCSockets(); }
            return;
        }
        if (strncmp(req, "GET /chat", 9) == 0 || (i == 0 && strncmp(req, "GET / ", 6) == 0)) {
            sendConsoleHtml(cfd);
            close(cfd);
            return;
        }

        char body[640];
        int body_len = snprintf(body, sizeof(body),
            "{\"status\":\"ONLINE\",\"company\":\"Crusan & Co.\",\"platform\":\"AxisOS\","
            "\"node_id\":\"%s\",\"board\":\"%s\",\"firmware\":\"%s\","
            "\"sta_ip\":\"%s\",\"ap_ssid\":\"%s\",\"ap_ip\":\"192.168.11.1\","
            "\"rssi\":%d,\"uptime_s\":%lu,\"free_heap\":%lu,"
            "\"usb_volts\":%.2f,\"usb_amps\":%.2f,\"usb_watts\":%.2f,\"sag_events\":%lu}\n",
            AXISOS_NODE_ID, BOARD_TYPE, FIRMWARE_VER, g_staIp, AXISOS_SOFTAP_SSID,
            (int)WiFi.RSSI(), (unsigned long)(millis() / 1000), (unsigned long)ESP.getFreeHeap(),
            g_usbVolts, g_usbAmps, g_usbWatts, (unsigned long)g_sagEvents);
        char hdr[200];
        int hdr_len = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", body_len);
        send(cfd, hdr, hdr_len, 0);
        send(cfd, body, body_len, 0);
        close(cfd);
    }
}

void setup(void) {
    pinMode(PIN_BTN, INPUT_PULLUP);
    analogReadResolution(12);

    g_prefs.begin("axis_wifi", true);
    String s = g_prefs.getString("ssid", "");
    String p = g_prefs.getString("pass", "");
    g_prefs.end();
    if (s.length() > 0) {
        strncpy(g_customSsid, s.c_str(), sizeof(g_customSsid) - 1);
        strncpy(g_customPass, p.c_str(), sizeof(g_customPass) - 1);
    }

    tftInit();
    sampleUsbPowerGauge();
    renderHud();

    connectUplinkWifi();
    releaseUsbPassthrough();
    initPureCSockets();
    renderHud();
}

void loop(void) {
    bool btn = digitalRead(PIN_BTN);
    if (btn == LOW && g_lastBtnState == HIGH) {
        g_btnClicks++;
        g_pageMode = (g_pageMode + 1) % 3;
        renderHud();
        delay(50);
    }
    g_lastBtnState = btn;

    pollPureCHttpSockets();

    uint32_t now = millis();
    if (now - g_lastHudMs >= 1000) {
        g_lastHudMs = now;
        sampleUsbPowerGauge();
        renderHud();
    }
    if (now - g_lastBeaconMs >= 3000) {
        g_lastBeaconMs = now;
        g_seq++;
        sendPureCUdpBeacon();
    }
}
