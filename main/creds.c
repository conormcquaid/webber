#include <string.h>
#include "esp_log.h"
#include "creds.h"
#include "main.h"
#include "tv.h"

#define NVS_NAMESPACE "secret"
#define WIFI_CRED_KEY "wifiCreds"
#define MAX_CREDENTIALS 10

static char* TAG = "creds.c";
static char* TIG = "|---->>";

static nvs_handle_t nvhandle = -1U;

static int verbose = 1;

#define TEST_SSID "elephant"
#define TEST_SSID2 "squirrel"
// #define TEST_HOST "conor-gram.local"
// #define TEST_PORT 8000
#define TEST_PWD "20217th&ve"

static WifiCred* pCredentials = NULL;

static uint8_t nCredentials = 0;

esp_err_t load_hw_config(tv_hardware_config_t* pConfig){

    if(-1U == nvhandle){
        ESP_ERROR_CHECK(init_wifi_creds_nvs());
    }
    
    size_t sz = sizeof(tv_hardware_config_t);
    // read size?
    esp_err_t e = nvs_get_blob(nvhandle, "hwConfig", NULL, &sz);
    if(e == ESP_OK){
        
        // load complete blob into tmp
        if(sz != sizeof(tv_hardware_config_t)){
            ESP_LOGE(TAG, "HW config format error");
            return ESP_ERR_INVALID_SIZE;
        }

        //get blob
        e = nvs_get_blob(nvhandle, "hwConfig", pConfig, &sz);
        ESP_LOGV(TAG, "Loading hw config, size %d", sz);
        
    
    }else if(e == ESP_ERR_NVS_NOT_FOUND){
        //blob did not exist
        // create a null record

        memset(pConfig, 0, sizeof(tv_hardware_config_t));
        ESP_LOGI(TAG, "Creating default empty hw config");
        pConfig->rotor_dir = ROTOR_DIR_CCW;
        pConfig->rotor_clicks = ROTOR_CLICKS_TWO;
        pConfig->resolution = TV_RESOLUTION_HIGH;
        pConfig->bytes_per_LED = 3;
        
    } 

    return ESP_OK;
}
esp_err_t save_hw_config(tv_hardware_config_t* pConfig){

    if(-1U == nvhandle){
        ESP_ERROR_CHECK(init_wifi_creds_nvs());
    }
    
    size_t sz = sizeof(tv_hardware_config_t);
    // write blob
    esp_err_t e = nvs_set_blob(nvhandle, "hwConfig", pConfig, sz);
    if(e != ESP_OK){ return e; }

    e = nvs_commit(nvhandle);
    return e;
}   
esp_err_t load_tv_preferences(tv_preferences_t* pPrefs){

    if(-1U == nvhandle){
        ESP_ERROR_CHECK(init_wifi_creds_nvs());
    }
    
    size_t sz = sizeof(tv_preferences_t);
    // read size?
    esp_err_t e = nvs_get_blob(nvhandle, "tvPrefs", NULL, &sz);
    if(e == ESP_OK){
        
        // load complete blob into tmp
        if(sz != sizeof(tv_preferences_t)){
            ESP_LOGE(TAG, "TV prefs format error");
            return ESP_ERR_INVALID_SIZE;
        }

        //get blob
        e = nvs_get_blob(nvhandle, "tvPrefs", pPrefs, &sz);
        ESP_LOGV(TAG, "Loading tv prefs, size %d", sz);
        
    
    }else if(e == ESP_ERR_NVS_NOT_FOUND){
        //blob did not exist
        // create a null record

        memset(pPrefs, 0, sizeof(tv_preferences_t));
        ESP_LOGI(TAG, "Creating default empty tv prefs");
        pPrefs->brightness = 0.5;
        pPrefs->mode = TV_MODE_SEQUENTIAL;
        pPrefs->interpolation = TV_INTERPOLATE_LINEAR_RGB;
        pPrefs->speed.interframe_millis = 41;
        pPrefs->speed.tweens = 10;
        
    } 

    return ESP_OK;
}   

esp_err_t save_tv_preferences(tv_preferences_t* pPrefs){

    if(-1U == nvhandle){
        ESP_ERROR_CHECK(init_wifi_creds_nvs());
    }
    
    size_t sz = sizeof(tv_preferences_t);
    // write blob
    esp_err_t e = nvs_set_blob(nvhandle, "tvPrefs", pPrefs, sz);
    if(e != ESP_OK){ return e; }

    e = nvs_commit(nvhandle);
    return e;
}   

esp_err_t get_credentials(WifiCred** pCreds, uint8_t* n){
    
    *pCreds = pCredentials;
    *n = nCredentials;
    return ESP_OK;
}

void test_credentials(void){

    ESP_LOGI(TAG, "Clearing wifi creds");

    ESP_ERROR_CHECK(remove_wifi_creds());

    ESP_ERROR_CHECK(load_wifi_creds());

    WifiCred oneCred = {
        .ssid = TEST_SSID,
        .pwd  = TEST_PWD,
    };
    ESP_LOGI(TIG, "Adding 1 wifi cred");
    ESP_ERROR_CHECK(add_credential(&oneCred));
    assert(nCredentials = 1);

    memcpy(oneCred.ssid, TEST_SSID2, strlen(TEST_SSID2));
    ESP_LOGI(TIG, "Adding 2nd wifi cred");
    ESP_ERROR_CHECK(add_credential(&oneCred));
    assert(nCredentials = 2);

    ESP_LOGI(TIG, "Deleting 1 wifi cred");
    ESP_ERROR_CHECK(delete_credential(0));
    assert(0 == memcmp(pCredentials[0].ssid, TEST_SSID2, strlen(TEST_SSID2)));

    ESP_LOGI(TIG, "Removing wifi creds");

    ESP_ERROR_CHECK(deinit_wifi_creds());

}

esp_err_t serialize_creds(void){

    size_t sz = nCredentials * sizeof(WifiCred);

    if(verbose) ESP_LOGI(TAG, "Wrinting %d bytes to nvs", sz);
    nvs_set_blob(nvhandle, WIFI_CRED_KEY, pCredentials, sz);
    nvs_commit(nvhandle);
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

esp_err_t load_wifi_creds(void){

    if(-1U == nvhandle){
        ESP_ERROR_CHECK(init_wifi_creds_nvs());
    }
    
    size_t sz = 0;
    // read size?
    esp_err_t e = nvs_get_blob(nvhandle, WIFI_CRED_KEY, NULL, &sz);
    if(e == ESP_OK){
        
        // load complete blob into tmp
        pCredentials = malloc(sz);

        ESP_ERROR_CHECK(pCredentials ? ESP_OK : ESP_ERR_NO_MEM);

        //get blob
        e = nvs_get_blob(nvhandle, WIFI_CRED_KEY, pCredentials, &sz);
        ESP_LOGV(TAG, "Loading wifi creds, size %d", sz);
        
    
        nCredentials  = sz / sizeof(WifiCred);

        if(sz % sizeof(WifiCred) != 0){
            ESP_LOGE(TAG, "Credentials format error");
        }
        assert(nCredentials < MAX_CREDENTIALS);


    }else if(e == ESP_ERR_NVS_NOT_FOUND){
        //blob did not exist
        // create a null record

        pCredentials = NULL;
        nCredentials = 0;
        ESP_LOGI(TAG, "Creating default empty wifi cred array");
        
    } 

    return ESP_OK;
}

esp_err_t delete_credential(uint8_t n){
    
    if( n < nCredentials){ return ESP_ERR_INVALID_ARG; }

    // overwrite deleted key with last key
    memcpy((void*)(&pCredentials[n]), (void*)(&pCredentials[nCredentials - 1]), sizeof(WifiCred) );
    pCredentials = realloc((void*)pCredentials, (nCredentials-1) * sizeof(WifiCred));

    if(!pCredentials){ return ESP_ERR_NO_MEM;  };
    nCredentials--;

    ESP_ERROR_CHECK(serialize_creds());

    return ESP_OK;
}


esp_err_t add_credential(WifiCred* cred){

    if(pCredentials == NULL){
        pCredentials = malloc(sizeof(WifiCred));
    }else{
        pCredentials = realloc((void*)pCredentials, (nCredentials+1)* sizeof(WifiCred));
    }
    if(!pCredentials){ return ESP_ERR_NO_MEM; }

    memcpy(&(pCredentials[nCredentials]), cred, sizeof(WifiCred));
    nCredentials++;

    ESP_ERROR_CHECK(serialize_creds());

    return ESP_OK;
}

esp_err_t remove_wifi_creds(){

    if(pCredentials == NULL){ return ESP_OK; }

    free(pCredentials);
    nCredentials = 0;

    nvs_erase_key(nvhandle, WIFI_CRED_KEY);

    return ESP_OK;
}


