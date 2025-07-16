#include <string.h>
#include "esp_log.h"
#include "creds.h"

#define NVS_NAMESPACE "secret"
#define WIFI_CRED_KEY "wifiCreds"
#define MAX_CREDENTIALS 10

static char* TAG = "creds.c";
static char* TIG = "|---->>";

static nvs_handle_t nvhandle = -1U;

static int verbose = 1;

#define TEST_SSID "elephant"
#define TEST_SSID2 "squirrel"
#define TEST_HOST "conor-gram.local"
#define TEST_PORT 8000
#define TEST_PWD "20217th&ve"

static WiFiCredBlob sWifiCreds;

void test_credentials(void){

    WiFiCredBlob* pCreds;

    ESP_LOGI(TAG, "Clearing wifi creds");

    ESP_ERROR_CHECK(remove_wifi_creds());

    ESP_ERROR_CHECK(load_wifi_creds(&pCreds));

    assert(pCreds->nCreds == 0);

    WifiCred oneCred = {
        .ssid = TEST_SSID,
        .host = TEST_HOST,
        .pwd  = TEST_PWD,
        .port = TEST_PORT
    };
    ESP_LOGI(TIG, "Adding 1 wifi cred");
    ESP_ERROR_CHECK(add_credential(&oneCred));
    assert(pCreds->nCreds = 1);

    memcpy(oneCred.ssid, TEST_SSID2, strlen(TEST_SSID2));
    ESP_LOGI(TIG, "Adding 2nd wifi cred");
    ESP_ERROR_CHECK(add_credential(&oneCred));
    assert(pCreds->nCreds = 2);

    ESP_LOGI(TIG, "Deleting 1 wifi cred");
    ESP_ERROR_CHECK(delete_credential(0));
    assert(0 == memcmp(pCreds->wifiCreds[0].ssid, TEST_SSID2, strlen(TEST_SSID2)));

    ESP_LOGI(TIG, "Removing wifi creds");

    ESP_ERROR_CHECK(deinit_wifi_creds());

}

esp_err_t serialize_creds(void){

    // pack all of the malloc'ed creds into one entity 'tmp'
    size_t int_sz = sizeof(sWifiCreds.nCreds);
    size_t sz = int_sz + sWifiCreds.nCreds *  (sizeof(WifiCred));
    void* tmp = malloc(sz);
    if(!tmp){ return ESP_ERR_NO_MEM; }

    void*p = tmp;
    // store nCreds
    memcpy(p, &sWifiCreds.nCreds, sizeof(sWifiCreds.nCreds));
    p += int_sz;
    // append the WififCreds
    memcpy(p, &sWifiCreds.wifiCreds[0], sWifiCreds.nCreds *  sizeof(WifiCred));
    if(verbose) ESP_LOGI(TAG, "Wrinting %d bytes to nvs", sz);
    nvs_set_blob(nvhandle, WIFI_CRED_KEY, tmp, sz);
    nvs_commit(nvhandle);
    free(tmp);
    return ESP_OK;
}


esp_err_t deinit_wifi_creds(void){
    
    // release handle
    nvs_close(nvhandle);

    return ESP_OK;
}

esp_err_t init_wifi_creds_nvs(void){

    esp_err_t e = ESP_OK;;
    if(-1U == nvhandle){
        e = nvs_open(NVS_NAMESPACE, NVS_READWRITE, & nvhandle);
        if(e != ESP_OK){ return ESP_ERR_NVS_BASE; }
    }
    //nvs_erase_key(nvhandle, WIFI_CRED_KEY);
    return e;
}

esp_err_t load_wifi_creds(WiFiCredBlob** pCreds){

    if(-1U == nvhandle){
        ESP_ERROR_CHECK(init_wifi_creds_nvs());
    }
    
    size_t sz = 0;
    // read size?
    esp_err_t e = nvs_get_blob(nvhandle, WIFI_CRED_KEY, NULL, &sz);
    if(e == ESP_OK){
        
        // load complete blob into tmp
        void* tmp = malloc(sz);

        //get blob
        e = nvs_get_blob(nvhandle, WIFI_CRED_KEY, tmp, &sz);
        if(verbose) ESP_LOGI(TAG, "Loading wifi creds, size %d", sz);
        // save nCreds
        sWifiCreds.nCreds = *(uint32_t*)tmp;
        //sanity check
        if(sWifiCreds.nCreds > 32){
            //setting a plausible cap
            return ESP_ERR_NO_MEM;
        }
        // allocate new mem for array
        void* pcreds  = malloc(sWifiCreds.nCreds * sizeof(WifiCred));
        // copy array
        memcpy(pcreds, tmp+sizeof(sWifiCreds.nCreds), sWifiCreds.nCreds*sizeof(WifiCred));  
        // take reference to array     
        sWifiCreds.wifiCreds = (WifiCred*)pcreds;

        if(verbose){ESP_LOGI(TIG, "I retrieved %ld wifi creds of size %u", sWifiCreds.nCreds, sz);}

        // done with blob
        free(tmp);

    }else if(e == ESP_ERR_NVS_NOT_FOUND){
        //blob did not exist
        sWifiCreds.nCreds = 0;
        sWifiCreds.wifiCreds = NULL;
        // *pCreds=malloc(sizeof(WiFiCredBlob));
        // if(!*pCreds) return ESP_ERR_NO_MEM;
        // bzero(*pCreds, sizeof(WiFiCredBlob));
        // WifiCred c = {
        //     .ssid = "dummyssid",
        //     .pwd = "dummypwd",
        //     .host = "dummyhost",
        //     .port = 999
        // };
        // *pCreds->wifiCreds = malloc(sizeof(WifiCred));
        // memcpy(*pCreds->wifiCreds, &c, sizeof(WifiCred));
        // return ESP_OK;
    } 
    *pCreds = &sWifiCreds;

    return ESP_OK;
}
esp_err_t delete_credential(uint8_t n){

    uint8_t nc = sWifiCreds.nCreds;
    assert( n <= nc);
    // overwrite deleted key with last key
    memcpy((void*)(&sWifiCreds.wifiCreds[n]), (void*)(&sWifiCreds.wifiCreds[nc - 1]), sizeof(WifiCred) );
    sWifiCreds.wifiCreds = realloc((void*)sWifiCreds.wifiCreds, (nc-1) * sizeof(WifiCred));

    assert(sWifiCreds.wifiCreds);
    sWifiCreds.nCreds--;

    ESP_ERROR_CHECK(serialize_creds());

    return ESP_OK;
}
esp_err_t add_credential(WifiCred* cred){

    uint8_t n = sWifiCreds.nCreds;
    if(sWifiCreds.wifiCreds == NULL){
        sWifiCreds.wifiCreds = malloc((n+1)* sizeof(WifiCred));
    }else{
        sWifiCreds.wifiCreds = realloc((void*)sWifiCreds.wifiCreds, (n+1)* sizeof(WifiCred));
    }
    assert(sWifiCreds.wifiCreds);
    memcpy(&(sWifiCreds.wifiCreds[n]), cred, sizeof(WifiCred));
    sWifiCreds.nCreds++;

    ESP_ERROR_CHECK(serialize_creds());

    return ESP_OK;
}

esp_err_t remove_wifi_creds(){

    if(sWifiCreds.wifiCreds == NULL){ return ESP_OK; }

    free(sWifiCreds.wifiCreds);
    sWifiCreds.nCreds = 0;

    nvs_erase_key(nvhandle, WIFI_CRED_KEY);

    return ESP_OK;
}


