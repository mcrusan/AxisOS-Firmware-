/*
 * ============================================================================
 * File:        firmware/include/axisOS_version.h
 * Project:     Crusan & Co. // AxisOS — Sovereign Pico 2 W Firmware
 * Author:      Marc F. Crusan / Crusan & Co. w/ AGY (AXEL)
 * Date:        2026-10-30
 * Version:     v1.7.0-Silicon
 * Description: AxisOS version stamp and target MCU identification header.
 * Changelog:
 *   - 2026-10-30 (MFC/AGY): Bumped to v1.7.0-Silicon (20261030).
 * ============================================================================
 */

#ifndef AXISOS_VERSION_H
#define AXISOS_VERSION_H

#define AXISOS_VERSION_MAJOR    1
#define AXISOS_VERSION_MINOR    7
#define AXISOS_VERSION_PATCH    0
#define AXISOS_VERSION_BUILD    "20261030"
#define AXISOS_VERSION_STRING   "1.7.0-Silicon"
#define AXISOS_VERSION_CODENAME "Silicon"

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
