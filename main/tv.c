#include "tv.h"
#include "sdcard.h"
#include "led_strip.h"


static const char* TAG = "teevee";

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


void tv_init(void){

    init_sd();

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
}