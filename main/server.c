#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"
#include "esp_http_server.h"
#include "dns_server.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_websocket_client.h"
#include <cJSON.h>
#include "mdns.h"
#include "creds.h"
#include "esp_spiffs.h"
#include "esp_websocket_client.h"
#include <cJSON.h>
#include "websock.h"
#include "main.h"
#include "server.h"

static char* TAG = "--server--";

static httpd_handle_t server_handle = NULL;

#define MAX_WS_CLIENTS 4
int ws_client[MAX_WS_CLIENTS];


void ws_transmit(httpd_req_t* req, char* pj);

esp_err_t ws_handler(httpd_req_t* req){

    if(req->method == HTTP_GET){
        ESP_LOGI(TAG, "new ws connection");
        for(int i = 0; i<MAX_WS_CLIENTS; i++){
            if(ws_client[i] == 0){
                ws_client[i] = httpd_req_to_sockfd(req);
                ESP_LOGI(TAG, "ws client %d connected on %d", i, ws_client[i]);
                //ws_transmit(req, "{\"init\":\"success\"}");
                return ESP_OK;
            }
        }
        //MAX CLIENTS reached
        ESP_LOGW(TAG, "Max WS clients reached");
        return ESP_FAIL;
    }
    // not a GET request
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(ws_pkt));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    // get length of the payload
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "Error receiving WS frame: %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "Got ws packet length %zu", ws_pkt.len);
    if(ws_pkt.len){
        // allocate a buffer for the payload
        uint8_t* buf = calloc(1, ws_pkt.len + 1);
        if(!buf){
            ESP_LOGE(TAG, "Failed to allocate buffer for WS payload");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        // read the payload
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if(ret != ESP_OK){
            ESP_LOGE(TAG, "Error receiving WS frame: %d", ret);
            free(buf);
            return ret;
        }
        buf[ws_pkt.len] = '\0'; // null-terminate the string
        ESP_LOGI(TAG, "Received WS message: %s", buf);

        //TODO: parsemessage here
        xQueueSendToBack(qSuperMessage, &buf, 50);
        //free(buf); // freed in message processor
    }

    if(ws_pkt.final && ws_pkt.len == 0){
        // client disconnected
        ESP_LOGI(TAG, "Client disconnected");
        int client_fd = httpd_req_to_sockfd(req);
        for(int i = 0; i<MAX_WS_CLIENTS; i++){
            if(ws_client[i] == client_fd){
                ws_client[i] = 0;
                ESP_LOGI(TAG, "Client %d disconnected on FD %d", i, client_fd);
                break;
            }
        }
    } 

    return ESP_OK;
}

void ws_transmit(httpd_req_t* req, char* a_json_msg) {
    // sending dynamic initializtion message to all connected clients
    //for(int i = 0; i < MAX_WS_CLIENTS; i++) {
    //    if(ws_client[i] != 0) {
            httpd_ws_frame_t ws_pkt;
            memset(&ws_pkt, 0, sizeof(ws_pkt));
            ws_pkt.type = HTTPD_WS_TYPE_TEXT;
            char m[64];
            ws_pkt.len = snprintf(m, sizeof(m), "{\"hue\":\"%d\"}", get_hue());
            ws_pkt.payload = (uint8_t*)m;
            ESP_LOGI(TAG, "Sending message to client %d: %s", 666, ws_pkt.payload);
            httpd_ws_send_frame(req, &ws_pkt);
    //    }
    //}
}

void ws_notify(char* a_json_msg) {
    // sending to all connected clients
    for(int i = 0; i < MAX_WS_CLIENTS; i++) {
        if(ws_client[i] != 0) {
            httpd_ws_frame_t ws_pkt;
            memset(&ws_pkt, 0, sizeof(ws_pkt));
            ws_pkt.type = HTTPD_WS_TYPE_TEXT;
            ws_pkt.payload = (uint8_t*)a_json_msg;
            ws_pkt.len = strlen(a_json_msg);
            ESP_LOGI(TAG, "Sending message to client socket %d: %s", ws_client[i], ws_pkt.payload);
            httpd_ws_send_data(server_handle , ws_client[i], &ws_pkt);
        }
    }
}


/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192
#define MIN(a,b) ((a) < (b) ? (a) : (b))

struct file_server_data {
    /* Base path of file storage */
    char base_path[100];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};

static struct file_server_data *server_data;


#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

extern int get_wifi_from_post(char* post, char** ssid, char** pwd);

esp_err_t post_process(char* post){

    char* pwd = NULL;
    char* ssid = NULL;


    get_wifi_from_post(post, &ssid, &pwd);

    ESP_LOGI(TAG, "post process got %s and %s\n", ssid, pwd);

    if(ssid == NULL || pwd == NULL){ return ESP_ERR_NOT_FOUND; }

    //save credentials to be used on next boot
    ESP_LOGI(TAG, "Saving credentials");

    WifiCred cred;
    bzero(&cred, sizeof(WifiCred));

    memcpy(cred.ssid, ssid, strlen(ssid));
    memcpy(cred.pwd,  pwd,  strlen(pwd));
    // memcpy(cred.host, host, strlen(host)+1);
    // cred.port = port;
    // WifiCred cred={
    //     .ssid = ssid,
    //     .pwd = pwd,
    //     .host = host,
    //     .port = port
    // };
    add_credential(&cred);
    return ESP_OK;
}

// captive POST
static esp_err_t captive_post_handler(httpd_req_t *req)
{
    /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char content[100];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    content[recv_size] = '\0';
    ESP_LOGI(TAG , "%s\n", content);

    if(ESP_OK == post_process(content)){

        // we got wifi credentials saved
        // let's reboot & go again
        esp_restart();
    }

    /* Send a simple response */
    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}



// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}



void init_spiff(void){
    
    // initialize SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path="/spiffs",
        .partition_label=NULL,
        .max_files=5,
        .format_if_mount_failed=false

    };
	esp_vfs_spiffs_register( &conf );
	printf("- SPIFFS VFS module registered\n");
}

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".js")) {
        return httpd_resp_set_type(req, "application/javascript");
    } else if (IS_FILE_EXT(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    } else if (IS_FILE_EXT(filename, ".png")) {
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".svg")) {
        return httpd_resp_set_type(req, "image/svg+xml");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

/* An HTTP GET handler */
static esp_err_t spiff_serve(httpd_req_t *req)
{
    const char *filename = req->uri;

    printf("+ root get with fname %s\n",(char*)filename);

    if(filename[0] == '/' && filename[1] == '\0'){
        strcat(filename, "index.html");
    }


	// check if it exists on SPIFFS
	char full_path[512+16];
	sprintf(full_path, "/spiffs%s", filename);
	printf("+ Serving static resource: %s\n", full_path);
	struct stat st;
	if (stat(full_path, &st) == 0) {

        //set header value for content-type
        set_content_type_from_file(req, filename);
		
		// open the file for reading
		FILE* f = fopen(full_path, "r");
		if(f == NULL) {
			printf("Unable to open the file %s\n", full_path);
			return ESP_FAIL;
		}
		
		// send the file content to the client
		char buffer[500];
		int size = 0;
		/*while(fgets(buffer, 500, f)) {
			netconn_write(conn, buffer, strlen(buffer), NETCONN_NOCOPY);
		}*/
		while(!feof(f)) {
			
			size_t char_read;
			char_read = fread(buffer, sizeof(char), 500, f);
			size += char_read / sizeof(char);
			httpd_resp_send_chunk(req, buffer, char_read / sizeof(char));	
		}
		fclose(f);
		fflush(stdout);
		printf("+ served %d bytes\n", size);
	}
	else {
		httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
        return ESP_FAIL;
	}
    // End response
    httpd_resp_send_chunk(req, NULL, 0);

    //TODO: check back here

    // client has a page, so let's listen for events
    //websocket_task();


    return ESP_OK;
}

static const httpd_uri_t cap_post = {

    .uri = "/",
    .method = HTTP_POST,
    .handler = captive_post_handler
};

static const httpd_uri_t ws = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = ws_handler,
    .user_ctx = NULL,
    .is_websocket = true,               // Mandatory: set to `true` to handler websocket protocol
    .handle_ws_control_frames = false,  // Optional: set to `true` for the handler to receive control packets, too
        
};


static const httpd_uri_t allget = {
    .uri       = "/*",
    .method    = HTTP_GET,
    .handler   = spiff_serve,
    .user_ctx  = NULL
};



httpd_handle_t start_webserver( void )
{
    char* base_path = "/sdcard";

    /* Allocate memory for server data */
    server_data = calloc(1, sizeof(struct file_server_data));
    if (!server_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return NULL;
    }
    strlcpy(server_data->base_path, base_path,
            sizeof(server_data->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = 8192;

    init_spiff();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &ws));
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &cap_post));
        
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &allget));
        
        ESP_ERROR_CHECK(httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler));

        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void init_webserver(void){

    server_handle = start_webserver();
    if(server_handle == NULL){
        ESP_LOGE(TAG, "Failed to start web server");
    } else {
        ESP_LOGI(TAG, "Web server started successfully");
    }
}






















