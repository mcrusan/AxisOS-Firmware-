/*
 * ============================================================================
 * File:        firmware/include/axisOS_core.h
 * Project:     Crusan & Co. // AxisOS — Sovereign Pico 2 W Firmware
 * Author:      Marc F. Crusan / Crusan & Co. w/ AGY (AXEL)
 * Date:        2026-10-30
 * Version:     v1.7.0-Silicon
 * Description: AxisOS core subsystem API (status codes, watchdog, tick, log).
 * Changelog:
 *   - 2026-10-30 (MFC/AGY): Standardized Crusan & Co. SOP header.
 * ============================================================================
 */

#ifndef AXISOS_CORE_H
#define AXISOS_CORE_H

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 *  Return codes
 * --------------------------------------------------------------------- */
typedef enum {
    AXISOS_OK            =  0,
    AXISOS_ERR_GENERIC   = -1,
    AXISOS_ERR_TIMEOUT   = -2,
    AXISOS_ERR_NOMEM     = -3,
    AXISOS_ERR_BUSY      = -4,
    AXISOS_ERR_PARAM     = -5,
    AXISOS_ERR_IO        = -6,
    AXISOS_ERR_NOT_FOUND = -7,
} axisOS_status_t;

/* -----------------------------------------------------------------------
 *  Watchdog
 * --------------------------------------------------------------------- */
#define AXISOS_WDT_TIMEOUT_MS   8000u   /* 8 s — Pico max is ~8.3 s */

/* -----------------------------------------------------------------------
 *  Tick
 * --------------------------------------------------------------------- */
uint32_t core_tick_ms(void);

/* -----------------------------------------------------------------------
 *  Lifecycle
 * --------------------------------------------------------------------- */
axisOS_status_t core_init(void);
void            core_deinit(void);
bool            core_is_ready(void);

/* -----------------------------------------------------------------------
 *  Logging
 * --------------------------------------------------------------------- */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
} axisOS_log_level_t;

void core_log_set_level(axisOS_log_level_t level);
void core_log(axisOS_log_level_t level, const char *tag, const char *fmt, ...);

#define LOG_D(tag, fmt, ...) core_log(LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#define LOG_I(tag, fmt, ...) core_log(LOG_LEVEL_INFO,  tag, fmt, ##__VA_ARGS__)
#define LOG_W(tag, fmt, ...) core_log(LOG_LEVEL_WARN,  tag, fmt, ##__VA_ARGS__)
#define LOG_E(tag, fmt, ...) core_log(LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)

#endif /* AXISOS_CORE_H */
