#ifndef __I2C_OLED__
#define __I2C_OLED__

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rmt_struct.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <driver/gpio.h>

//totally unrelated to have here, but expedient :)
#define BUTTON_B GPIO_NUM_27
#define BUTTON_A GPIO_NUM_33

void OLED_init(void);
void OLED_fill(uint8_t start, uint8_t end, uint8_t* source);
void OLED_Clear(int start_line, int end_line);
void OLED_WriteBig( char* s, uint8_t line, uint8_t charpos);

#endif /* __I2C_OLED__ */