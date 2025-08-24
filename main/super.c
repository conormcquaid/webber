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

static char* TAG = "super";
int hue = 0;

QueueHandle_t qSuperMessage;

int get_hue(void){
    return hue;
}

RGBColor hslToRgb(float h, float s, float l) {
    float r, g, b;
    float c, x, m;

    h = fmod(h, 360.0f); // Ensure hue is within 0-360
    if (h < 0) {
        h += 360.0f;
    }
    s /= 100.0f;
    l /= 100.0f;

    c = (1.0f - fabs(2.0f * l - 1.0f)) * s;
    x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
    m = l - c / 2.0f;

    if (h >= 0.0f && h < 60.0f) {
        r = c, g = x, b = 0.0f;
    } else if (h >= 60.0f && h < 120.0f) {
        r = x, g = c, b = 0.0f;
    } else if (h >= 120.0f && h < 180.0f) {
        r = 0.0f, g = c, b = x;
    } else if (h >= 180.0f && h < 240.0f) {
        r = 0.0f, g = x, b = c;
    } else if (h >= 240.0f && h < 300.0f) {
        r = x, g = 0.0f, b = c;
    } else {
        r = c, g = 0.0f, b = x;
    }

    RGBColor color;
    color.r = (int)((r + m) * 255.0f);
    color.g = (int)((g + m) * 255.0f);
    color.b = (int)((b + m) * 255.0f);

    return color;
}
void update_page_hue(void) {
    char hue_msg[64];
    snprintf(hue_msg, sizeof(hue_msg), "{\"hue\":%d}", hue);
    ESP_LOGI(TAG, "Sending hue message: %s", hue_msg);
    // send the hue message to the websocket clients
    ws_notify(hue_msg);
}

#define SUPER_SLEEP 100

void supervisor(void* params){

    // setup
    int event_count; // event count from the rotary encoder

    const int hue_increment = 45;
    qSuperMessage = xQueueCreate(10, sizeof(void*));
    char* msg;

    // for(int q = 0; q < 54; q++){
    //     led_strip_pixels[q] = 0; // clear the led strip pixels
    //     RGBColor c = hslToRgb(360 * q / 54.0, 90, 30);
    //     led_strip_pixels[q*3] = c.r;
    //     led_strip_pixels[q*3 + 1] = c.g;
    //     led_strip_pixels[q*3 + 2] = c.b;

    // }
    // set_the_led(9,9,9);

    // static int wait = 1000;
    // static int en = 0;

//    current_ui_state->init();  

    // go
    while(1){

        if (xQueueReceive(qRotor, &event_count, 0)){

            hue = hue + (event_count < 0 ? -hue_increment : hue_increment);
            hue = hue % 360;
            ESP_LOGI(TAG, "Setting new hue: %d", hue);
            RGBColor rgbc = hslToRgb(hue, 90, 30);
            
            //set_the_led(rgbc.r, rgbc.g, rgbc.b);

            update_page_hue();

        }
        if(xQueueReceive(qSuperMessage, &msg, 0)){
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
                RGBColor rgbc = hslToRgb(hue, 90, 30);
                
                //set_the_led(rgbc.r, rgbc.g, rgbc.b);
            }
            cJSON* status = cJSON_GetObjectItem(json, "status");
            if(status && cJSON_IsString(status)){
                ESP_LOGI(TAG, "Status message: %s", status->valuestring);

            }



            free(msg); // free the message after processing
        }

        // if(wait){
        //     wait--;
        // }else{
            
        //    // led_strip_pixels[en] = 0;
        //     en ++;
        //     ESP_LOGI(TAG, "Setting byte %d", en);
        //    /// led_strip_pixels[en] = 55;
        //    // write_leds();
        //     wait = 100;
        // }

   //     current_ui_state->tick(SUPER_SLEEP);         

        vTaskDelay( SUPER_SLEEP / portTICK_PERIOD_MS);
    }    
}