#include "tv.h"
#include "sdcard.h"
#include "led_strip.h"
#include "main.h"


static const char* TAG = "teevee";

static uint8_t* currFB;
static uint8_t* prevFB;
static uint8_t* renderFB;
static float*   delta;
static uint32_t frame_count; // TODO: get this from the getting place

#define BLINK_GPIO 21

extern void gamma_bright_correct(uint8_t* r, uint8_t* g, uint8_t* b, float bright);


tv_config_block_t tv_config_block;

int g_frame_size;
static int nLEDs;
static led_strip_handle_t led_strip;

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO, // The GPIO that connected to the LED strip's data line
        .max_leds = 64,      // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,        // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = (10 * 1000 * 1000), // RMT counter clock frequency
        .mem_block_symbols = 64, // the memory block size used by the RMT channel
        .flags = {
            .with_dma = 1,     // Using DMA can improve performance when driving more LEDs
        }
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");

    return led_strip;
}


TaskHandle_t tv_init(tv_status_t* tv_status){

    
    tv_config_block.rotor_dir = ROTOR_DIR_CCW;
    tv_config_block.rotor_clicks = ROTOR_CLICKS_TWO;
    tv_config_block.resolution = TV_RESOLUTION_HIGH;
  //tv_config_block.interpolation = TV_INTERPOLATE_NONE;
    tv_config_block.interpolation = TV_INTERPOLATE_LINEAR_RGB;
    tv_config_block.mode = TV_MODE_SEQUENTIAL;
    tv_config_block.speed.tweens = 5;
    tv_config_block.speed.interframe_millis = 41;
    tv_config_block.bytes_per_LED = 3 ;


    g_frame_size = (tv_config_block.resolution == TV_RESOLUTION_HIGH ? FRAME_RESOLUTION_HIGH : FRAME_RESOLUTION_LOW) * tv_config_block.bytes_per_LED;
    nLEDs = (tv_config_block.resolution == TV_RESOLUTION_HIGH ? FRAME_RESOLUTION_HIGH : FRAME_RESOLUTION_LOW);

    esp_err_t e = init_sd();

    if(e != ESP_OK){
        ESP_LOGE(TAG, "SD Card init failed, setting TV mode to OFF");
        tv_config_block.mode = TV_MODE_LAMP;
    }

    led_strip = configure_led();

    for(int q = 0; q < nLEDs; q++){
            
        RGBColor c = hslToRgb(360.0 * q / nLEDs, 90, 10);

    // RGBColor c = hslToRgb(36, 90, 10);

        led_strip_set_pixel(led_strip, q, c.r, c.g, c.b);

    }

    led_strip_refresh(led_strip);

    TaskHandle_t hTV;

    xTaskCreatePinnedToCore(tv_task, "tv_task", 4096, tv_status, configMAX_PRIORITIES-3, &hTV, 1);

    return hTV;
}



void tv_open_next_file(tv_status_t* pTV){

    // TODO: depends on tv mode being seq, rand, loop
    open_next_file(pTV);

    frame_count = 0;

}
///////////////////////////////////////////////////////////////////////////////////////////

/*
  Called for each tween
*/
void interpolate_linear_rgb(int tween){
       
    assert(tv_config_block.bytes_per_LED = 3);
    // TODO: really should be dealing with LED structs which have 4 bytes, may use only 3. FBs should be arrays of these

    // memoize tweens and validate
    int tweens = tv_config_block.speed.tweens;
    tweens = (tweens < 1) ? 1 : tweens;

    if(tween == 0){
        // 'from' buffer init
        memcpy(prevFB, currFB, g_frame_size);

        //calculate linear deltas
        for(int i = 0; i < g_frame_size; i++){

            delta[i] =  (currFB[i] - prevFB[i]) / (1.0 *  tweens); 
        }
    }
    ESP_LOGI(TAG, "PrevR1: %02x, CurrR1: %02x Delta:%3.3f",prevFB[0], currFB[0], delta[0]);

    for(int i = 0; i < nLEDs; i++){

        uint8_t r,g,b;
        r = prevFB[3*i]   + (uint8_t)(tween * delta[3*i]); 
        g = prevFB[3*i+1] + (uint8_t)(tween * delta[3*i+1]);
        b = prevFB[3*i+2] + (uint8_t)(tween * delta[3*i+2]);

        // TODO: fixmeup gamma_bright_correct(&r, &g, &b, pTV->brightness);

        led_strip_set_pixel(led_strip, i , r, g, b);
    }
    
    led_strip_refresh(led_strip);

    memcpy(prevFB, currFB, g_frame_size);

}

void tv_off(void){
    ESP_LOGI(TAG, "TV is off");
    // turn leds off, I suppose
    for(int q = 0; q < nLEDs; q++){
        led_strip_set_pixel(led_strip, q, 0, 0, 0);
    }
    led_strip_refresh(led_strip);
    
}
void act_like_a_lamp(void){

    static float n = 0.0;
            
    for(int q = 0; q < nLEDs; q++){
    
        RGBColor c = hslToRgb(n + 360.0 * (q) / nLEDs, 90, 10);

        led_strip_set_pixel(led_strip, q, c.r, c.g, c.b);

        n=n+0.01;

    }
    // for(int q = 0; q < nLEDs; q++){
    //     led_strip_set_pixel(led_strip, q, lamp_rgbc.r, lamp_rgbc.g, lamp_rgbc.b);
    // }
    led_strip_refresh(led_strip);

    vTaskDelay(pdMS_TO_TICKS(tv_config_block.speed.interframe_millis));
}


void tv_task(void* param){

    tv_status_t* pTV = (tv_status_t*)param;

    currFB   = malloc(g_frame_size);
    prevFB   = malloc(g_frame_size);
    renderFB = malloc(g_frame_size);
    delta    = malloc(g_frame_size);


    tv_config_block.interpolation = TV_INTERPOLATE_NONE;

    if(!currFB || !prevFB){
        tv_config_block.mode = TV_MODE_LAMP;
    }

    // TODO: iff we have a valid sdcard attached
    tv_open_next_file(pTV);

    while(1){

        // tv message queue for prev/next file?

        // moving from one file to another should have a darkness gap. supervisor turns off, then on?

        static int one_second_accum = 0;

        switch(tv_config_block.mode){

            case TV_MODE_OFF:

                tv_off();
                break;

            case TV_MODE_LAMP:
                act_like_a_lamp();
                break;

            case TV_MODE_LIFE:
                // not implemented yet

            break;

            case TV_MODE_LOOP:
            case TV_MODE_SEQUENTIAL:
            case TV_MODE_RANDOM:

            // normal playing modes
            // read a frame
            int r = getFrame(currFB);
            if(r != g_frame_size){

                // config settings for new board bringup (possibly even new boards?)
                // rgb test files for interpolation implies ability to choose playback file!

                ESP_LOGE(TAG, "Frame read size %d does not match expected %d", r, g_frame_size);
                // try to recover by opening next file
                if(r == -1){ // eof
                    tv_open_next_file(pTV);
                    

                }
                if( r == -2){
                    //card read error

                    //reboot will fall back to non-tv status if sd card is not initialized ok

                    //TODO: check for sd det
                    // TDOD: set appropriate tv mode if no sd card
                    esp_restart();
                }
            }

            pTV->file_frame_count = pTV->file_size_bytes/nLEDs;
            frame_count++;
            pTV->frame_number = frame_count;
            // customize behavior based on interpolation mode

            if(tv_config_block.interpolation == TV_INTERPOLATE_NONE){
                uint8_t* pPixel = currFB;
                for(int p = 0; p < nLEDs; p++){

                    uint8_t r,g,b;
                    r = *pPixel;
                    g = *(pPixel + 1);
                    b = *(pPixel + 2);
                    gamma_bright_correct(&r, &g, &b, pTV->brightness);
                    
                    led_strip_set_pixel(led_strip, p, r, g, b);
                    pPixel += tv_config_block.bytes_per_LED;
                    
                    led_strip_refresh(led_strip);

                }
            }else if(tv_config_block.interpolation == TV_INTERPOLATE_LINEAR_RGB ){

                for(int i = 0; i < tv_config_block.speed.tweens; i++){
                    interpolate_linear_rgb(i);
                    //vTaskDelay(pdMS_TO_TICKS(tv_config_block.speed.interframe_millis));
                }
            }

            break;
        };
            
        
        // once a second or so, we should push progress notification to web and gui info block
        vTaskDelay(pdMS_TO_TICKS(tv_config_block.speed.interframe_millis));

        one_second_accum += tv_config_block.speed.interframe_millis;
        if(one_second_accum > 10*1000 ){
            ESP_LOGI(TAG, "Speed.tweens %d, millis %d", tv_config_block.speed.tweens, tv_config_block.speed.interframe_millis);
            ESP_LOGI(TAG,"Elapsed millis: %lld", esp_timer_get_time()/1000);
            one_second_accum = 0;
        }
    }

}

void tv_play_file(const char* path);
void tv_set_mode(tv_mode_t mode);
void tv_set_state(tv_state_t state);
tv_mode_t tv_get_mode(void);

void tv_set_speed(tv_status_t* tv_status, tv_speed_t speed){
    tv_config_block.speed.tweens = speed.tweens;
    tv_config_block.speed.interframe_millis = speed.interframe_millis;
}
tv_speed_t tv_get_speed(tv_status_t* tv_status){
    return tv_config_block.speed;
}
tv_state_t tv_get_state(void);