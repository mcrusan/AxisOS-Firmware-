/**
 * @file    axisOS_version.h
 * @brief   AxisOS Version Stamp — auto-generated, do not edit manually
 *
 * Copyright (c) 2026 AxisOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef AXISOS_VERSION_H
#define AXISOS_VERSION_H

#define AXISOS_VERSION_MAJOR    1
#define AXISOS_VERSION_MINOR    4
#define AXISOS_VERSION_PATCH    0
#define AXISOS_VERSION_BUILD    "20260617"
#define AXISOS_VERSION_STRING   "1.4.0-20260617"
#define AXISOS_VERSION_CODENAME "Cascadia"

#define AXISOS_TARGET_BOARD     "Raspberry Pi Pico 2W"
#define AXISOS_TARGET_MCU       "RP2350"
#define AXISOS_SDK_VERSION      "2.1.0"

/* Release channel: RELEASE | RC | BETA | NIGHTLY */
#define AXISOS_RELEASE_CHANNEL  "RELEASE"

/* Composite integer for compile-time comparisons */
#define AXISOS_VERSION_INT      ((AXISOS_VERSION_MAJOR << 16) | \
                                 (AXISOS_VERSION_MINOR <<  8) | \
                                  AXISOS_VERSION_PATCH)

#endif /* AXISOS_VERSION_H */
