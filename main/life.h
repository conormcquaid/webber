#ifndef __LIFE_H__
#define __LIFE_H__

#include "stdint.h"

void life_init(void);
void random_seed(int count);
int pop_count(void);
void update_grid(void); 
void life_tick(void);
void get_signs_of_life(uint8_t** grid);

#endif /* __LIFE_H__ */