/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "driver/rmt_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type of led strip encoder configuration
 */
typedef struct {
    uint32_t resolution; /*!< Encoder resolution, in Hz */
} led_strip_encoder_config_t;

/**
 * @brief Create RMT encoder for encoding LED strip pixels into RMT symbols
 *
 * @param[in] config Encoder configuration
 * @param[out] ret_encoder Returned encoder handle
 * @return
 *      - ESP_ERR_INVALID_ARG for any invalid arguments
 *      - ESP_ERR_NO_MEM out of memory when creating led strip encoder
 *      - ESP_OK if creating encoder successfully
 */
esp_err_t rmt_new_led_strip_encoder(const led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

#define POCKET 99

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#ifdef POCKET
#define RMT_LED_STRIP_GPIO_NUM      21
#else
#define RMT_LED_STRIP_GPIO_NUM      13
#endif

#define EXAMPLE_LED_NUMBERS         3
#define EXAMPLE_CHASE_SPEED_MS      10

extern uint8_t led_strip_pixels[]; //[EXAMPLE_LED_NUMBERS * 3];

void rmt_led_init(int nLeds);

void set_the_led(uint8_t r, uint8_t g, uint8_t b);
void set_led_n(uint8_t n, uint8_t r, uint8_t g, uint8_t b);
void write_leds(void);

#ifdef __cplusplus
}
#endif
