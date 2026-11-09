<!--
==============================================================================
File:        README.md
Description: Official technical documentation for Crusan & Co. AxisOS-Firmware — Bare-Metal RP2350 (Pico 2 W) & Embedded Micro-Node Firmware Ecosystem.
Author:      Marc F. Crusan (MFC) / Crusan & Co. w/ PP & AGY (AXEL)
Date:        2026110901
Version:     v1.8.1-Silicon
==============================================================================
-->
<!-- 2026110901 (MFC/PP/AGY) Upgraded from 2-line stub to full 5-Field SOP README with Crusan & Co. IP & Trademark notice. -->

# Crusan & Co. — AxisOS™ Embedded Firmware Ecosystem
### **Bare-Metal C11 / `NO_SYS=1` lwIP Firmware for Raspberry Pi Pico 2 W (`RP2350`) & Sovereign Micro-Nodes (`Cub`)**

[![Pillars: HONEST • AUDITABLE • OWNED](https://img.shields.io/badge/AxisOS%E2%84%A2-HONEST%20%E2%80%A2%20AUDITABLE%20%E2%80%A2%20OWNED-brightgreen.svg)](https://axisos.pro)
[![Architecture: ARM Cortex-M33 (RP2350)](https://img.shields.io/badge/Silicon-RP2350%20%28Pico%202%20W%29-blue.svg)](firmware/CMakeLists.txt)
[![Company: Crusan & Co.](https://img.shields.io/badge/Company-Crusan%20%26%20Co.-gold.svg)](https://axisos.pro)

**Parent Enterprise:** Crusan & Co.  
**Flagship Platform:** **AxisOS™** (*U.S. Trademark Application Serial No. `99878469`*)  
**Architect:** Marc F. Crusan  
**Website:** [https://axisos.pro](https://axisos.pro)  

> **Intellectual Property & Proprietary Core Exclusion Notice:** This repository provides the public reference SDK layer, HAL interfaces, and release verification artifacts for the **AxisOS™ Pico 2 W (`RP2350`) Embedded Ecosystem** built by **Crusan & Co.** The underlying **AxisOS™ Native Bare-Metal Kernel** (`axisos-kernel.img`), the **freestanding C99 direct syscall userland** (`svc #0`), the **Sovereign Seven (`Sov-7`)** autonomous embedded daemon suite (`zero-core`, `axel-core`, `medona-core`, `lite`, `axis-agent`), `RAMGAK` deterministic memory bracket tooling, and production `AXIS_ROLE_MAYOR` mesh security keys are not distributed in this repository and remain the exclusive, proprietary intellectual property of **Crusan & Co.** (Marc F. Crusan). All Rights Reserved.

---

## 1. Architecture & The Three Pillars of AxisOS™

The **AxisOS™** micro-node firmware stack is engineered from first principles for deterministic, bare-metal execution on the **Raspberry Pi Pico 2 W (`RP2350` Dual Cortex-M33)** without RTOS bloat or cloud telemetry dependencies:

- **HONEST**: Hardware-level die temperature ADC telemetry (`GPIO26` / internal temp sensor), real-time CYW43439 RSSI signal reporting, and deterministic hardware watchdog (`WDT`) supervision.
- **AUDITABLE**: Cooperative non-preemptive task scheduler (`axisOS_tasks.c`) paired with a bare-metal `NO_SYS=1` `pico_cyw43_arch_lwip_poll` network stack (`lwipopts.h`).
- **OWNED**: Zero external cloud brokers, zero MQTT subscriptions, and local LAN/Mesh discovery (`UDP :50000`) with `COME-AND-GET-IT` over-the-air (`OTA`) flash readiness.

---

## 2. Repository Structure

| Path | Description |
| :--- | :--- |
| `firmware/CMakeLists.txt` | CMake build specification targeting `pico2_w` (`RP2350`) with `pico_cyw43_arch_lwip_poll` |
| `firmware/config/axisOS_config.h` | Compile-time configuration defaults (Wi-Fi uplink, hardware watchdog intervals, GPIO pinout, OTA endpoint) |
| `firmware/config/lwipopts.h` | Bare-metal `NO_SYS=1` lwIP memory and TCP/UDP stack tuning |
| `firmware/include/axisOS_core.h` | Core status codes, millisecond tick counter, watchdog refresh, and structured logging API |
| `firmware/include/axisOS_hal.h` | Hardware Abstraction Layer (`GPIO`, `ADC` die temp & `VBAT`, `I2C0`, `SPI0`, onboard CYW43 LED) |
| `firmware/include/axisOS_tasks.h` | Cooperative fixed-slot task scheduler API with deterministic slot reclamation |
| `firmware/include/axisOS_wifi.h` | CYW43439 Wi-Fi station state machine and watchdog-safe connection manager |
| `firmware/include/axisOS_version.h` | Semantic firmware version macros (`v1.8.1-Silicon`) |
| `firmware/src/*.c` | Pure C11 implementation modules (`main.c`, `axisOS_core.c`, `axisOS_hal.c`, `axisOS_tasks.c`, `axisOS_wifi.c`) |
| `firmware/tools/build.sh` | Automated toolchain & Pico SDK discovery build script generating `.uf2` images and `.sha256` digests |

---

## 3. Building the Firmware (`UF2`)

Ensure the **Raspberry Pi Pico SDK (`>= 2.0.0`)** and `arm-none-eabi-gcc` are installed, then run:

```bash
cd firmware
./tools/build.sh release
```

To pass custom Wi-Fi uplink credentials at compile time without modifying source headers:

```bash
cmake -B firmware/build -S firmware \
  -DPICO_BOARD=pico2_w \
  -DCMAKE_C_FLAGS="-DAXISOS_WIFI_SSID=\\\"YourUplinkSSID\\\" -DAXISOS_WIFI_PASSWORD=\\\"YourPassphrase\\\""
cmake --build firmware/build --parallel
```

The resulting `axisOS-v1.8.1-Silicon.uf2` binary and its `.sha256` manifest are emitted into `firmware/release/`.

---

**Copyright © 2026 Crusan & Co. (Marc F. Crusan). All Rights Reserved.**  
**AxisOS™** is a trademark of Crusan & Co. (*U.S. Trademark Application Serial No. `99878469`*).
