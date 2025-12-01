#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "freertos/portmacro.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "main.h"
#include "ui/ui_state.h"

QueueHandle_t qRotor;
static ui_event_t evt;

static volatile int tick_counter = 0;
int resolution_ms = 5;

uint8_t  a, b, a_prev;
uint32_t a_history, b_history;
uint8_t  history_depth = 4;
uint32_t mask;


IRAM_ATTR void vApplicationTickHook( void ){
    
    // BaseType_t xHigherPriorityTaskWoken = pdFALSE;


    // if(tick_counter++ < resolution_ms)
    //     return;

    // tick_counter = 0;
    
    // a = gpio_get_level(GPIO_NUM_38);
    // b = gpio_get_level(GPIO_NUM_37);    
    

    // a_history = (a_history << 1) | a;
    // b_history = (b_history << 1) | b;

    // uint8_t a_tmp = (a_history & mask);
    // uint8_t b_tmp = (b_history & mask);

    // // do we have stable levels?

    // if( (a_tmp != 0x00 && a_tmp != ( mask )) && (b_tmp != 0x0 && b_tmp != ( mask ) ) ){
    //     return;
    // }
    
    // if(a != a_prev ){

    //     a_prev = a;

    //     if(a){// positive transition on a?
    //         if(b){
    //             evt.type =  UI_ROTOR_INC;
    //         }else{
    //             evt.type =  UI_ROTOR_DEC;
    //         }
    //         if( pdTRUE != xQueueSendToBackFromISR(qRotor, &evt, &xHigherPriorityTaskWoken)){ 
        
    //             ESP_LOGE("FFS", "qRotor FULL");
        
    //         }
    //     }
    // }

}
    

void shitty_rotary_encoder_handler(void* p){

    int resolution_ms = 5;

    uint8_t  a, b, a_prev;
    uint32_t a_history, b_history;
    uint8_t  history_depth = 4;
    uint32_t mask = (1<<history_depth)-1;

    a = gpio_get_level(GPIO_NUM_38);
    b = gpio_get_level(GPIO_NUM_37); 
    a_prev = a; 
    a_history = a ? 0xffffffff : 0x00000000;
    b_history = b ? 0xffffffff : 0x00000000;
    int cnt = 0;

    

    while (1) {

        vTaskDelay(resolution_ms); //configTICK_RATE_HZ s.b. set to 1000Hz (normally 100 Hz)portTICK_PERIOD_MS

        a = gpio_get_level(GPIO_NUM_38);
        b = gpio_get_level(GPIO_NUM_37);    
        

        a_history = (a_history << 1) | a;
        b_history = (b_history << 1) | b;

        uint8_t a_tmp = (a_history & mask);
        uint8_t b_tmp = (b_history & mask);

        // do we have stable levels?

        if( (a_tmp != 0x00 && a_tmp != ( mask )) && (b_tmp != 0x0 && b_tmp != ( mask ) ) ){
            continue;
        }
        
        if(a != a_prev ){

            a_prev = a;

            if(a){// positive transition on a?
                if(b){
                    cnt--;
                    evt.type =  UI_ROTOR_INC;
                }else{
                    cnt--;
                    evt.type =  UI_ROTOR_DEC;
                }
                if( pdTRUE != xQueueSendToBack(qRotor, &evt, 1000/portTICK_PERIOD_MS)){ //no ticks to wait: return immediately if queue is full
            
                    ESP_LOGE("FFS", "qRotor FULL");
            
                }
            }
        }

        // transition on a?
        // if( a != a_prev ){

        //     a_prev = a;
        //     if( a == b ){
        //         // clockwise
        //         cnt++;
        //         evt.type =  UI_ROTOR_INC;
        //     } else {
        //         // counter-clockwise
        //         cnt--;
        //         evt.type =  UI_ROTOR_DEC;
        //     }
        //     // history will be shifted out of visibility eventually
        //     // poison current history to prevent retriggering
        //     a_history ^= -1UL;
        //     b_history ^= -1UL;
            
        //     ESP_LOGI("FFS", "cnt: %d\n", cnt);
 
        //     if( pdTRUE != xQueueSendToBack(qRotor, &evt, 1000/portTICK_PERIOD_MS)){ //no ticks to wait: return immediately if queue is full
        
        //         ESP_LOGE("FFS", "qRotor FULL");
        
        //     }
        // }

    }
}

void init_rot_encoder_shitty(void){

    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_37 | 1ULL << GPIO_NUM_38),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_config);

    mask = (1<<history_depth)-1;

    a = gpio_get_level(GPIO_NUM_38);
    b = gpio_get_level(GPIO_NUM_37); 
    a_prev = a; 
    a_history = a ? 0xffffffff : 0x00000000;
    b_history = b ? 0xffffffff : 0x00000000;

    qRotor = xQueueCreate(10, sizeof(ui_event_t));

   // xTaskCreatePinnedToCore(&shitty_rotary_encoder_handler, "shitty_rotary_encoder_handler", 4096, NULL, configMAX_PRIORITIES-3, NULL, tskNO_AFFINITY);
}