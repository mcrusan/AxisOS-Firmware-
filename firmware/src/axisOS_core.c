/**
 * @file    axisOS_core.c
 * @brief   AxisOS Core Subsystem Implementation
 *
 * Copyright (c) 2026 AxisOS Project
 * SPDX-License-Identifier: MIT
 */

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
static bool              s_ready      = false;
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
    s_log_level = level;
}

void core_log(axisOS_log_level_t level, const char *tag, const char *fmt, ...) {
    if (level < s_log_level) return;

    printf("[%s][%7lu][%s] ", s_level_str[level], (unsigned long)core_tick_ms(), tag);

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

    LOG_I("CORE", "AxisOS %s initialising...", AXISOS_VERSION_STRING);

    /* Seed any platform RNG if available */
    /* rosc_hw->randombit provides a weak random source on RP2350 */

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
