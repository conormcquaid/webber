#include <stdio.h>

#include "main.h"

void app_main(void)
{
    TaskHandle_t hWifi;
    
    xTaskCreate(wifi_manager_task, "wifi_man", 10*1024, NULL, tskIDLE_PRIORITY, &hWifi);
}