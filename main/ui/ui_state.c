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
#include "main.h"

static const char* TAG = "ui_";

PROTOYPE_STATE(idle);
PROTOYPE_STATE(internals);
PROTOYPE_STATE(splash);
PROTOYPE_STATE(menu);
PROTOYPE_STATE(about);
PROTOYPE_STATE(settings);
PROTOYPE_STATE(update);
PROTOYPE_STATE(file_browser);
PROTOYPE_STATE(error);
PROTOYPE_STATE(projector);


ui_state_t* current_ui_state;
void ui_task(void* params);



void button_install(void){
        // set up IO36 as the switch input
    
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;//GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << ROTOR_PUSH_BUTTON);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));

}

#define UI_DELAY 25

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

        //
        if(!current_ui_state || !current_ui_state->event_handler){
            vTaskDelay(UI_DELAY / portTICK_PERIOD_MS);
            set_next_state(&error_state);
            continue;
        }

        // check for ui inputs
        evt.type = UI_NONE;

        // button events
        if(gpio_get_level(ROTOR_PUSH_BUTTON) == 0){
            // button is held down
            held_ms += UI_DELAY;
            if(held_ms > LONG_PRESS_DURATION){
                // long press, go to system menu
                held_ms = 0;
                set_next_state(&settings_state);
            }
        } else {
            // button released
            if(held_ms > LONG_PRESS_DURATION){
                // long press, go to system menu
                set_next_state(&settings_state);

            } else if(held_ms >= MEDIUM_PRESS_DURATION){

                evt.type = UI_MEDIUM_PRESS;
                ESP_LOGI(TAG, "Medium");

            } else if(held_ms >= SHORT_PRESS_DURATION){
                evt.type = UI_SHORT_PRESS;
            }
   
            if(evt.type != UI_NONE){
                current_ui_state->event_handler( evt);
            }
            held_ms = 0;
        }
        // process any rotor events
        while (xQueueReceive(qRotor, &evt, 0)){

            ESP_LOGI(TAG, "Rotor event type: %d", evt.type);
            current_ui_state->event_handler( evt);
        }
        
        vTaskDelay(UI_DELAY / portTICK_PERIOD_MS);
    }
}

/////////////////////////////////////////////////////////////////////////////

#define SPLASH_TIMEOUT_MS (1000 * 3) //1000 * seconds
//static bool shown = false;
static int splash_duration = 0;
void splash_init(void){ 
    OLED_Clear(0,7);
}
void splash_event_handler(ui_event_t event){ 
    /* any event exits */
    set_next_state(&menu_state);
}
void splash_tick(int milliseconds){ 
    
    // if(!shown){
    //     shown = true;
         splash_render();
    // }    
    splash_duration += milliseconds;
    if(splash_duration >= SPLASH_TIMEOUT_MS){
        set_next_state(&projector_state);    
    }
 }
 void splash_render(void){ 
    static bool once = false;
    if(!once){
        OLED_Clear(0,7);
        OLED_WriteBig("Slow TV",       1, 3, false);
        OLED_WriteBig("(c)2013-2025",  3, 0, false);
        OLED_WriteBig("cmcq.com",      5, 2, false);
        once = true;
    }
 }

/////////////////////////////////////////////////////////////////////////////
static int projector_millis;
static int projector_frame;
#define PROJECTOR_FRAMES (21)
#define PROJ_MS_DELAY (120)

void projector_init(void){
    OLED_Clear(0,7);
};
void projector_event_handler(ui_event_t event){};

void projector_tick(int milliseconds){
    projector_millis += milliseconds;
    if(projector_millis > PROJ_MS_DELAY){
        if(projector_frame < PROJECTOR_FRAMES){
            projector_render();
            projector_millis = 0;
        }else{
            set_next_state(&idle_state);
        }
    }
};
void projector_render(void){

    OLED_fill2(22, 22 + 83, 0, 7, &projector[projector_frame][0], true);
    projector_frame++;
};

/////////////////////////////////////////////////////////////////////////////
static int idle_idx = 0;
static int idle_ms = 0;
static bool invert_idle = true;
static bool asleep = false;
#define IDLE_FRAME_DURATION (120)
#define IDLE_NUM_FRAMES (24)
#define IDLE_TIMEOUT (15 * 1000) /* 15 seconds before sleeping */
void idle_init(void){

    OLED_fill(0, 7, NULL, 0); 
    idle_render();

};
void idle_tick(int milliseconds){

    if(asleep) return;

    idle_ms += milliseconds;
    if(idle_ms > IDLE_TIMEOUT){
        asleep = true;
        idle_ms = 0;
        OLED_fill(0, 7, NULL, 0); 
    }    
     
};
void idle_render(void){ 

    OLED_fill(0, 7, (uint8_t*)hi_rez_3cog[idle_idx], invert_idle);

    char buf[8];
    sprintf(buf, "%02d", idle_idx);
    OLED_WriteBig( buf, 0, 100, !invert_idle);
}
void idle_event_handler(ui_event_t evt){

    asleep = false;
    idle_ms = 0;

    ESP_LOGI(TAG, "Idle event: %d", evt.type);
    if (evt.type == UI_ROTOR_DEC){

        idle_idx = (idle_idx - 1 + IDLE_NUM_FRAMES) % IDLE_NUM_FRAMES;
        
    }else if (evt.type == UI_ROTOR_INC){

        idle_idx = (idle_idx + 1) % IDLE_NUM_FRAMES;
    }else if(evt.type == UI_SHORT_PRESS){
        set_next_state(&menu_state);
        return;
    }else if(evt.type == UI_LONG_PRESS){
        set_next_state(&settings_state);
        return;
    }
    idle_render();
}
/////////////////////////////////////////////////////////////////////////////
void internals_init(void){};
void internals_event_handler(ui_event_t event){};
void internals_tick(int milliseconds){};
void internals_render(void){};

/////////////////////////////////////////////////////////////////////////////
static int menu_millis;
static int menu_sel;
char title_str[32];
char tween_str[16];
char fps_str[16];
int nMenu = 4;
bool selecting = false;
enum{NONE, TWEEN, FPS, THING3, THING4};

void menu_init(void){
    OLED_Clear(0,7);
    
    menu_sel = 0;
    
    menu_render();
};
void menu_event_handler(ui_event_t event){
    if(selecting){
        if(event.type == UI_ROTOR_INC){ menu_sel = (menu_sel + 1) % nMenu;}
        else if(event.type == UI_ROTOR_INC){ menu_sel = (menu_sel +nMenu - 1) % nMenu;}
        else if(event.type == UI_SHORT_PRESS){ selecting = false; }
    }else{
        //editing
        switch(menu_sel){
            case TWEEN:

            break;

            case FPS:
            break;

            case THING3:
            break;

            case THING4:
            break;
        }
        if(event.type == UI_MEDIUM_PRESS){ selecting = true; }
    }
    menu_render();

};
void menu_tick(int milliseconds){
    menu_millis += milliseconds;
};
void menu_render(void){    /////////////////////////////TODO: what if there is no memory card!!

    sprintf(title_str, "%.20s %02d", &tv_status.current_file[0], (int)(100.0 * tv_status.frame_number / tv_status.file_frame_count));
    sprintf(tween_str, "tweens %02d", tv_config_block.speed.tweens);

    OLED_WriteSmall(title_str, 0, 0, (menu_sel == 1));
    OLED_WriteSmall(tween_str, 2, 0, (menu_sel == 2));
    OLED_WriteSmall("Thomas!", 4, 0, (menu_sel == 3));

    OLED_WriteSmall("line 1", 5, 0, 0);
    OLED_WriteSmall("another line", 6, 0, 1);
    OLED_WriteSmall("12345678901234567890", 7, 0, 0);

};

void about_init(void){};
void about_event_handler(ui_event_t event){};
void about_tick(int milliseconds){};
void about_render(void){};

//////////////////////////////////////////////////////////////////////////////////
// set the config parameters for tv
#define CONFIG_IDLE_TIMEOUT (15 * 1000 )
static int config_millis = 0;
char* config_items[] = {
    "Resolution",
    "Speed",
    "Bytes/LED",
    "Rotor Dir",
    "Rotor Clicks"

};
int nItems = sizeof(config_items)/sizeof(config_items[0]);
int selected_line = 0;
typedef enum{ SELECTING, EDITING} config_mode_t;
config_mode_t config_mode = SELECTING;

void settings_init(void){
    OLED_Clear(0,7);
    settings_render();

}
void settings_event_handler(ui_event_t event){
    if(event.type == UI_ROTOR_DEC){
        selected_line = (selected_line + nItems - 1) % nItems;
        if(selected_line < 0) selected_line = 0;
    }else if(event.type == UI_ROTOR_INC){
        selected_line = (selected_line + 1) % nItems;
    }else if(event.type == UI_LONG_PRESS){
        set_next_state(&menu_state);
        return;
    }
    settings_render();
    config_millis = 0;
}
void settings_tick(int milliseconds){

    config_millis += milliseconds;
    if(config_millis > CONFIG_IDLE_TIMEOUT){
        set_next_state(&idle_state);
    }
}
void settings_render(void){

    OLED_Clear(0,7);
    OLED_WriteBig("Settings", 0, 3, false);
    for(int i = 0; i < nItems; i++){
        OLED_WriteBig(config_items[i], 2 + i*2, 0, (i == selected_line));
    }
}
//////////////////////////////////////////////////////////////////////////////////

void update_init(void){};

void update_event_handler(ui_event_t event){
    if(event.type == UI_ROTOR_DEC){
        selected_line = (selected_line - 1 + sizeof(config_items)/sizeof(config_items[0])) % (sizeof(config_items)/sizeof(config_items[0]));
        update_render();
    
    }
};
void update_tick(int milliseconds){};
void update_render(void){};

void file_browser_init(void){};
void file_browser_event_handler(ui_event_t event){};
void file_browser_tick(int milliseconds){};
void file_browser_render(void){};

void error_init(void){
    error_render();
}
void error_event_handler(ui_event_t event){};
void error_tick(int milliseconds){};
void error_render(void){
    OLED_Clear(0,7);
    OLED_WriteBig("Error",       1, 3, false);
    OLED_WriteBig("State",       3, 3, false);
    OLED_WriteBig("No UI",       5, 2, false);
};

/////////////////////////////////////////////////////////////////////////////

