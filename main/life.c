/*
Implements Conway's Game of life on 6*9 grid
*/


#include "tv.h"
#include "life.h"
#include "oled_i2c.h"
#include "ui/ui_state.h"

#define GRID_WIDTH (9)
#define GRID_HEIGHT (6)

static uint8_t life_grid[GRID_WIDTH * GRID_HEIGHT];
static uint8_t life_grid_tmp[GRID_WIDTH * GRID_HEIGHT];

static int ticks = 0;

void get_signs_of_life(uint8_t** grid){
    *grid = &life_grid[0];
}

void life_tick(void){

    if(ticks == 0){
        life_init();
        return;
    }
    update_grid();

    if(pop_count() == 0){
        random_seed(9);
    }

    ticks++;
}

void life_init(void){
    memset(life_grid, 0, sizeof(life_grid));
    memset(life_grid_tmp, 0, sizeof(life_grid_tmp));
    int midx = 6/2;
    int midy = 9/2;

    life_grid[ (midy*6) + midx ] = 1;
    life_grid[ ((midy-1)*6) + (midx+1) ] = 1;
    life_grid[ ((midy-2)*6) + (midx-1) ] = 1;
    life_grid[ ((midy-2)*6) + (midx) ] = 1;
    life_grid[ ((midy-2)*6) + (midx+1) ] = 1;

}   

void random_seed(int count){
    for(int i = 0; i < count; i++){
        int x = rand() % GRID_WIDTH;
        int y = rand() % GRID_HEIGHT;
        life_grid[ (y*6) + x ] = 1;
    }
}

int pop_count(void){
    int count = 0;
    for(int i = 0; i < sizeof(life_grid); i++){
        if(life_grid[i] == 1){ count++; }
    }
    return count;
}

void update_grid(void){

    for(int y = 0; y < 9; y++){
        for(int x = 0; x < 6; x++){

            int idx = (y*6) + x;
            int ncount = 0;
            // count neighbours
            for(int ny = -1; ny <= 1; ny++){
                for(int nx = -1; nx <= 1; nx++){
                    if(ny == 0 && nx == 0){ continue; } // skip self
                    int xx = (x + nx) % GRID_WIDTH;
                    int yy = (y + ny) % GRID_HEIGHT;
                    int nidx = (yy*6) + xx;
                    ncount += life_grid[nidx];
                }
            }
            // apply rules
            if(life_grid[idx] == 1){
                // live cell
                if(ncount < 2 || ncount > 3){
                    life_grid_tmp[idx] = 0; // dies
                }else{
                    life_grid_tmp[idx] = 1; // lives
                }
            }else{
                // dead cell
                if(ncount == 3){
                    life_grid_tmp[idx] = 1; // becomes alive
                }else{
                    life_grid_tmp[idx] = 0; // stays dead
                }
            }
        }
    }
    // copy tmp to main grid
    memcpy(life_grid, life_grid_tmp, sizeof(life_grid));
}