#include "math.h"
#include "main.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "led_strip_encoder.h"
#include "cJSON.h"
#include "server.h"
#include "ui/ui_state.h"
#include "sdcard.h"
#include "led_strip.h"
#include "oled_i2c.h"
#include "tv.h"
#include "creds.h"
#include "driver/gpio.h"

static char* TAG = "super";
int hue = 0;
tv_runtime_status_t tv_status;

// Spupervisor can receive JSON messages passed via void*
QueueHandle_t qSuperMessage;

int get_hue(void){
    return hue;
}

RGBColor lamp_rgbc;

extern RGBColor hslToRgb(float h, float s, float l);

void send_current_file_data(void){

    char payload[512]; // fname 260, 
    snprintf(payload, sizeof payload, "{\"type\":\"NowPlaying\",\
\"filename\":\"%s\",\
\"nframes\":%ld,\"frame\":%ld\
}", tv_status.current_file, tv_status.file_frame_count, tv_status.frame_number);
    ws_notify(payload);
}

void update_page_hue(void) {
    char hue_msg[64];
    snprintf(hue_msg, sizeof(hue_msg), "{\"hue\":%d}", hue);
    ESP_LOGI(TAG, "Sending hue message: %s", hue_msg);
    // send the hue message to the websocket clients
    ws_notify(hue_msg);
}

#define SUPER_SLEEP 25

extern void wifi_init_apsta(void);
extern tv_preferences_t     tv_prefs;

void supervisor(void* params){
    
    memset(&tv_status, 0, sizeof tv_status);

    // setup TODO add to status nLEDs, buffer size
    g_frame_size = (tv_hw_config.resolution = TV_RESOLUTION_HIGH ? FRAME_RESOLUTION_HIGH : FRAME_RESOLUTION_LOW) * tv_hw_config.bytes_per_LED;
    

    const int hue_increment = 45;
    qSuperMessage = xQueueCreate(10, sizeof(void*));
    char* msg;

    static int one_second_accum = 0;  //TODO : counter to trigger ws update


    ESP_ERROR_CHECK( load_hw_config(&tv_hw_config));
    ESP_ERROR_CHECK( load_tv_preferences(&tv_prefs));


    TaskHandle_t hUI = ui_init(&tv_status);

    TaskHandle_t hTV = tv_init(&tv_status);   

    // temp test
    gpio_config_t cd_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << GPIO_NUM_8),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&cd_conf);
    uint8_t curr_cd, prev_cd;
    curr_cd = prev_cd = gpio_get_level(GPIO_NUM_8);

    // go
    while(1){

        curr_cd = gpio_get_level(GPIO_NUM_8);
        if(curr_cd != prev_cd){
            prev_cd = curr_cd;
            ESP_LOGI(TAG, "SDCard dtection is %d", curr_cd);
        }

        while(xQueueReceive(qSuperMessage, &msg, 0)){
            ESP_LOGI(TAG, "Received super message: %s", msg);
            // do something with the super message
            if(!msg){
                ESP_LOGE(TAG, "Received NULL message");
                continue;
            }
            cJSON* json = cJSON_Parse(msg);
            if(!json){
                ESP_LOGE(TAG, "Failed to parse JSON message: %s", cJSON_GetErrorPtr());
                goto json_end;
            }
            // process the json message
            cJSON* hue_item = cJSON_GetObjectItem(json, "hue");

            if(hue_item && cJSON_IsNumber(hue_item)){
                hue = hue_item->valueint;
                ESP_LOGI(TAG, "Setting new hue from message: %d", hue);
                RGBColor lamp_rgbc = hslToRgb(hue, 90, 30);
                
                //cJSON_Delete(hue_item);
            }
            cJSON* status = cJSON_GetObjectItem(json, "status");
            if(status && cJSON_IsString(status)){
                ESP_LOGI(TAG, "Status message: %s", status->valuestring);

                //TODO send dynamic page data, e.g. list of files on sd card
                // currently playing file
                // frame size and frame count
                send_current_file_data();
                //cJSON_Delete(status);

            }
            cJSON* tv_control = cJSON_GetObjectItem(json, "TVcontrol");
            if(tv_control && cJSON_IsString(tv_control)){
                ESP_LOGI(TAG, "TV COntrol message: %s", tv_control->valuestring);

                //TODO send dynamic page data, e.g. list of files on sd card
                // currently playing file
                // frame size and frame count
                //cJSON_Delete(tv_control);

            }

json_end:
            cJSON_Delete(json); // apparently we need to _Delete even a NULL object
            if(msg){free(msg);}else{ESP_LOGI(TAG, "==> NULL message here <==");} // free the message after processing
        }

        one_second_accum += SUPER_SLEEP;
        if(one_second_accum > 1000){
            one_second_accum = 0;

            //update webpage
            send_current_file_data();
        }

        vTaskDelay( SUPER_SLEEP / portTICK_PERIOD_MS);
    }    
}