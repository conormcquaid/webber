/*
This module manages the wifi connection.
Both station and access point modes will be active

If valid credentials are found in the NVS, then the station mode will be active.

At any time the AP will be active, allowing ssid credentials to be saved

*/

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_rom_efuse.h"
#include "esp_mac.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"
#include "esp_http_server.h"
#include "dns_server.h"
#include "nvs_flash.h"
#include "driver/touch_sensor.h"
#include "main.h"
#include "driver/gpio.h"
#include "mdns.h"
#include "main.h"
#include "mdns.h"
#include "creds.h"
#include "websock.h"


#define MAX_NUM_APS 32

#define NO_CONNECTION  0
#define CAPTIVE_PORTAL 1
#define STATION        2
int connectionStatus = 0;

static char* TAG = "Wifi_Man";

esp_err_t init_wifi_creds_nvs(void);
esp_err_t deinit_wifi_creds(void);
esp_err_t load_wifi_creds(WiFiCredBlob** pCreds);

WiFiCredBlob* pCredentials;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* AP Configuration */
//#define WIFI_AP_SSID_PREFIX     "emp-"
#define WIFI_AP_SSID_PREFIX     "Empire-"
#define WIFI_AP_PASSWD          "goldenmean"
#define WIFI_CHANNEL            6
#define MAX_STA_CONN            4
#define STA_MAXIMUM_RETRY       10

/*DHCP server option*/
#define DHCPS_OFFER_DNS             0x02

static const char *TAG_AP = "WiFi SoftAP";
static const char *TAG_STA = "WiFi Sta";

static int s_retry_num = 0;

static EventGroupHandle_t s_wifi_event_group;

void start_mdns_service()
{
    //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err) {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    //set hostname
    mdns_hostname_set(MDNS_HOSTNAME);
    //set default instance
    mdns_instance_name_set(MDNS_HOSTNAME);
}



static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {

        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG_AP, "Station "MACSTR" joined, AID=%d", MAC2STR(event->mac), event->aid);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {

        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG_AP, "Station "MACSTR" left, AID=%d, reason:%d",  MAC2STR(event->mac), event->aid, event->reason);

        s_retry_num++;
        if(s_retry_num > STA_MAXIMUM_RETRY){
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }else{
            ESP_LOGI(TAG, "Retrying to connect to Wi-Fi network ...");
            esp_wifi_connect();
        }


    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGE(TAG_STA, "Station disconnected, reason: %d", event->reason);
        if (s_retry_num < STA_MAXIMUM_RETRY) {
            ESP_LOGI(TAG_STA, "Retrying connection (%d/%d)", s_retry_num + 1, STA_MAXIMUM_RETRY);
            s_retry_num++;
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG_STA, "Max retries reached");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
   



    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {

        esp_wifi_connect();
        ESP_LOGI(TAG_STA, "Station started");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG_STA, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void scan(char* APs[], int* nAPs){

    ESP_LOGI(TAG, "Begin wifi scan");
    wifi_scan_config_t scan_cfg = {
        .bssid = 0,
        .ssid = 0,
        .channel = 0,
        .show_hidden = false
    };

    // added 2 lines    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK( esp_wifi_scan_start(&scan_cfg, true)); // blocking scan = true

    uint16_t num_found;
    esp_wifi_scan_get_ap_num(&num_found);
    ESP_LOGI(TAG, "Found %d access points", num_found);

    wifi_ap_record_t records[MAX_NUM_APS]; // picking a nnumber
    uint16_t max_num = MAX_NUM_APS;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&max_num, records));
    if(max_num){ESP_LOGI(TAG, "            --ssid--            | rssi");}
    for(int i = 0; i < max_num; i++){
        ESP_LOGI(TAG, "%32s, %4d", records[i].ssid, records[i].rssi);

        APs[i] = strdup((char*)records[i].ssid);
    }
    *nAPs = max_num;

    ESP_ERROR_CHECK(esp_wifi_stop());
    //ESP_ERROR_CHECK(esp_wifi_deinit()); // & took this out
}

/* Initialize soft AP */
esp_netif_t *wifi_init_softap(void)
{
    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

    // grab unique key: available on S3 only
    uint32_t hmac_key[4];
    esp_err_t err = esp_efuse_read_field_blob(ESP_EFUSE_OPTIONAL_UNIQUE_ID, &hmac_key, 16 * 8);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_efuse_read_field_blob failed!");
    }

    wifi_config_t wifi_ap_config = {
        .ap = {
            .channel = WIFI_CHANNEL,
            .password = WIFI_AP_PASSWD,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    bzero(&wifi_ap_config.ap.ssid, 32);
    snprintf((char*)wifi_ap_config.ap.ssid, 32, "%s%08lx", WIFI_AP_SSID_PREFIX, hmac_key[0]);
    //snprintf((char*)wifi_ap_config.ap.ssid, 32, "%s1", WIFI_AP_SSID_PREFIX);
    wifi_ap_config.ap.ssid_len = strlen((char*)wifi_ap_config.ap.ssid);

    if (wifi_ap_config.ap.ssid_len == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI(TAG_AP, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             wifi_ap_config.ap.ssid, WIFI_AP_PASSWD, WIFI_CHANNEL);

    connectionStatus |= CAPTIVE_PORTAL;

    return esp_netif_ap;
}

/* Initialize wifi station */
esp_err_t wifi_init_sta(uint8_t ssid[], uint8_t pwd[])
{
    wifi_config_t wifi_sta_config = {
        .sta = {

            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = STA_MAXIMUM_RETRY,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
            * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    memcpy(wifi_sta_config.sta.ssid, ssid, 32);
    memcpy(wifi_sta_config.sta.password, pwd, 64);
    //wifi_sta_config.sta.ssid_len = strlen((char*)wifi_sta_config.sta.ssid);

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(err));
        return err;
    }

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(10000));


    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "----connected to ap SSID:%s password:%s",
                 ssid, pwd);

                 return ESP_OK;

    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "----Failed to connect to SSID:%s, password:%s",
                 ssid, pwd);

                 return ESP_ERR_ESP_NETIF_INIT_FAILED;


    } else {
        ESP_LOGE(TAG, "----UNEXPECTED EVENT");
        return ESP_ERR_ESP_NETIF_INIT_FAILED;
    }

}

void softap_set_dns_addr(esp_netif_t *esp_netif_ap,esp_netif_t *esp_netif_sta)
{
    esp_netif_dns_info_t dns;
    esp_netif_get_dns_info(esp_netif_sta,ESP_NETIF_DNS_MAIN,&dns);
    uint8_t dhcps_offer_option = DHCPS_OFFER_DNS;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(esp_netif_ap));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(esp_netif_ap, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_option, sizeof(dhcps_offer_option)));
    ESP_ERROR_CHECK(esp_netif_set_dns_info(esp_netif_ap, ESP_NETIF_DNS_MAIN, &dns));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(esp_netif_ap));
}

void wifi_sta_scan(){

    // scan_for_APs
    char* APs[MAX_NUM_APS];
    int nRec;
    scan(APs, &nRec);

    esp_err_t et = load_wifi_creds(&pCredentials);
    if(ESP_OK != et){
        ESP_LOGI(TAG, "Resetting wifi credentials and restarting. Error: %d\n", et);
        remove_wifi_creds();
        esp_restart();
    }


    esp_netif_create_default_wifi_sta();

    // match AP to credentials
    for(int i = 0; i< nRec; i++){

        ESP_LOGI(TAG, "Checking ssid %s ", APs[i]);
        for(int j = 0; j < pCredentials->nCreds; j++){
            if(0 == memcmp(pCredentials->wifiCreds[j].ssid, APs[i], strlen(APs[i]))){
                ESP_LOGI(TAG, "SSID match %s %s", pCredentials->wifiCreds[j].ssid, APs[i]);
                ESP_LOGI(TAG, "Trying ssid %s with pwd %s", (char*)pCredentials->wifiCreds[j].ssid, (char*)pCredentials->wifiCreds[j].pwd);
                if(ESP_OK == wifi_init_sta(pCredentials->wifiCreds[j].ssid, pCredentials->wifiCreds[j].pwd)){
                    ESP_LOGI(TAG, "Connected to %s ", (char*)pCredentials->wifiCreds[j].ssid);
                    connectionStatus |= STATION;
                    break;
                }else{
                    ESP_LOGI(TAG, "No luck with %s and %s", (char*)pCredentials->wifiCreds[j].ssid, (char*)pCredentials->wifiCreds[j].pwd);
                }
            }
        }
    }

}

char* APs[MAX_NUM_APS];
int nRec;

void wifi_sta_connect(){

    // scan_for_APs

    // BUG in esp-idf: scan cannot take place in APSTA mode
    // b/c this triggers WIFI_EVENT_STA_START, enev though only AP segment is configured
    //scan(APs, &nRec);

    esp_err_t et = load_wifi_creds(&pCredentials);
    if(ESP_OK != et){
        ESP_LOGI(TAG, "Resetting wifi credentials and restarting. Error: %d\n", et);
        remove_wifi_creds();
        esp_restart();
    }
    if(pCredentials->nCreds == 0){
        ESP_LOGI(TAG, "No credentials found\n");
        return;
    }
    // match AP to credentials
    for(int j = 0; j < pCredentials->nCreds; j++){
            
        ESP_LOGI(TAG, "Trying ssid %s with pwd %s", (char*)pCredentials->wifiCreds[j].ssid, (char*)pCredentials->wifiCreds[j].pwd);
        if(ESP_OK == wifi_init_sta(pCredentials->wifiCreds[j].ssid, pCredentials->wifiCreds[j].pwd)){

            ESP_LOGI(TAG, "Successful connection to %s", (char*)pCredentials->wifiCreds[j].ssid);
                    
            connectionStatus |= STATION;
            break;
        }else{
            ESP_LOGI(TAG, "No luck with %s and %s", (char*)pCredentials->wifiCreds[j].ssid, (char*)pCredentials->wifiCreds[j].pwd);
        }
    }
}

void wifi_manager_task( void* nothing){

    // setup
    ESP_ERROR_CHECK(nvs_flash_init());   

    // Initialize networking stack
    ESP_ERROR_CHECK(esp_netif_init());    
    // Create default event loop needed by the  main app
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));



    scan(APs,&nRec);

    s_wifi_event_group = xEventGroupCreate();

    /* Register Event handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,    &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,   IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));


    // dual mode
    esp_wifi_set_mode(WIFI_MODE_APSTA);

    // TODO: implications?
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    //esp_netif_t *esp_netif_ap = 
    
    wifi_init_softap();

    ESP_ERROR_CHECK(esp_wifi_start());

    start_webserver();
    start_mdns_service();

    //TaskHandle_t hWebsocket;
    
    //xTaskCreate(websocket_task, "websock", 10*1024, NULL, tskIDLE_PRIORITY, &hWebsocket);


   //wifi_sta_scan(); //
    wifi_sta_connect();


    while(true){

        vTaskDelay(20 / portTICK_PERIOD_MS);

        //if we lose station connection ...
        // wifi_sta_scan() again

    }
}