
/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "ui/ui_state.h"
#include "main.h"

static const char *TAG = "example";

QueueHandle_t qRotor; // notify rotary encoder events externally

#define EXAMPLE_PCNT_HIGH_LIMIT 4
#define EXAMPLE_PCNT_LOW_LIMIT  -4

#define EXAMPLE_EC11_GPIO_A 37
#define EXAMPLE_EC11_GPIO_B 38

static bool example_pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    ui_event_t evt;
    int rotor_count = edata->watch_point_value;
    if(tv_hw_config.rotor_dir == ROTOR_DIR_CCW){
        rotor_count *= -1;
    }
    evt.type  = (rotor_count > 0) ? UI_ROTOR_INC : UI_ROTOR_DEC;
    evt.param = (void*)(edata->watch_point_value);

    BaseType_t high_task_wakeup;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    // send event data to queue, from this interrupt callback
    if(edata->watch_point_value != 0){
        //xQueueSendFromISR(queue,  &(edata->watch_point_value), &high_task_wakeup);  
        xQueueSendFromISR(qRotor, &(evt),                      &high_task_wakeup);
    }
    return (high_task_wakeup == pdTRUE);
}

void init_rotary_encoder(void* p)
{

    qRotor = xQueueCreate(16, sizeof(ui_event_t));

    ESP_LOGI(TAG, "install pcnt unit");
    pcnt_unit_config_t unit_config = {
        .high_limit = EXAMPLE_PCNT_HIGH_LIMIT,
        .low_limit = EXAMPLE_PCNT_LOW_LIMIT,
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    ESP_LOGI(TAG, "set glitch filter");
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    ESP_LOGI(TAG, "install pcnt channels");
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = EXAMPLE_EC11_GPIO_A,
        .level_gpio_num = EXAMPLE_EC11_GPIO_B,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = EXAMPLE_EC11_GPIO_B,
        .level_gpio_num = EXAMPLE_EC11_GPIO_A,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

    ESP_LOGI(TAG, "set edge and level actions for pcnt channels");
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_LOGI(TAG, "add watch points and register callbacks");
    int watch_points[] = {EXAMPLE_PCNT_LOW_LIMIT,  0, EXAMPLE_PCNT_HIGH_LIMIT};
    for (size_t i = 0; i < sizeof(watch_points) / sizeof(watch_points[0]); i++) {
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, watch_points[i]));
    }
    pcnt_event_callbacks_t cbs = {
        .on_reach = example_pcnt_on_reach,
    };
    QueueHandle_t queue = xQueueCreate(10, sizeof(int));
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, queue));

    ESP_LOGI(TAG, "enable pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_LOGI(TAG, "clear pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_LOGI(TAG, "start pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

#if CONFIG_EXAMPLE_WAKE_UP_LIGHT_SLEEP
    // EC11 channel output high level in normal state, so we set "low level" to wake up the chip
    ESP_ERROR_CHECK(gpio_wakeup_enable(EXAMPLE_EC11_GPIO_A, GPIO_INTR_LOW_LEVEL));
    ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());
    ESP_ERROR_CHECK(esp_light_sleep_start());
#endif

    // // Report counter value
    // int pulse_count = 0;
    // int event_count = 0;
    // while (1) {
    //     if (xQueueReceive(queue, &event_count, pdMS_TO_TICKS(1000))) {
    //         ESP_LOGI(TAG, "Watch point event, count: %d", event_count);
    //     } else {
    //         ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
    //         ESP_LOGI(TAG, "Pulse count: %d", pulse_count);
    //     }
    // }
}


// /*
//  * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
//  *
//  * SPDX-License-Identifier: CC0-1.0
//  */

// #include "sdkconfig.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/queue.h"
// #include "esp_log.h"
// #include "driver/pulse_cnt.h"
// #include "driver/gpio.h"
// #include "esp_sleep.h"
// #include "main.h"
// #include <math.h>
// #include <stdio.h>
// #include "ui/ui_state.h"

// QueueHandle_t qRotor; // notify rotary encoder events externally


// static const char *TAG = "rotary";


// // TODO: these numbers should come from HW config rotor count
// #define EXAMPLE_PCNT_HIGH_LIMIT  4
// #define EXAMPLE_PCNT_LOW_LIMIT  -4

// #define ROTOR_EC11_GPIO_A   GPIO_NUM_37
// #define ROTOR_EC11_GPIO_B   GPIO_NUM_38
// #define ROTOR_PUSH_BUTTON   GPIO_NUM_36

  

// static bool example_pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
// {
//     BaseType_t high_task_wakeup = false;
//  //   QueueHandle_t queue = (QueueHandle_t)user_ctx;
//     // send event data to queue, from this interrupt callback

//     ui_event_t evt;
//     int rotor_count = edata->watch_point_value;
//     if(tv_hw_config.rotor_dir == ROTOR_DIR_CCW){
//         rotor_count *= -1;
//     }
//     evt.type  = (rotor_count > 0) ? UI_ROTOR_INC : UI_ROTOR_DEC;
//     evt.param = (void*)(edata->watch_point_value);

//     if(edata->watch_point_value == 0){
//         // ignore zero counts
//         return false;
//     }

//     xQueueSendFromISR(qRotor, &evt, &high_task_wakeup);

//     //pcnt_unit_clear_count(unit);

//     return (high_task_wakeup == pdTRUE);
// }

// void init_rotary_encoder(void* p)
// {
//     ESP_LOGI(TAG, "install pcnt unit");
//     pcnt_unit_config_t unit_config = {
//         //.intr_priority = configMAX_PRIORITIES-4,
//         .high_limit = EXAMPLE_PCNT_HIGH_LIMIT,
//         .low_limit  = EXAMPLE_PCNT_LOW_LIMIT,
//         .intr_priority = 3,
//         .flags.accum_count = false        
//     };
//     pcnt_unit_handle_t pcnt_unit = NULL;
//     ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

//     ESP_LOGI(TAG, "set glitch filter");
//     pcnt_glitch_filter_config_t filter_config = {
//         .max_glitch_ns = 1000,
//     };
//     ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

//     ESP_LOGI(TAG, "install pcnt channels");
//     pcnt_chan_config_t chan_a_config = {
//         .edge_gpio_num = ROTOR_EC11_GPIO_A,
//         .level_gpio_num = ROTOR_EC11_GPIO_B,
//     };
//     pcnt_channel_handle_t pcnt_chan_a = NULL;

//     ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));


//     pcnt_chan_config_t chan_b_config = {
//         .edge_gpio_num = ROTOR_EC11_GPIO_B,
//         .level_gpio_num = ROTOR_EC11_GPIO_A,
//     };
//     pcnt_channel_handle_t pcnt_chan_b = NULL;
//     ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

//     ESP_LOGI(TAG, "set edge and level actions for pcnt channels");
//     ESP_ERROR_CHECK(pcnt_channel_set_edge_action( pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
//     ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP,    PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
//     ESP_ERROR_CHECK(pcnt_channel_set_edge_action( pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
//     ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP,    PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

//      ESP_LOGI(TAG, "add watch points at %d and -%d", tv_hw_config.rotor_clicks, tv_hw_config.rotor_clicks);
// //  //   int watch_points[] = {EXAMPLE_PCNT_LOW_LIMIT, -50, 0, 50, EXAMPLE_PCNT_HIGH_LIMIT};
//     int watch_points[] = { tv_hw_config.rotor_clicks, -tv_hw_config.rotor_clicks}; //, 2 * tv_hw_config.rotor_clicks, -2 * tv_hw_config.rotor_clicks };

//     for (size_t i = 0; i < sizeof(watch_points) / sizeof(watch_points[0]); i++) {
//         ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, watch_points[i]));
//     }
//     pcnt_event_callbacks_t cbs = {
//         .on_reach = example_pcnt_on_reach
//     };
//     qRotor = xQueueCreate(16, sizeof(ui_event_t));
//     ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, qRotor));

//     ESP_LOGI(TAG, "enable pcnt unit");
//     ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
//     ESP_LOGI(TAG, "clear pcnt unit");
//     ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
//     ESP_LOGI(TAG, "start pcnt unit");
//     ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

// #if CONFIG_EXAMPLE_WAKE_UP_LIGHT_SLEEP
//     // EC11 channel output high level in normal state, so we set "low level" to wake up the chip
//     ESP_ERROR_CHECK(gpio_wakeup_enable(ROTOR_EC11_GPIO_A, GPIO_INTR_LOW_LEVEL));
//     ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());
//     ESP_ERROR_CHECK(esp_light_sleep_start());
// #endif

// }
