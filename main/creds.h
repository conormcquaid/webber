#ifndef __CREDS_H__
#define __CREDS_H__

#include <stdint.h>

#include "esp_err.h"
#include "nvs_flash.h"

#define SSID_KEY "new_ssid"
#define PWD_KEY  "new_pwd"
#define HOST_KEY "hostname"
#define PORT_KEY "port"

typedef struct{
    uint8_t ssid[32];
    uint8_t pwd[64];
    // uint8_t host[64]; <== useful for OSC
    // int port;
}WifiCred;

// typedef struct{
//     uint32_t nCreds; // make 4bytes to simplify layout during ser/des
//     WifiCred *wifiCreds; // pointer to array of WifiCred[nCreds]
// }WiFiCredBlob;

void test_credentials(void);
esp_err_t init_wifi_creds_nvs(void);
esp_err_t deinit_wifi_creds(void);
esp_err_t load_wifi_creds(void);
esp_err_t delete_credential(uint8_t n);
esp_err_t add_credential(WifiCred* cred);
esp_err_t remove_wifi_creds();

esp_err_t get_credentials(WifiCred** pCreds, uint8_t* n);




#endif /* __CREDS_H__ */