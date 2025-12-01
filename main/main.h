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

extern RGBColor lamp_rgbc;

#define FRAME_RESOLUTION_HIGH (6*9)
#define FRAME_RESOLUTION_LOW (3*5)

extern int g_frame_size;

    

typedef enum{ 
    ROTOR_DIR_NONE,
    ROTOR_DIR_CW,
    ROTOR_DIR_CCW
}rotor_dir_t;

typedef enum{
    ROTOR_CLICKS_NONE,
    ROTOR_CLICKS_TWO,
    ROTOR_CLICKS_FOUR
}rotor_clicks_t;

typedef enum {
    TV_RESOLUTION_LOW, // 3x5
    TV_RESOLUTION_HIGH // 6x9
}tv_resolution_t;

typedef enum{
    TV_INTERPOLATE_NONE,
    TV_INTERPOLATE_LINEAR_RGB,
    TV_INTERPOLATE_LINEAR_HSV,
    TV_INTERPOLATE_CIE_RGB,
    TV_INTERPOLATE_CIE_LAB
}tv_interpolation_t;

typedef enum {
    TV_MODE_OFF,         /* sure, why not? */
    TV_MODE_SEQUENTIAL,  /* 'TV' modes */
    TV_MODE_LOOP,
    TV_MODE_RANDOM,
    TV_MODE_LAMP,       /* have a solid hue?*/
    TV_MODE_LIFE,       /* Conway FTW */

}tv_mode_t;

typedef struct{
    uint8_t tweens; // 1-10
    uint8_t interframe_millis; 
}tv_speed_t;

typedef struct{

    char      ip_addr[16];
    char      ssid[32];

}wifi_status_t;
extern wifi_status_t wifi_status;

// persistable preferences

typedef struct{

    tv_mode_t          mode;
    float              brightness;
    tv_speed_t         speed;
    tv_interpolation_t interpolation;
    
}tv_preferences_t;

// runtime data

typedef struct{

    char      current_file[260];
    uint32_t  file_size_bytes;
    uint32_t  file_frame_count;
    uint32_t  frame_number;


}tv_runtime_status_t;





// HW specific configuration
// Only changes if display &| rotary encoder change

typedef struct{

    rotor_dir_t        rotor_dir;
    rotor_clicks_t     rotor_clicks;
    tv_resolution_t    resolution;
    uint8_t            bytes_per_LED;

}tv_hardware_config_t;

// globally accessible config block
// persisted in NVS
extern tv_hardware_config_t tv_hw_config;


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