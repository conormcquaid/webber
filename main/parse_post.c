
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "nvs_mgr.h"

static const char *TAG = "parse";

// Converts a hex char to its integer value (e.g., 'A' -> 10, '5' -> 5)
static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0; // Should not happen with valid hex
}

/**
 * @brief Decodes URL-encoded string in-place.
 * Assumes the input string is null-terminated and writable.
 * Decodes '+' to space and %XX sequences.
 * @param encoded_str The null-terminated string to decode. It will be modified in-place.
 * @return A pointer to the decoded string (same as input pointer).
 */
char* url_decode_inplace(char* encoded_str) {
    char *read_ptr = encoded_str;
    char *write_ptr = encoded_str;

    while (*read_ptr != '\0') {
        if (*read_ptr == '+') {
            *write_ptr = ' ';
        } else if (*read_ptr == '%') {
            // Ensure there are at least two more characters for the hex code
            if (*(read_ptr + 1) != '\0' && *(read_ptr + 2) != '\0') {
                int val = hex_char_to_int(*(read_ptr + 1)) * 16 + hex_char_to_int(*(read_ptr + 2));
                *write_ptr = (char)val;
                read_ptr += 2; // Skip the two hex characters
            } else {
                // Malformed encoding, just copy the '%'
                *write_ptr = *read_ptr;
            }
        } else {
            *write_ptr = *read_ptr;
        }
        read_ptr++;
        write_ptr++;
    }
    *write_ptr = '\0'; // Null-terminate the decoded string
    return encoded_str;
}

int get_wifi_from_post(char* post, char** ssid, char** pwd, char** host, int* port){

    char* key = post;
    char* start = post;
	char* val;
	while(start && (val = strchr(start, '='))){

		*val = '\0';
		val++;

		key = start;

		// is there another k/v pair ahead?
		if((start = strchr(val, '&'))){
			*start = '\0';
			start++;
		}
		url_decode_inplace(key);
		url_decode_inplace(val);

		printf("Key: %s, Value: %s\n", key, val);

        if(0 == strcmp(SSID_KEY,  key)){ *ssid = strdup(val); }
        if(0 == strcmp(PWD_KEY,   key)){ *pwd  = strdup(val); }
        if(0 == strcmp(HOST_KEY,  key)){ *host = strdup(val); }
        if(0 == strcmp(PORT_KEY,  key)){ *port = atoi(val); }

	}
    ESP_LOGI(TAG, "get_wifi got %s and %s and %s and %d\n", *ssid, *pwd, *host, *port);
    return 0;
}