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





void tv_init(void);
void tv_play_file(const char* path);
void tv_set_mode(tv_mode_t mode);
void tv_set_state(tv_state_t state);
tv_mode_t tv_get_mode(void);
void tv_task(void* p);
void tv_set_speed(tv_speed_t speed);
tv_speed_t tv_get_speed(void);
tv_state_t tv_get_state(void);


#endif /* __TV_H__ */