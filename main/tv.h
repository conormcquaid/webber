#ifndef __TV_H__
#define __TV_H__

#include "main.h"


typedef enum{
    TV_STATE_IDLE,
    TV_STATE_PLAYING,
    TV_STATE_PAUSED,
    TV_STATE_STOPPED,
    TV_STATE_ERROR  
}tv_state_t;


TaskHandle_t tv_init(tv_runtime_status_t* tv_status);
void tv_play_file(const char* path);
void tv_set_mode(tv_mode_t mode);
void tv_set_state(tv_state_t state);
tv_mode_t tv_get_mode(void);
void tv_task(void* p);
void tv_set_speed(tv_speed_t speed);
tv_speed_t tv_get_speed(void);
tv_state_t* tv_get_state(void);
tv_runtime_status_t* tv_get_runtime(void);

float tv_get_brightness(void);
void  tv_set_brightness(float b);

void tv_goto_next(void);
void tv_goto_prev(void);


#endif /* __TV_H__ */