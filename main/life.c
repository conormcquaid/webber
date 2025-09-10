/*
Implements Conway's Game of life on 6*9 grid
*/

uint8_t life_grid[6*9];
uint8_t life_grid_tmp[6*9];
#include "tv.h"
#include "life.h"
#include "oled_i2c.h"
#include "ui_state.h"
#include "modes.h"  

const int grid_width = 9;
const int grid_height = 6;

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
        int x = esp_random() % grid_width;
        int y = esp_random() % grid_height;
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
                    int xx = (x + nx) % grid_width;
                    int yy = (y + ny) % grid_height;
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