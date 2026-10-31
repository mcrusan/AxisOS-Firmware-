/*
 * ============================================================================
 * File:        firmware/src/axisOS_hal.c
 * Project:     Crusan & Co. // AxisOS — Sovereign Pico 2 W Firmware
 * Author:      Marc F. Crusan / Crusan & Co. w/ AGY (AXEL)
 * Date:        2026-10-30
 * Version:     v1.7.0-Silicon
 * Description: AxisOS Hardware Abstraction Layer implementation for Pico 2 W
 *              (GPIO, CYW43 LED, ADC, I2C, and NULL-safe SPI transfers).
 * Changelog:
 *   - 2026-10-30 (MFC/AGY): Fixed NULL tx/rx pointer dereference in
 *     hal_spi_transfer() by branching on (tx && rx), tx-only, and rx-only.
 * ============================================================================
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "axisOS_hal.h"

#define TAG "HAL"

/* -----------------------------------------------------------------------
 *  GPIO
 * --------------------------------------------------------------------- */
axisOS_status_t hal_gpio_init(uint8_t pin, hal_gpio_dir_t dir, bool pull_up) {
    gpio_init(pin);
    gpio_set_dir(pin, (dir == HAL_GPIO_OUT) ? GPIO_OUT : GPIO_IN);
    if (dir == HAL_GPIO_IN && pull_up) gpio_pull_up(pin);
    return AXISOS_OK;
}

void hal_gpio_write(uint8_t pin, bool value) { gpio_put(pin, value); }
bool hal_gpio_read(uint8_t pin)               { return gpio_get(pin); }
void hal_gpio_toggle(uint8_t pin)             { gpio_xor_mask(1u << pin); }

/* -----------------------------------------------------------------------
 *  LED (Pico 2W: CYW43 GPIO 0)
 * --------------------------------------------------------------------- */
void hal_led_set(bool on) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
}

void hal_led_toggle(void) {
    static bool state = false;
    state = !state;
    hal_led_set(state);
}

void hal_led_blink(uint32_t on_ms, uint32_t off_ms, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        hal_led_set(true);
        sleep_ms(on_ms);
        hal_led_set(false);
        sleep_ms(off_ms);
    }
}

/* -----------------------------------------------------------------------
 *  ADC
 * --------------------------------------------------------------------- */
axisOS_status_t hal_adc_init(uint8_t channel) {
    adc_init();
    if (channel <= 3) {
        adc_gpio_init(26 + channel);
    } else if (channel == 4) {
        adc_set_temp_sensor_enabled(true);
    } else {
        return AXISOS_ERR_PARAM;
    }
    return AXISOS_OK;
}

uint16_t hal_adc_read_raw(uint8_t channel) {
    adc_select_input(channel);
    return adc_read();
}

float hal_adc_read_voltage(uint8_t channel) {
    return hal_adc_read_raw(channel) * (3.3f / (1 << 12));
}

float hal_adc_read_temp_c(void) {
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);
    uint16_t raw = adc_read();
    float v = raw * (3.3f / (1 << 12));
    return 27.0f - (v - 0.706f) / 0.001721f;
}

/* -----------------------------------------------------------------------
 *  I2C
 * --------------------------------------------------------------------- */
axisOS_status_t hal_i2c_init(uint8_t i2c_id, uint8_t sda_pin,
                              uint8_t scl_pin, uint32_t baud_hz) {
    i2c_inst_t *inst = (i2c_id == 0) ? i2c0 : i2c1;
    i2c_init(inst, baud_hz);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
    return AXISOS_OK;
}

axisOS_status_t hal_i2c_write(uint8_t i2c_id, uint8_t addr,
                               const uint8_t *buf, size_t len) {
    if (!buf || len == 0) return AXISOS_ERR_PARAM;
    i2c_inst_t *inst = (i2c_id == 0) ? i2c0 : i2c1;
    int rc = i2c_write_blocking(inst, addr, buf, len, false);
    return (rc == (int)len) ? AXISOS_OK : AXISOS_ERR_IO;
}

axisOS_status_t hal_i2c_read(uint8_t i2c_id, uint8_t addr,
                              uint8_t *buf, size_t len) {
    if (!buf || len == 0) return AXISOS_ERR_PARAM;
    i2c_inst_t *inst = (i2c_id == 0) ? i2c0 : i2c1;
    int rc = i2c_read_blocking(inst, addr, buf, len, false);
    return (rc == (int)len) ? AXISOS_OK : AXISOS_ERR_IO;
}

/* -----------------------------------------------------------------------
 *  SPI
 * --------------------------------------------------------------------- */
axisOS_status_t hal_spi_init(uint8_t spi_id, uint8_t sck_pin,
                              uint8_t mosi_pin, uint8_t miso_pin,
                              uint32_t baud_hz) {
    spi_inst_t *inst = (spi_id == 0) ? spi0 : spi1;
    spi_init(inst, baud_hz);
    gpio_set_function(sck_pin,  GPIO_FUNC_SPI);
    gpio_set_function(mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(miso_pin, GPIO_FUNC_SPI);
    return AXISOS_OK;
}

axisOS_status_t hal_spi_transfer(uint8_t spi_id, uint8_t cs_pin,
                                  const uint8_t *tx, uint8_t *rx, size_t len) {
    if ((!tx && !rx) || len == 0) return AXISOS_ERR_PARAM;
    spi_inst_t *inst = (spi_id == 0) ? spi0 : spi1;
    gpio_put(cs_pin, 0);
    if (tx && rx) {
        spi_write_read_blocking(inst, tx, rx, len);
    } else if (tx) {
        spi_write_blocking(inst, tx, len);
    } else {
        spi_read_blocking(inst, 0x00, rx, len);
    }
    gpio_put(cs_pin, 1);
    return AXISOS_OK;
}

/* -----------------------------------------------------------------------
 *  HAL lifecycle
 * --------------------------------------------------------------------- */
axisOS_status_t hal_init(void) {
    hal_adc_init(4); /* enable onboard temp sensor */
    hal_led_set(false);
    LOG_I(TAG, "HAL ready. Core temp: %.1f C", hal_adc_read_temp_c());
    return AXISOS_OK;
}

void hal_deinit(void) {
    hal_led_set(false);
}
