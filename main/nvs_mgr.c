#include "nvs_mgr.h"
#include "esp_log.h"



#define MAX_CREDENTIALS 10

static char* TAG = "Creds";
static char* TIG = "|---->>";

static nvs_handle_t nvhandle = -1U;

static int verbose = 1;


void test_credentials(){

    init_credentials_mgr();

    delete_credentials();
    int n;
    char* ssids[MAX_CREDENTIALS];//

    get_ssids(ssids, &n);

    assert( n == 0 );

    set_credentials("elephant", "2021", "conor-gram", 8000);

    get_ssids(ssids, &n);

    assert( n == 1 );

    set_credentials("two_ssid", "second_pwd12", "host12", 12);
    set_credentials("thirdssid13", "third_pwd13", "host13", 13);
    set_credentials("fourthssid14", "fourth_pwd14", "host14", 14);

    get_ssids(ssids, &n);

   // assert( n == 4 );

    get_credentials_count(&n);

    assert( n == 4);

    char* ssid;
    char* pwd;
    char* host;
    int port;

    for(int i = 0; i < n; i++){

        get_credentials(&ssid, &pwd, &host, &port);
        ESP_LOGI(TAG, "%32s, %32s, %32s, %03d", ssid, pwd, host, port);
    }

}


esp_err_t init_credentials_mgr(void){

    esp_err_t e = ESP_OK;;
    if(-1U == nvhandle){
        e = nvs_open("secret", NVS_READWRITE, & nvhandle);
        if(e != ESP_OK){ return ESP_ERR_NVS_BASE; }
    }
    return e;
}

void delete_credentials(void){

    nvs_erase_all(nvhandle);
}

esp_err_t get_credentials_count(int* num){

    int n = 0;
    char key[129];
    size_t sz = 0;
    while( n < MAX_CREDENTIALS){

        snprintf(key, 128, "%s%d", SSID_KEY, n);

        if(ESP_ERR_NVS_NOT_FOUND == nvs_get_str(nvhandle, key, NULL, &sz)){
            break;
        }
        n++;
    }
    *num = n;
    if(verbose){ESP_LOGI(TIG, "I count %d credentials", n);}
    return ESP_OK;
}

/*
    get the value corresponding to the indexed key
    e.g. key=SSID_KEY, idx = 3 will look up "new_ssid3"
    return value in val
*/
esp_err_t get_nvs_string(char* key, char** val, int idx){

    char keyidx[129];
    size_t sz = 0;

    snprintf(keyidx, 128, "%s%d", key, idx);
    esp_err_t e = nvs_get_str(nvhandle, keyidx, NULL, &sz);
    if(e == ESP_OK){
        
        //TODO: use a local to grab val
        char* tmp = malloc(sz);
        e = nvs_get_str(nvhandle, keyidx, tmp, &sz);
        if(verbose){ESP_LOGI(TIG, "I retrieved %s%d: %s", key, idx, tmp);}
        *val = tmp;
    } 
    return e;
}
/*
    read wifi credentials from NVS, if they exist => ESP_OK
    if not in NVS, return ESP_ERR_NOT_FOUND

    will return all sets of credentials in order.
    Callers responsibility to acquire all extant sets

*/
static int cred_num = 0;

esp_err_t get_credentials(char** ssid, char** pwd, char** host, int* port){

    esp_err_t e;
    if(-1U == nvhandle){// <-- not initalized
        return ESP_ERR_NVS_BASE; 
    }
    int ncreds;
    get_credentials_count(&ncreds);

    if(cred_num >= MAX_CREDENTIALS || cred_num >= ncreds){ cred_num = 0;}

    if(verbose){ESP_LOGI(TIG, "I'm getting credentials at index %d", cred_num);}
    
    e = get_nvs_string(SSID_KEY, ssid, cred_num);
    if(ESP_OK != e){ return e; }

    e = get_nvs_string(PWD_KEY, pwd, cred_num);
    if(ESP_OK != e){ return e; }

    e = get_nvs_string(HOST_KEY, host, cred_num);
    if(ESP_OK != e){ return e; }

    uint32_t p;
    e = nvs_get_u32(nvhandle, PORT_KEY, &p);
    if(e == ESP_OK){ *port = p;}

    cred_num++; 

    return e;
}


esp_err_t set_credentials(char* ssid, char* pwd, char*host, int port){

    int n = 0;
    char key[129];
    size_t sz;
    esp_err_t e;
    while( n < MAX_CREDENTIALS){

        snprintf(key, 128, "%s%d", SSID_KEY, n);

        e =  nvs_get_str(nvhandle, key, NULL, &sz);

        // if(ESP_ERR_NVS_NOT_FOUND == e){
        //     break;
        // }
        if(ESP_OK == e){ 
            n++; 
            if(verbose){ESP_LOGI(TIG, "In setting credentials, skipping %s", key);}
        } else{ 
            if(verbose){ESP_LOGI(TIG, "In setting credentials, %s not found", key);}
            break; 
        }

        //n++;
    }
    if(ESP_ERR_NVS_NOT_FOUND != e){
        ESP_LOGI(TAG, "unexpected event %d", e);
    }

    if(n >= MAX_CREDENTIALS) return ESP_ERR_NVS_NOT_ENOUGH_SPACE;

    if(verbose){ESP_LOGI(TIG, "I'm writing credentials at index %d", n);}

    snprintf(key, 128, "%s%d", SSID_KEY, n);
    ESP_ERROR_CHECK(nvs_set_str(nvhandle, key, ssid));

    snprintf(key, 128, "%s%d", PWD_KEY, n);
    ESP_ERROR_CHECK(nvs_set_str(nvhandle, key, pwd));

    snprintf(key, 128, "%s%d", HOST_KEY, n);
    ESP_ERROR_CHECK(nvs_set_str(nvhandle, key, host));

    snprintf(key, 128, "%s%d", PORT_KEY, n);
    ESP_ERROR_CHECK(nvs_set_i32(nvhandle, key, port));
    
    return ESP_OK;
}
/*
    return all ssids stored in NVS
    in/out: reference to array of char*
    out: number of ssids

*/
esp_err_t get_ssids(char* ssids[], int *count){

    esp_err_t e = ESP_OK;
    *count = 0;
    if(-1U == nvhandle){// <-- not initalized
        return ESP_ERR_NVS_BASE; 
    }
    int ncreds;
    get_credentials_count(&ncreds);

    for(int i = 0; i < ncreds; i++){

        e = get_nvs_string(SSID_KEY,  &ssids[i], i);
        if(ESP_OK != e){ return e; }
        *count= *count + 1;
        if(verbose){ESP_LOGI(TIG, "I see ssid %s%d", SSID_KEY, i);}
    }
    return e;
}

