/**
 * @file    main.c
 * @brief   AxisOS — Main Entry Point for Raspberry Pi Pico 2W
 *
 * Copyright (c) 2026 AxisOS Project
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"

#include "axisOS_version.h"
#include "axisOS_config.h"
#include "axisOS_core.h"
#include "axisOS_wifi.h"
#include "axisOS_hal.h"
#include "axisOS_tasks.h"

/* -----------------------------------------------------------------------
 *  Boot banner
 * --------------------------------------------------------------------- */
static void print_banner(void) {
    printf("\r\n");
    printf("  ╔══════════════════════════════════════════╗\r\n");
    printf("  ║          A x i s O S  v%-18s║\r\n", AXISOS_VERSION_STRING);
    printf("  ║   Codename: %-30s║\r\n", AXISOS_VERSION_CODENAME);
    printf("  ║   Board   : %-30s║\r\n", AXISOS_TARGET_BOARD);
    printf("  ║   Channel : %-30s║\r\n", AXISOS_RELEASE_CHANNEL);
    printf("  ╚══════════════════════════════════════════╝\r\n");
    printf("\r\n");
}

/* -----------------------------------------------------------------------
 *  Tasks
 * --------------------------------------------------------------------- */
static void heartbeat_task(void *ctx) {
    (void)ctx;
    hal_led_toggle();
}

/* -----------------------------------------------------------------------
 *  Watchdog helpers
 * --------------------------------------------------------------------- */
static void watchdog_init_safe(void) {
    if (watchdog_caused_reboot()) {
        printf("[WARN] System recovered from watchdog reset.\r\n");
    }
    watchdog_enable(AXISOS_WDT_TIMEOUT_MS, true);
}

/* -----------------------------------------------------------------------
 *  main
 * --------------------------------------------------------------------- */
int main(void) {
    task_id_t heartbeat_id;

    /* --- Platform init --- */
    stdio_init_all();
    sleep_ms(200);   /* Allow USB-CDC to enumerate */

    print_banner();

    printf("[BOOT] Clock speed : %lu MHz\r\n", clock_get_hz(clk_sys) / 1000000);
    printf("[BOOT] Build stamp : %s\r\n", AXISOS_VERSION_BUILD);

    /* --- Hardware Abstraction Layer --- */
    if (hal_init() != AXISOS_OK) {
        printf("[ERROR] HAL init failed — halting.\r\n");
        while (true) tight_loop_contents();
    }

    /* --- CYW43 / Wi-Fi chip --- */
    if (cyw43_arch_init()) {
        printf("[ERROR] CYW43 init failed — halting.\r\n");
        while (true) tight_loop_contents();
    }

    /* --- Core subsystems --- */
    core_init();
    wifi_init();
    tasks_init();

    tasks_register("heartbeat", heartbeat_task, NULL, AXISOS_HEARTBEAT_MS, &heartbeat_id);

    /* --- Watchdog --- */
    watchdog_init_safe();

    printf("[BOOT] AxisOS ready.\r\n\r\n");

    /* --- Main loop --- */
    while (true) {
        tasks_run();
        wifi_poll();
        watchdog_update();
        sleep_ms(1);
    }

    return 0; /* unreachable */
}
