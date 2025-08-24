#include "ui_state.h"
#include "oled_i2c.h"
#include "stdbool.h"
#include "cog2.h"
#include "cog3.h"

PROTOYPE_STATE(idle);
PROTOYPE_STATE(system);
PROTOYPE_STATE(splash);
PROTOYPE_STATE(menu);
PROTOYPE_STATE(about);
PROTOYPE_STATE(settings);
PROTOYPE_STATE(update);
PROTOYPE_STATE(file_browser);

ui_state_t* current_ui_state = &splash_state;

void set_next_state(ui_state_t* next_state){
    if(!next_state) return;
    if(next_state != current_ui_state){
        current_ui_state = next_state;
        if(current_ui_state && current_ui_state->init){
            current_ui_state->init();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

#define SPLASH_TIMEOUT_MS (1000 * 30) //30 seconds
static bool shown = false;
void splash_init(void){ 
    shown = false;
}
void splash_event_handler(ui_event_type_t event, void* param){ 
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
    OLED_WriteBig("Welcome to", 0, 0);
    OLED_WriteBig("Morse Tutor", 2, 0);
    OLED_WriteBig("By VK3PE", 4, 0);
 }

/////////////////////////////////////////////////////////////////////////////
void idle_init(void){};
void idle_event_handler(ui_event_type_t event, void* param){};
void idle_tick(int milliseconds){};
void idle_render(void){ 
    OLED_Clear(0,7);
    OLED_fill((uint8_t*)cog3);
}

void system_init(void){};
void system_handler(ui_event_type_t event, void* param){};
void system_tick(int milliseconds){};
void system_render(void){};

void menu_init(void){};
void menu_event_handler(ui_event_type_t event, void* param){};
void menu_tick(int milliseconds){};
void menu_render(void){};

void about_init(void){};
void about_event_handler(ui_event_type_t event, void* param){};
void about_tick(int milliseconds){};
void about_render(void){};

void settings_init(void){};
void settings_event_handler(ui_event_type_t event, void* param){};
void settings_tick(int milliseconds){};
void settings_render(void){};

void update_init(void){};
void update_event_handler(ui_event_type_t event, void* param){};
void update_tick(int milliseconds){};
void update_render(void){};

void file_browser_init(void){};
void file_browser_event_handler(ui_event_type_t event, void* param){};
void file_browser_tick(int milliseconds){};
void file_browser_render(void){};



/////////////////////////////////////////////////////////////////////////////

