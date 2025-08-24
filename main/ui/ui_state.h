#ifndef __UI_STATE_H__
#define __UI_STATE_H__

typedef enum {
    UI_KEYPRESS,
    UI_ROTOR_INC,
    UI_ROTOR_DEC
}ui_event_type_t;

typedef struct {
    ui_event_type_t type;
    void* param;
}ui_event_t;

typedef struct{
    void (*init)(void);
    void (*event_handler)(ui_event_type_t event, void* param);
    void (*tick)(int millis);
    void (*render)(void);
} ui_state_t;

//extern ui_state_t idle_state, system_state, splash_state, menu_state, about_state, settings_state, update_state, file_browser_state;
extern ui_state_t* current_ui_state;

#define PROTOYPE_STATE(STATE_NAME) \
void STATE_NAME ## _init(void); \
void STATE_NAME ## _event_handler(ui_event_type_t event, void* param); \
void STATE_NAME ## _tick(int milliseconds); \
void STATE_NAME ## _render(void); \
\
     ui_state_t STATE_NAME ## _state   = { \
    .init = STATE_NAME ## _init, \
    .event_handler = STATE_NAME ## _event_handler, \
    .tick = STATE_NAME ## _tick, \
    .render = STATE_NAME ## _render \
    }  


// PROTOYPE_STATE(idle);
// PROTOYPE_STATE(system);
// PROTOYPE_STATE(splash);
// PROTOYPE_STATE(menu);
// PROTOYPE_STATE(about);
// PROTOYPE_STATE(settings);
// PROTOYPE_STATE(update);
// PROTOYPE_STATE(file_browser);


//TODO: including a render fumction may be unnecessary

#endif /* __UI_STATE_H__ */