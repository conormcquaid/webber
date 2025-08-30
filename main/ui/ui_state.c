#include "ui_state.h"
#include "oled_i2c.h"
#include "stdbool.h"
#include "cog2.h"
#include "cog3.h"
#include "esp_log.h"
#include "cog3_hi_res.h"
#include "string.h"
#include "oled_i2c.h"
#include "rotary_encoder.h"
#include "stdbool.h"
#include "proj.h"

static const char* TAG = "ui_";

PROTOYPE_STATE(idle);
PROTOYPE_STATE(system);
PROTOYPE_STATE(splash);
PROTOYPE_STATE(menu);
PROTOYPE_STATE(about);
PROTOYPE_STATE(settings);
PROTOYPE_STATE(update);
PROTOYPE_STATE(file_browser);

ui_state_t* current_ui_state;
void ui_task(void* params);

//static ui_event_t button_event;

// void  button_handler_isr(void* aaargh){

//     if((uint32_t)aaargh != ROTOR_PUSH_BUTTON){
//         return;
//     }   
//     int level = gpio_get_level(ROTOR_PUSH_BUTTON);

//     if(level == 0){
//         // button pressed
//         button_event.type = UI_KEYDOWN;

//     } else {
//         // button released
//         button_event.type = UI_KEYUP;
//     }
//     xQueueSendFromISR(qRotor, &button_event, NULL);
// } 

void button_install(void){
        // set up IO36 as the switch input
    
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;//GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << ROTOR_PUSH_BUTTON);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // gpio_install_isr_service(0);
    // gpio_isr_handler_add(ROTOR_PUSH_BUTTON, button_handler_isr, (void*)ROTOR_PUSH_BUTTON);
}

#define UI_DELAY 100

void ui_init(void){

    //spawn ui task
    TaskHandle_t hUI_task;
    xTaskCreatePinnedToCore(ui_task, "ui_task", 20*1024, NULL, configMAX_PRIORITIES-2, &hUI_task, 1);

}

void set_next_state(ui_state_t* next_state){
    if(!next_state) return;
    if(next_state != current_ui_state){
        current_ui_state = next_state;
        if(current_ui_state && current_ui_state->init){
            current_ui_state->init();
        }
    }
}
////////////////////////////////////////////////////////////////////////////
void ui_task(void* params){

    current_ui_state = &splash_state;

    OLED_init();

    OLED_fill(0,7,NULL, false);
    
    init_rotary_encoder(NULL);

    button_install();
   
    ui_event_t evt;
    int held_ms = 0;

    while(1){

        // process tick
        if(current_ui_state && current_ui_state->tick){
            current_ui_state->tick(UI_DELAY);
        }

        // chechk for ui inputs
        evt.type = UI_NONE;
        while (xQueueReceive(qRotor, &evt, 0)){

            ESP_LOGI(TAG, "Rotor event type: %d", evt.type);
        }

        if(gpio_get_level(ROTOR_PUSH_BUTTON) == 0){
            // button is held down
            held_ms += UI_DELAY;
            if(held_ms > LONG_PRESS_DURATION){
                // long press, go to system menu
                held_ms = 0;
                set_next_state(&system_state);
            }
        } else {
            // button released
            if(held_ms >= MEDIUM_PRESS_DURATION){
                evt.type = UI_MEDIUM_PRESS;
                ESP_LOGI(TAG, "Medium");
            } else if(held_ms >= SHORT_PRESS_DURATION){
                evt.type = UI_SHORT_PRESS;
            }
   
            // reset held time counter
            evt.type = UI_NONE;
            held_ms = 0;
        }
        // process event
        if(evt.type != UI_NONE && current_ui_state->event_handler){
            current_ui_state->event_handler( evt);
            ESP_LOGI(TAG, "event handled. type: %d", evt.type);

        }
        
        vTaskDelay(UI_DELAY / portTICK_PERIOD_MS);
    }
}

/////////////////////////////////////////////////////////////////////////////

#define SPLASH_TIMEOUT_MS (1000 * 3) //30 seconds
static bool shown = false;
void splash_init(void){ 
    shown = false;
}
void splash_event_handler(ui_event_t event){ 
    /* any event exits */
    set_next_state(&menu_state);
}
void splash_tick(int milliseconds){ 
    static int timeout = 0;
    if(!shown){
        shown = true;
        splash_render();
    }    
    timeout += milliseconds;
    if(timeout >= SPLASH_TIMEOUT_MS){
        set_next_state(&idle_state);    
    }
 }
 void splash_render(void){ 
    OLED_Clear(0,7);
    OLED_WriteBig("Slow TV",       1, 3, false);
    OLED_WriteBig("(c)2013-2025",  3, 0, false);
    OLED_WriteBig("cmcq.com",      5, 2, false);
 }

/////////////////////////////////////////////////////////////////////////////
static int idle_idx = 0;
static int idle_ms = 0;
static bool invert_idle = true;
#define IDLE_FRAME_DURATION (40)
#define IDLE_NUM_FRAMES (24)
void idle_init(void){
    idle_render();

};
void idle_tick(int milliseconds){
    idle_ms += milliseconds;
    // if(idle_ms >= IDLE_FRAME_DURATION){
    //     idle_ms = 0;
    //     //idle_idx = (idle_idx + 1) % IDLE_NUM_FRAMES;
    //     idle_render();
    // }       
};
void idle_render(void){ 
    //OLED_fill(0, 7, (uint8_t*)cog3[idle_idx]);
    OLED_fill(0, 7, (uint8_t*)hi_rez_3cog[idle_idx], invert_idle);

    char buf[8];
    sprintf(buf, "%02d", idle_idx);
    OLED_WriteBig( buf, 0, 103, !invert_idle);
}
void idle_event_handler(ui_event_t evt){
    ESP_LOGI(TAG, "Idle event: %d", evt.type);
    if (evt.type == UI_ROTOR_DEC){

        idle_idx = (idle_idx - 1 + IDLE_NUM_FRAMES) % IDLE_NUM_FRAMES;
        
    }else if (evt.type == UI_ROTOR_INC){

        idle_idx = (idle_idx + 1) % IDLE_NUM_FRAMES;
    }
    idle_render();
}

void system_init(void){};
void system_handler(ui_event_t event){};
void system_tick(int milliseconds){};
void system_render(void){};

void menu_init(void){};
void menu_event_handler(ui_event_t event){};
void menu_tick(int milliseconds){};
void menu_render(void){};

void about_init(void){};
void about_event_handler(ui_event_t event){};
void about_tick(int milliseconds){};
void about_render(void){};

void settings_init(void){};
void settings_event_handler(ui_event_t event){};
void settings_tick(int milliseconds){};
void settings_render(void){};

void update_init(void){};
void update_event_handler(ui_event_t event){};
void update_tick(int milliseconds){};
void update_render(void){};

void file_browser_init(void){};
void file_browser_event_handler(ui_event_t event){};
void file_browser_tick(int milliseconds){};
void file_browser_render(void){};



/////////////////////////////////////////////////////////////////////////////

