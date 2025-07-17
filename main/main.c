#include <stdio.h>

#include "main.h"

extern void wifi_init_apsta(void);
void app_main(void)
{
    // TaskHandle_t hWifi;
    
    // xTaskCreate(wifi_manager_task, "wifi_man", 10*1024, NULL, tskIDLE_PRIORITY, &hWifi);

    wifi_init_apsta();
}