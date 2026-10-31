/**
 * @file    axisOS_tasks.h
 * @brief   AxisOS Cooperative Task Scheduler API
 *
 * Copyright (c) 2026 AxisOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef AXISOS_TASKS_H
#define AXISOS_TASKS_H

#include <stdint.h>
#include <stdbool.h>
#include "axisOS_core.h"

/* -----------------------------------------------------------------------
 *  Limits
 * --------------------------------------------------------------------- */
#define AXISOS_MAX_TASKS        16u
#define AXISOS_TASK_NAME_LEN    24u

/* -----------------------------------------------------------------------
 *  Task handle
 * --------------------------------------------------------------------- */
typedef uint8_t task_id_t;
#define AXISOS_TASK_INVALID  ((task_id_t)0xFF)

/* -----------------------------------------------------------------------
 *  Task function signature
 * --------------------------------------------------------------------- */
typedef void (*task_fn_t)(void *ctx);

/* -----------------------------------------------------------------------
 *  API
 * --------------------------------------------------------------------- */

/**
 * @brief  Initialise the task scheduler.
 */
axisOS_status_t tasks_init(void);

/**
 * @brief  Register a periodic task.
 * @param  name        Human-readable name (for logging/debugging).
 * @param  fn          Function to call on each period.
 * @param  ctx         Opaque context pointer passed to fn.
 * @param  period_ms   Execution period in milliseconds.
 * @param  out_id      Assigned task ID written here on success.
 */
axisOS_status_t tasks_register(const char  *name,
                                task_fn_t    fn,
                                void        *ctx,
                                uint32_t     period_ms,
                                task_id_t   *out_id);

/**
 * @brief  Run all due tasks. Call from the main loop every ~1 ms.
 */
void tasks_run(void);

/**
 * @brief  Suspend / resume a task by ID.
 */
axisOS_status_t tasks_suspend(task_id_t id);
axisOS_status_t tasks_resume(task_id_t id);

/**
 * @brief  Unregister and free a task slot.
 */
axisOS_status_t tasks_unregister(task_id_t id);

/**
 * @brief  Dump task table to stdout (debug builds).
 */
void tasks_dump(void);

#endif /* AXISOS_TASKS_H */
