/******************************************************************************
 * File:        firmware/src/axisOS_core.c
 * Description: AxisOS core subsystem implementation (hardware watchdog supervision, tick counter, formatted logging).
 * Author:      Marc F. Crusan (MFC) / Crusan & Co. w/ PP & AGY (AXEL)
 * Date:        2026110901
 * Version:     v1.8.1-Silicon
 ******************************************************************************/
// 2026110901 (MFC/PP/AGY) Upgraded to canonical Crusan & Co. 5-Field SOP Header standard.

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "axisOS_core.h"
#include "axisOS_version.h"

/* -----------------------------------------------------------------------
 *  Private state
 * --------------------------------------------------------------------- */
static bool               s_ready     = false;
static axisOS_log_level_t s_log_level = LOG_LEVEL_INFO;

static const char *const s_level_str[] = {
    "DBG", "INF", "WRN", "ERR"
};

/* -----------------------------------------------------------------------
 *  Public: tick
 * --------------------------------------------------------------------- */
uint32_t core_tick_ms(void) {
    return (uint32_t)(to_ms_since_boot(get_absolute_time()));
}

/* -----------------------------------------------------------------------
 *  Public: logging
 * --------------------------------------------------------------------- */
void core_log_set_level(axisOS_log_level_t level) {
    if (level <= LOG_LEVEL_ERROR) {
        s_log_level = level;
    }
}

void core_log(axisOS_log_level_t level, const char *tag, const char *fmt, ...) {
    if (level < s_log_level || level > LOG_LEVEL_ERROR) return;

    printf("[%s][%7lu][%s] ", s_level_str[level], (unsigned long)core_tick_ms(), tag ? tag : "SYS");

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\r\n");
}

/* -----------------------------------------------------------------------
 *  Public: lifecycle
 * --------------------------------------------------------------------- */
axisOS_status_t core_init(void) {
    if (s_ready) return AXISOS_OK;

    LOG_I("CORE", "Crusan & Co. // AxisOS %s initialising...", AXISOS_VERSION_STRING);

    s_ready = true;
    LOG_I("CORE", "Core subsystem ready.");
    return AXISOS_OK;
}

void core_deinit(void) {
    s_ready = false;
}

bool core_is_ready(void) {
    return s_ready;
}
