/*
 * sdcard.h
 *
 *  Created on: Apr 8, 2017
 *      Author: Conor
 */

#ifndef MAIN_SDCARD_H_
#define MAIN_SDCARD_H_

#include <stdint.h>
#include <esp_err.h>

esp_err_t init_sd(void);

int getFrame(uint8_t * pFrame);

char* getNextFilename(char* f, size_t l);

int set_cur_file(char *fname);

void open_next_file(void);

#endif /* MAIN_SDCARD_H_ */