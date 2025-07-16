#ifndef __NVS_MGR_H__
#define __NVS_MGR_H__

#include "esp_err.h"
#include "nvs_flash.h"

#define SSID_KEY "new_ssid"
#define PWD_KEY  "new_pwd"
#define HOST_KEY "hostname"
#define PORT_KEY "port"

void test_credentials();

esp_err_t init_credentials_mgr(void);

void delete_credentials(void);

esp_err_t get_credentials(char** ssid, char** pwd, char** host, int* port);
esp_err_t set_credentials(char*  ssid, char*  pwd, char*  host, int  port);

esp_err_t get_credentials_count(int* num);

esp_err_t get_ssids(char* ssids[], int *count);

#endif /*  __NVS_MGR_H__ */