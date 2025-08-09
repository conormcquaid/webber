#ifndef __MAIN_H__
#define __MAIN_H__

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_http_server.h"

#define CONFIG_WEBSOCKET_URI "ws://tv-remote.local"
#define NO_DATA_TIMEOUT_SECS 10

#define MDNS_HOSTNAME "tv-remote"

typedef union{
    uint32_t color;
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
} RGBColor;

extern QueueHandle_t qSuperMessage;
extern QueueHandle_t qRotor; // notify of rotary encoder events
extern int get_hue(void);
extern RGBColor hslToRgb(float h, float s, float l);

void wifi_manager_task( void* nothing);

void init_webserver(void);





void supervisor(void* params);


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



#endif /* __MAIN_H__ */