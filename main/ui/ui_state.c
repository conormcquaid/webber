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
#include "dial.h"
#include "main.h"
#include "tv.h"
#include "ui/icons2.h"
#include "ui/projector.h"
#include "creds.h"

static const char* TAG = "ui_";

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))

#define MENU_TIMEOUT ( 30 * 1000 )
static int menu_timeout;

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
PROTOYPE_STATE(brightness);
PROTOYPE_STATE(tweens);
PROTOYPE_STATE(prev_next);
PROTOYPE_STATE(millis_dial);
PROTOYPE_STATE(mode);


ui_state_t* current_ui_state;
void ui_task(void* params);
static tv_runtime_status_t* pTV;

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

#define UI_DELAY 10

TaskHandle_t ui_init(tv_runtime_status_t* pTV_status){

    //spawn ui task
    TaskHandle_t hUI_task;
    xTaskCreatePinnedToCore(ui_task, "ui_task", 20*1024, pTV_status, configMAX_PRIORITIES-9, &hUI_task, 1);

    return hUI_task;
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

extern void init_rot_encoder_shitty(void);


void ui_task(void* params){

    pTV = (tv_runtime_status_t*) params;

    current_ui_state = &splash_state;

    OLED_init();

    OLED_fill(0,7,NULL, false);
    
   // init_rotary_encoder(NULL);
   // init_rot_encoder_shitty();

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
                continue;
            }
        } else {
            // button released
            if(held_ms > LONG_PRESS_DURATION){
                evt.type = UI_LONG_PRESS;
                ESP_LOGI(TAG, "Long");

            } else if(held_ms >= MEDIUM_PRESS_DURATION){

                evt.type = UI_MEDIUM_PRESS;
                ESP_LOGI(TAG, "Medium");

            } else if(held_ms >= SHORT_PRESS_DURATION){
                evt.type = UI_SHORT_PRESS;
                ESP_LOGI(TAG, "Short");
            }
   
            if(evt.type != UI_NONE){
                current_ui_state->event_handler( evt);
            }
            held_ms = 0;
        }
        // process any rotor events
        while (xQueueReceive(qRotor, &evt, 0)){

            ESP_LOGI(TAG, "Rotor event %s", (evt.type == UI_ROTOR_INC) ? "INC" : "DEC");
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
static int projector_frame = 0;
#define PROJECTOR_FRAMES (21)
#define PROJ_MS_DELAY (120)

void projector_init(void){
    OLED_fill(0, 7, NULL, true);
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

    OLED_fill2(22, 22 + 82, 0, 7, &projector[projector_frame][0], true);
    projector_frame++;
};

/////////////////////////////////////////////////////////////////////////////
static int idle_ms = 0;
static bool invert_idle = false;
static bool asleep = false;
#define IDLE_FRAME_DURATION (120)
#define IDLE_NUM_FRAMES (24)
#define IDLE_TIMEOUT (15 * 1000) /* 15 seconds before sleeping */
void idle_init(void){

    OLED_fill(0, 7, NULL, 0); 
    idle_render();
    idle_ms = 0;
    asleep = false;
};

void idle_tick(int milliseconds){

    idle_ms += milliseconds;
    if(idle_ms > IDLE_TIMEOUT){
        asleep = true;
        idle_ms = 0;
        OLED_fill(0, 7, NULL, 0); 
    }    
     
};
void idle_render(void){ 

    OLED_WriteSmall("Now Playing:", 0, 0, invert_idle);

    tv_mode_t mode = tv_get_mode();
    if(mode == TV_MODE_OFF){
        OLED_WriteSmall("TV Off", 2, 0, invert_idle);
    }else if(mode == TV_MODE_LAMP){
        OLED_WriteSmall("Lamp Mode", 2, 0, invert_idle);
    }else if(mode == TV_MODE_LIFE){
        OLED_WriteSmall("Life Mode", 2, 0, invert_idle);
    }else{
        OLED_WriteBig(&pTV->current_file[0], 2, 0, invert_idle);
        
        uint8_t pct = (100.0 * pTV->frame_number) / pTV->file_frame_count ;

        char buf[8];
        snprintf(buf, 8, "% 3d%%", pct);
        OLED_WriteBig( buf, 5, 1, false);
    }


    if(wifi_status.ip_addr[0] != 0){
        OLED_WriteSmall(wifi_status.ip_addr, 7, 0, false);
    }
    
}
void idle_event_handler(ui_event_t evt){

    idle_ms = 0;

    switch(evt.type){
        
        case UI_ROTOR_INC:
        case UI_ROTOR_DEC:
        case UI_SHORT_PRESS:
        if(asleep){
            asleep = false;
        }else{
            set_next_state(&tweens_state);
            return;
        }
            break;
        case UI_MEDIUM_PRESS:
            set_next_state(&tweens_state);
            return;
            break;
        case UI_LONG_PRESS:
        set_next_state(&settings_state);
            return;
            break;
        default:
            break;
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
    if(menu_millis > MENU_TIMEOUT){
        set_next_state(&idle_state);
        return;
    }   
};
void menu_render(void){    /////////////////////////////TODO: what if there is no memory card!!

    sprintf(title_str, "%.20s %02d", &pTV->current_file[0], (int)(100.0 * pTV->frame_number / pTV->file_frame_count));
    sprintf(tween_str, "tweens %02d", 1234);

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
// FACTORY MENU
// [0] exit
// [1] configure hardware
// [2] factory reset




// set the config parameters for tv
// rotor chooses which option is highlighted, short press changes value
// all options are bi-valent

#define CONFIG_IDLE_TIMEOUT (15 * 1000 )
static int config_millis = 0;
enum{EDITING, SAVING}editSaveMode = EDITING;

char* config_items[] = {
    "nLEDs", 
    "LED[]",
    "Dir",
    "Count"
};
int nItems = sizeof(config_items)/sizeof(config_items[0]);
int selected_line = 0;


void settings_init(void){
    OLED_Clear(0,7);
    settings_render();
    config_millis = 0;
    editSaveMode = EDITING;
    selected_line = 0;

}
void settings_event_handler(ui_event_t event){

    switch(event.type){
        case UI_ROTOR_INC:
            selected_line = (selected_line + 1) % (sizeof(config_items)/sizeof(config_items[0]));
            break;
        case UI_ROTOR_DEC:  
            selected_line = (selected_line - 1 + sizeof(config_items)/sizeof(config_items[0])) % (sizeof(config_items)/sizeof(config_items[0]));
            break;
        case UI_SHORT_PRESS:
            switch(selected_line){
                case 0:
                    tv_hw_config.resolution = (tv_hw_config.resolution == TV_RESOLUTION_HIGH) ? TV_RESOLUTION_LOW : TV_RESOLUTION_HIGH;
                    break;
                case 1:
                    tv_hw_config.bytes_per_LED = (tv_hw_config.bytes_per_LED == 3) ? 4 : 3;
                    break;
                case 2:
                    tv_hw_config.rotor_dir = (tv_hw_config.rotor_dir == ROTOR_DIR_CW) ? ROTOR_DIR_CCW : ROTOR_DIR_CW;
                    break;
                case 3:
                    tv_hw_config.rotor_clicks = (tv_hw_config.rotor_clicks == ROTOR_CLICKS_TWO) ? ROTOR_CLICKS_FOUR : ROTOR_CLICKS_TWO;
                    break;
            }
            break;
        case UI_MEDIUM_PRESS:
            selected_line = 0;
            editSaveMode = SAVING;
            break;
        case UI_LONG_PRESS:
            set_next_state(&menu_state);
            return;
            break;
        default:
            break;
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
    if(editSaveMode == SAVING){
        OLED_WriteBig("Saving...", 3, 3, false);
        save_hw_config(&tv_hw_config);
        ESP_LOGI(TAG, "Saving config to NVS");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        set_next_state(&idle_state);
        return;
    }
    char buf[12];
    sprintf(buf, "nLEDs %d", tv_hw_config.resolution == TV_RESOLUTION_HIGH ? FRAME_RESOLUTION_HIGH : FRAME_RESOLUTION_LOW);
    OLED_WriteBig(buf, 0, 0, (selected_line == 0 ));
    sprintf(buf, "LED[%d]",  tv_hw_config.bytes_per_LED == 3 ? 3 : 4);
    OLED_WriteBig(buf, 2, 0, (selected_line == 1 ));
    sprintf(buf, "Dir %cCW", tv_hw_config.rotor_dir == ROTOR_DIR_CW ? ' ' : 'C');
    OLED_WriteBig(buf, 4, 0, (selected_line == 2 ));
    sprintf(buf, "Clicks %d",tv_hw_config.rotor_clicks == ROTOR_CLICKS_TWO ? 2 : 4);
    OLED_WriteBig(buf, 6, 0, (selected_line == 3 ));
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
#include "bright.h"
static float brightness;
static float bright_increment = 0.01;

void brightness_init(void){

    brightness = tv_get_brightness();
    
    OLED_Clear(0,7);
    menu_timeout = 0;
    brightness_render();
};
void brightness_event_handler(ui_event_t event){

    menu_timeout = 0;

    if(event.type == UI_ROTOR_DEC){
        brightness = brightness - bright_increment;
        brightness = MAX(0, brightness);
        
    }else if(event.type == UI_ROTOR_INC){
        brightness = brightness + bright_increment;
        brightness = MIN(1.0, brightness);

    }else if(event.type == UI_SHORT_PRESS || event.type == UI_MEDIUM_PRESS){

        set_next_state(&prev_next_state);
        return;
    }else if(event.type == UI_LONG_PRESS){
        set_next_state(&settings_state);
        return;
    }
    tv_set_brightness(brightness);;
    brightness_render();
};
void brightness_tick(int milliseconds){
    menu_timeout += milliseconds;
    if(menu_timeout > MENU_TIMEOUT){
        set_next_state(&idle_state);
    }
};
void brightness_render(void){

   // OLED_Clear(0,7);
   // OLED_fill(4,7,NULL, false);

    OLED_fill2(0, 127, 0, 3, bright_bg, true);

    int bar_width = (int)(brightness * 127.0);

    for(int i =0; i < 4; i++){
        OLED_fill2( 0, bar_width, 4 + i, 4 + i, &bright_lvl[i * 128], true);
    }
    OLED_fill2(bar_width, 127, 4, 7, NULL, false);

//     OLED_fill2(0, 127, 4, 7, bright_lvl, true);
// // obscure RHS of bar
//     OLED_fill2(bar_width, 127, 4, 7, NULL, false);

    char b[6];
    snprintf(b, 6, "%3.0f%%", brightness * 100);
    OLED_WriteSmall(b, 0, 12, 0);


};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// tv_speed_t tv_speed;
// enum{TWEENS, MILLIS}edit_state;

// void tweens_init(void){

//     OLED_Clear(0,7);
//     menu_timeout = 0;
//     tv_speed = tv_get_speed();
//     edit_state = TWEEN;
//     tweens_render();
// };
// void tweens_event_handler(ui_event_t event){

//     menu_timeout = 0;
//     uint8_t item;
//     item = (edit_state == TWEENS) ? tv_speed.tweens : tv_speed.interframe_millis; 
    
//     if(event.type == UI_ROTOR_DEC){
//         item = item - 1;
//         item = MAX(1, item);
//     }else if(event.type == UI_ROTOR_INC){
//         item = item + 1;
//         item = MIN(99, item);

//     }else if(event.type == UI_SHORT_PRESS ){

//         edit_state = 1 - edit_state;
//         item = (edit_state == TWEENS) ? tv_speed.tweens : tv_speed.interframe_millis;

//     }
//     else if(event.type == UI_MEDIUM_PRESS){
//         set_next_state(&prev_next_state);
//         return;

//     }else if(event.type == UI_LONG_PRESS){
//         //TODO: system mode??
//     }

//     if(edit_state == TWEENS){ tv_speed.tweens = item; } else {tv_speed.interframe_millis = item;}

//     tv_set_speed(tv_speed);
//     tweens_render();
// };
// void tweens_tick(int milliseconds){
//     menu_timeout += milliseconds;
//     if(menu_timeout > MENU_TIMEOUT){
//         set_next_state(&idle_state);
//     }
// };
// void tweens_render(void){

//     OLED_WriteSmall("tweens", 1, 0, 0);
//     OLED_WriteSmall("millis", 1, 15, 0);
//     char b[4];
//     snprintf(b, 4, "%2d", tv_speed.tweens);
//     OLED_WriteBig(b, 4, 1, (edit_state == TWEENS));
//     snprintf(b, 4, "%2d", tv_speed.interframe_millis);
//     OLED_WriteBig(b, 4, 8, (edit_state == MILLIS));
// };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static enum{PREV, NO_CHANGE, NEXT, NO_CHANGE2}pi_choices;
static uint8_t nchoices = 4;

void prev_next_init(void){
    OLED_Clear(0,7);
    menu_timeout = 0;
    pi_choices = NO_CHANGE;
    prev_next_render();
}

void prev_next_event_handler(ui_event_t event){
    menu_timeout = 0;

    if(event.type == UI_ROTOR_DEC){
        pi_choices = (pi_choices + nchoices - 1) % nchoices;
    }else if(event.type == UI_ROTOR_INC){

        pi_choices = (pi_choices + 1) % nchoices;

    }else if(event.type == UI_SHORT_PRESS){

        switch(pi_choices){

            case PREV:
            ESP_LOGI(TAG, "Go to previous file");
            tv_goto_prev();
            break;
            case NEXT:
            ESP_LOGI(TAG, "Go to next file");
            tv_goto_next();
            break;
            
            default:
            case NO_CHANGE2:
            case NO_CHANGE:
                set_next_state(&mode_state);
                return;
                break;
            
            break;
        }
        char buf[16];
        memcpy((void*)buf, (void*)pTV->current_file, 16);
        OLED_WriteSmall(buf, 7, 0, false);

    }
    else if(event.type == UI_MEDIUM_PRESS){
        set_next_state(&mode_state);
        return;

    }else if(event.type == UI_LONG_PRESS){

        set_next_state(&settings_state);
        return;

    }
    
    prev_next_render();    
}

void prev_next_tick(int milliseconds){
        menu_timeout += milliseconds;
    if(menu_timeout > MENU_TIMEOUT){
        set_next_state(&idle_state);
    }
}
void prev_next_render(void){

    // w = 128. (32 * 3) = 96. 128 - 96 = 32. 32 /4 = 8
    // [8] [32] [8] [32] [8] [32] [8]
    OLED_fill2(8,   8 + 31, 2, 5, prev_small_png, (pi_choices != PREV) );
    //OLED_fill2(48, 48 + 31, 2, 6, (playing ? play_small_png : pause_small_png), (pi_choices != PLAY_PAUSE) );
    OLED_fill2(88, 88 + 31, 2, 5, next_small_png, (pi_choices != NEXT) );
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "modes.h"

//static enum{mSEQ, mRAND, mLOOP, mLAMP, mLIFE, mOFF}mode_choices;
tv_mode_t mode_choice;
static uint8_t mode_choices_sz = 6;

void mode_init(void){
    OLED_Clear(0,7);
    menu_timeout = 0;
    mode_choice = tv_get_mode();
    mode_render();
}

void mode_event_handler(ui_event_t event){
    menu_timeout = 0;

    if(event.type == UI_ROTOR_DEC){
        mode_choice = (mode_choice + mode_choices_sz - 1) % mode_choices_sz;
    }else if(event.type == UI_ROTOR_INC){

        mode_choice = (mode_choice + 1) % mode_choices_sz;

    }else if(event.type == UI_SHORT_PRESS){

        if(mode_choice != tv_get_mode()){
            tv_set_mode(mode_choice);
        }
        
        set_next_state(&idle_state);
        return;

    }
    else if(event.type == UI_MEDIUM_PRESS){
        set_next_state(&idle_state);
        return;

    }else if(event.type == UI_LONG_PRESS){

        set_next_state(&settings_state);
        return;

    }
    
    mode_render();    
}

void mode_tick(int milliseconds){
    
    menu_timeout += milliseconds;
    if(menu_timeout > MENU_TIMEOUT){
        set_next_state(&idle_state);
    }
}
void mode_render(void){

    OLED_fill2(8,   8 + 31, 0, 3, seq,      (mode_choice == TV_MODE_SEQUENTIAL) );
    OLED_fill2(48, 48 + 31, 0, 3, loop,     (mode_choice == TV_MODE_LOOP) );
    OLED_fill2(88, 88 + 31, 0, 3, randmode, (mode_choice == TV_MODE_RANDOM) );
    
    OLED_fill2(8,   8 + 31, 4, 7, lamp, (mode_choice == TV_MODE_LAMP) );
    OLED_fill2(48, 48 + 31, 4, 7, life, (mode_choice != TV_MODE_LIFE) );
    //OLED_fill2(88, 88 + 31, 4, 7, NULL, (mode_choice != TV_MODE_OFF) );

    OLED_WriteBig("OFF", 5, 9, (mode_choice == TV_MODE_OFF));

    // w = 128. (32 * 3) = 96. 128 - 96 = 32. 32 /4 = 8
    // [8] [32] [8] [32] [8] [32] [8]
    // OLED_fill2(8,   8 + 31, 2, 6, prev_small_png, (pi_choices != PREV) );
    // OLED_fill2(48, 48 + 31, 2, 6, (playing ? play_small_png : pause_small_png), (pi_choices != PLAY_PAUSE) );
    // OLED_fill2(88, 88 + 31, 2, 6, next_small_png, (pi_choices != NEXT) );
}



/// TODO add menus for playback mode seq/loop/rand

// add menu for tv mode : lamp, life, tv, ??

        // uint8_t play_small_png[128] 
        // /* Image: next_small.png, width 32, height 32 */
        // uint8_t next_small_png[128]
        // /* Image: prev_small.png, width 32, height 32 */
        // uint8_t prev_small_png[128] 
        // /* Image: pause_small.png, width 32, height 32 */
        // uint8_t pause_small_png[128] 
        // /* Image: rand2.png, width 32, height 32 */
        // uint8_t rand2_png[128] 
        // /* Image: rand.png, width 32, height 32 */
        // uint8_t rand_png[128] 
        // /* Image: seq.png, width 32, height 32 */
        // uint8_t seq_png[128] 

///////////////////////////////////////////////////////////////////////////////////////////////////////
static int tween_idx = 0;
static bool invert_tween  = false;
#define TWEEN_NUM_FRAMES (25)
static tv_speed_t speed;

void tweens_init(void){

    OLED_Clear(0,7);
    menu_timeout = 0;
    speed = tv_get_speed();
    tween_idx = speed.tweens % TWEEN_NUM_FRAMES;
    tweens_render();
};

void tweens_tick(int milliseconds){

    menu_timeout += milliseconds;
    if(menu_timeout > MENU_TIMEOUT){
        tv_set_speed(speed);
        set_next_state(&idle_state);
    }    
     
};
void tweens_render(void){ 

    OLED_fill(0, 7, (uint8_t*)hi_rez_3cog[tween_idx], invert_tween);

    char buf[8];
    snprintf(buf, 8, "% d", speed.tweens);
    OLED_WriteBig( buf, 0, 9, !invert_tween);
}
void tweens_event_handler(ui_event_t evt){

    menu_timeout = 0;

    switch(evt.type){
        case UI_ROTOR_DEC:
            tween_idx = (tween_idx - 1 + TWEEN_NUM_FRAMES) % TWEEN_NUM_FRAMES;
            speed.tweens -= 1;
            speed.tweens = MIN(99, speed.tweens);
            
        break;
        case UI_ROTOR_INC:
            tween_idx = (tween_idx + 1) % TWEEN_NUM_FRAMES;
            speed.tweens += 1;
            speed.tweens = MAX(1, speed.tweens);
        break;
        case UI_SHORT_PRESS:
        case UI_MEDIUM_PRESS:
            ESP_LOGI(TAG, "Press!");
            tv_set_speed(speed);
            set_next_state(&millis_dial_state);
            return;
        break;
        case UI_LONG_PRESS:
            set_next_state(&settings_state);
            return;
        break;
        default:
            return;
    }   
    tweens_render();
}
/////////////////////////////////////////////////////////////////////////////
static int millis_dial_idx = 0;
static bool invert_dial = true;
#define DIAL_NUM_FRAMES (20)

void millis_dial_init(void){

    OLED_Clear(0,7);
    menu_timeout = 0;
    speed = tv_get_speed();
    millis_dial_render();
};
void millis_dial_tick(int milliseconds){

    menu_timeout += milliseconds;
    if(menu_timeout > MENU_TIMEOUT){
        tv_set_speed(speed);
        set_next_state(&idle_state);
    }    
     
};
void millis_dial_render(void){ 

    OLED_fill(0, 7, (uint8_t*)dial[millis_dial_idx], invert_dial);

    char buf[8];
    snprintf(buf, 8, "% d", speed.interframe_millis);
    OLED_WriteBig( buf, 0, 9, !invert_dial);
}
void millis_dial_event_handler(ui_event_t evt){

    menu_timeout = 0;

    ESP_LOGI(TAG, "dial event: %d", evt.type);

    switch(evt.type){
        case UI_ROTOR_INC:
            millis_dial_idx = (millis_dial_idx - 1 + DIAL_NUM_FRAMES) % DIAL_NUM_FRAMES;
            speed.interframe_millis = MIN(99, speed.interframe_millis + 1);
            
        break;
        case UI_ROTOR_DEC:
            millis_dial_idx = (millis_dial_idx + 1) % DIAL_NUM_FRAMES;
            speed.interframe_millis = MAX(1, speed.interframe_millis - 1);
            
        break;
        case UI_SHORT_PRESS:
        case UI_MEDIUM_PRESS:
            tv_set_speed(speed);
            set_next_state(&brightness_state);
            return;
        break;
        case UI_LONG_PRESS:
            set_next_state(&settings_state);
            return;
        break;
        default:
            return;
    }   

    millis_dial_render();
}