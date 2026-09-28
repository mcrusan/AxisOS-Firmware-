/**
 * @file    axisOS_tasks.c
 * @brief   AxisOS Cooperative Task Scheduler Implementation
 *
 * Copyright (c) 2026 AxisOS Project
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include <stdio.h>
#include "axisOS_tasks.h"
#include "axisOS_core.h"

#define TAG "TASKS"

/* -----------------------------------------------------------------------
 *  Internal task descriptor
 * --------------------------------------------------------------------- */
typedef struct {
    char        name[AXISOS_TASK_NAME_LEN];
    task_fn_t   fn;
    void       *ctx;
    uint32_t    period_ms;
    uint32_t    last_run_ms;
    bool        active;
    bool        suspended;
    uint32_t    run_count;
} task_entry_t;

/* -----------------------------------------------------------------------
 *  Private state
 * --------------------------------------------------------------------- */
static task_entry_t s_tasks[AXISOS_MAX_TASKS];
static uint8_t      s_task_count = 0;
static bool         s_ready      = false;

/* -----------------------------------------------------------------------
 *  Public API
 * --------------------------------------------------------------------- */
axisOS_status_t tasks_init(void) {
    memset(s_tasks, 0, sizeof(s_tasks));
    s_task_count = 0;
    s_ready      = true;
    LOG_I(TAG, "Task scheduler ready. Max slots: %u", AXISOS_MAX_TASKS);
    return AXISOS_OK;
}

axisOS_status_t tasks_register(const char *name, task_fn_t fn, void *ctx,
                                uint32_t period_ms, task_id_t *out_id) {
    if (!s_ready || !fn || !out_id)       return AXISOS_ERR_PARAM;
    if (s_task_count >= AXISOS_MAX_TASKS) return AXISOS_ERR_NOMEM;

    task_entry_t *t = &s_tasks[s_task_count];
    strncpy(t->name, name ? name : "unnamed", AXISOS_TASK_NAME_LEN - 1);
    t->fn          = fn;
    t->ctx         = ctx;
    t->period_ms   = period_ms;
    t->last_run_ms = core_tick_ms();
    t->active      = true;
    t->suspended   = false;
    t->run_count   = 0;

    *out_id = s_task_count++;
    LOG_I(TAG, "Registered task [%u] '%s' @ %lu ms", *out_id, t->name, (unsigned long)period_ms);
    return AXISOS_OK;
}

void tasks_run(void) {
    if (!s_ready) return;
    uint32_t now = core_tick_ms();
    for (uint8_t i = 0; i < s_task_count; i++) {
        task_entry_t *t = &s_tasks[i];
        if (!t->active || t->suspended) continue;
        if ((now - t->last_run_ms) >= t->period_ms) {
            t->last_run_ms = now;
            t->run_count++;
            t->fn(t->ctx);
        }
    }
}

axisOS_status_t tasks_suspend(task_id_t id) {
    if (id >= s_task_count) return AXISOS_ERR_NOT_FOUND;
    s_tasks[id].suspended = true;
    return AXISOS_OK;
}

axisOS_status_t tasks_resume(task_id_t id) {
    if (id >= s_task_count) return AXISOS_ERR_NOT_FOUND;
    s_tasks[id].suspended   = false;
    s_tasks[id].last_run_ms = core_tick_ms();
    return AXISOS_OK;
}

axisOS_status_t tasks_unregister(task_id_t id) {
    if (id >= s_task_count) return AXISOS_ERR_NOT_FOUND;
    memset(&s_tasks[id], 0, sizeof(task_entry_t));
    s_tasks[id].active = false;
    return AXISOS_OK;
}

void tasks_dump(void) {
    printf("\r\n--- Task Table ---\r\n");
    for (uint8_t i = 0; i < s_task_count; i++) {
        task_entry_t *t = &s_tasks[i];
        printf("  [%02u] %-24s  %5lu ms  runs=%-8lu  %s\r\n",
               i, t->name, (unsigned long)t->period_ms,
               (unsigned long)t->run_count,
               t->suspended ? "SUSPENDED" : (t->active ? "ACTIVE" : "INACTIVE"));
    }
    printf("------------------\r\n\r\n");
}
