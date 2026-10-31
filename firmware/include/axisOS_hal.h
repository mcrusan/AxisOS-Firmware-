/**
 * @file    axisOS_hal.h
 * @brief   AxisOS Hardware Abstraction Layer — Pico 2W GPIO / ADC / I2C / SPI
 *
 * Copyright (c) 2026 AxisOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef AXISOS_HAL_H
#define AXISOS_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "axisOS_core.h"

/* -----------------------------------------------------------------------
 *  GPIO
 * --------------------------------------------------------------------- */
typedef enum {
    HAL_GPIO_IN  = 0,
    HAL_GPIO_OUT = 1,
} hal_gpio_dir_t;

axisOS_status_t hal_gpio_init(uint8_t pin, hal_gpio_dir_t dir, bool pull_up);
void            hal_gpio_write(uint8_t pin, bool value);
bool            hal_gpio_read(uint8_t pin);
void            hal_gpio_toggle(uint8_t pin);

/* -----------------------------------------------------------------------
 *  Onboard LED (CYW43 GPIO 0)
 * --------------------------------------------------------------------- */
void hal_led_set(bool on);
void hal_led_toggle(void);
void hal_led_blink(uint32_t on_ms, uint32_t off_ms, uint8_t count);

/* -----------------------------------------------------------------------
 *  ADC (channels 0–3 available on Pico 2W)
 * --------------------------------------------------------------------- */
axisOS_status_t hal_adc_init(uint8_t channel);
uint16_t        hal_adc_read_raw(uint8_t channel);
float           hal_adc_read_voltage(uint8_t channel);

/* Internal temperature sensor on ADC channel 4 */
float           hal_adc_read_temp_c(void);

/* -----------------------------------------------------------------------
 *  I2C
 * --------------------------------------------------------------------- */
axisOS_status_t hal_i2c_init(uint8_t i2c_id, uint8_t sda_pin,
                              uint8_t scl_pin, uint32_t baud_hz);
axisOS_status_t hal_i2c_write(uint8_t i2c_id, uint8_t addr,
                               const uint8_t *buf, size_t len);
axisOS_status_t hal_i2c_read(uint8_t i2c_id, uint8_t addr,
                              uint8_t *buf, size_t len);

/* -----------------------------------------------------------------------
 *  SPI
 * --------------------------------------------------------------------- */
axisOS_status_t hal_spi_init(uint8_t spi_id, uint8_t sck_pin,
                              uint8_t mosi_pin, uint8_t miso_pin,
                              uint32_t baud_hz);
axisOS_status_t hal_spi_transfer(uint8_t spi_id, uint8_t cs_pin,
                                  const uint8_t *tx, uint8_t *rx, size_t len);

/* -----------------------------------------------------------------------
 *  HAL lifecycle
 * --------------------------------------------------------------------- */
axisOS_status_t hal_init(void);
void            hal_deinit(void);

#endif /* AXISOS_HAL_H */
