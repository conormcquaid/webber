#ifndef __MAIN_H__
#define __MAIN_H__

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define CONFIG_WEBSOCKET_URI "ws://tv-remote.local"
#define NO_DATA_TIMEOUT_SECS 10

#define MDNS_HOSTNAME "tv-remote"


void wifi_manager_task( void* nothing);
void start_webserver(void);

// // how long to hold GPIO0 button before factory reset
// #define RESET_PRESS_SECONDS 5

// typedef union{
//     uint32_t color;
//     struct {
//         uint8_t r;
//         uint8_t g;
//         uint8_t b;
//         uint8_t a;
//     };
// } RGBColor;

// typedef struct{
//     uint8_t* buffer;
//     uint32_t length;
// }udp_tx_buffer;

// #define MAX_OSC_PATH 256
// extern char osc_address[MAX_OSC_PATH];

// // rotary encoder notifier
// extern QueueHandle_t qRotor;
// extern QueueHandle_t qHue;

// esp_err_t wifi_init_sta(char* ssid, char* pwd);
// void captiveportal(void);

// void set_the_led(uint8_t r, uint8_t g, uint8_t b);

#endif /* __MAIN_H__ */