/*
 * ============================================================================
 * File:        firmware/include/axisOS_tasks.h
 * Project:     Crusan & Co. // AxisOS — Sovereign Pico 2 W Firmware
 * Author:      Marc F. Crusan / Crusan & Co. w/ AGY (AXEL)
 * Date:        2026-10-30
 * Version:     v1.7.0-Silicon
 * Description: AxisOS cooperative periodic task scheduler API.
 * Changelog:
 *   - 2026-10-30 (MFC/AGY): Standardized Crusan & Co. SOP header.
 * ============================================================================
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
axisOS_status_t tasks_init(void);

axisOS_status_t tasks_register(const char  *name,
                                task_fn_t    fn,
                                void        *ctx,
                                uint32_t     period_ms,
                                task_id_t   *out_id);

void tasks_run(void);

axisOS_status_t tasks_suspend(task_id_t id);
axisOS_status_t tasks_resume(task_id_t id);
axisOS_status_t tasks_unregister(task_id_t id);
void            tasks_dump(void);

#endif /* AXISOS_TASKS_H */
