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

static char* TAG = "super";
int hue = 0;
tv_status_t tv_status;

QueueHandle_t qSuperMessage;

int get_hue(void){
    return hue;
}

RGBColor lamp_rgbc;
extern RGBColor hslToRgb(float h, float s, float l);

void update_page_hue(void) {
    char hue_msg[64];
    snprintf(hue_msg, sizeof(hue_msg), "{\"hue\":%d}", hue);
    ESP_LOGI(TAG, "Sending hue message: %s", hue_msg);
    // send the hue message to the websocket clients
    ws_notify(hue_msg);
}

#define SUPER_SLEEP 25

extern void wifi_init_apsta(void);

void supervisor(void* params){
    
    memset(&tv_status, 0, sizeof tv_status);

    // setup TODO add to status nLEDs, buffer size
    g_frame_size = (tv_config_block.resolution = TV_RESOLUTION_HIGH ? FRAME_RESOLUTION_HIGH : FRAME_RESOLUTION_LOW) * tv_config_block.bytes_per_LED;
    

    const int hue_increment = 45;
    qSuperMessage = xQueueCreate(10, sizeof(void*));
    char* msg;

    tv_status.brightness = 1.0;
    // tv_status.current_file;
    // tv_status.file_frame_count;
    // tv_status.file_size_bytes;
    // tv_status.frame_number;
    // tv_status.ip_addr;
    tv_status.speed.tweens = 7;
    tv_status.speed.interframe_millis = 41;
    // tv_status.ssid;

    
    ui_init(&tv_status);

    // this gut will set tv_status items. prudent to pass it in as a pointer to make it obvious?
    tv_init(&tv_status);   



    // go
    while(1){

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
                continue;
            }
            // process the json message
            cJSON* hue_item = cJSON_GetObjectItem(json, "hue");

            //cJSON_HasObjectItem

            if(hue_item && cJSON_IsNumber(hue_item)){
                hue = hue_item->valueint;
                ESP_LOGI(TAG, "Setting new hue from message: %d", hue);
                RGBColor lamp_rgbc = hslToRgb(hue, 90, 30);
                
                //set_the_led(rgbc.r, rgbc.g, rgbc.b);
            }
            cJSON* status = cJSON_GetObjectItem(json, "status");
            if(status && cJSON_IsString(status)){
                ESP_LOGI(TAG, "Status message: %s", status->valuestring);

            }
            cJSON_Delete(json);
            free(msg); // free the message after processing
        }

        vTaskDelay( SUPER_SLEEP / portTICK_PERIOD_MS);
    }    
}