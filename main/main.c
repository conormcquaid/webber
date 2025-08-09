#include <stdio.h>

#include "main.h"
#include "rotary_encoder.h"
#include "led_strip.h"


// temporary
#include "sdcard.h"
#include "led_strip.h"
#include "oled_i2c.h"


const char* TAG = "main";

extern void wifi_init_apsta(void);
extern void init_rotary_encoder(void* p);

// I belong elsewhere
QueueHandle_t qHue;

#define BLINK_GPIO 21

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

void app_main(void)
{


/// Create the LED strip object
led_strip_handle_t led_strip = configure_led();

led_strip_set_pixel(led_strip, 0, 50, 0, 0);
led_strip_set_pixel(led_strip, 1, 0, 50, 0);
led_strip_set_pixel(led_strip, 2, 0, 0, 50);
led_strip_set_pixel(led_strip, 3, 0, 0, 0);
led_strip_set_pixel(led_strip, 4, 50, 0, 0);
led_strip_set_pixel(led_strip, 5, 0, 50, 0);
led_strip_set_pixel(led_strip, 6, 0, 0, 50);
led_strip_set_pixel(led_strip, 7, 0, 0, 0);
led_strip_set_pixel(led_strip, 8, 50, 0, 0);
led_strip_set_pixel(led_strip, 9, 0, 50, 0);

    int nLeds = 54;
    for(int q = 0; q < nLeds; q++){
        
        RGBColor c = hslToRgb(360.0 * q / nLeds, 90, 10);

       // RGBColor c = hslToRgb(36, 90, 10);

        led_strip_set_pixel(led_strip, q, c.r, c.g, c.b);

    }

led_strip_refresh(led_strip);


  //  OLED_init();
  //  OLED_Clear(0,7);
  //  OLED_WriteBig( "tits!", 3, 3);

    

    // for(int q = 0; q < 11; q++){
    //     led_strip_pixels[q] = 0; // clear the led strip pixels
    //     RGBColor c = hslToRgb(360 * q / 12.0, 90, 30);
    //     led_strip_pixels[q*4 + 0] = c.r;
    //     led_strip_pixels[q*4 + 1] = c.g;
    //     led_strip_pixels[q*4 + 2] = c.b;
    //     led_strip_pixels[q*4 + 3] = 20;

    // }

    // for(int q = 0; q < 11; q++){
    //     led_strip_pixels[q] = 0; // clear the led strip pixels
    //     //RGBColor c = hslToRgb(360 * q / 12.0, 90, 30);
    //     led_strip_pixels[48 + q*3 + 0] = 55;
    //     led_strip_pixels[48 + q*3 + 1] = 0;
    //     led_strip_pixels[48 + q*3 + 2] = 0;
        

    // }


    
    // write_leds();

    init_sd();

    wifi_init_apsta();

    init_rotary_encoder(NULL);
    
    TaskHandle_t hSupervisor;
    xTaskCreate(supervisor, "supervisor", 10*1024, NULL, tskIDLE_PRIORITY, &hSupervisor);

    // TaskHandle_t hTV;
    // xTaskCreate(tv_task, "tv_task", 10*1024, NULL, tskIDLE_PRIORITY, &hTV);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


