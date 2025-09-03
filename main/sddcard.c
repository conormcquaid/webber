/*
 * sddcard.c
 *
 *  Created on: Apr 8, 2017
 *      Author: Conor
 */




#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "oled_i2c.h"
#include "dirent.h"
#include <errno.h>
#include "sdcard.h"
#include "main.h"

static const char* TAG = "sdcard";

#define MOUNT_POINT "/sdcard"



DIR* dirp;
FILE* f = NULL;
int file_count = 0;
static uint32_t frame_number = 0;

void enumerate_files(void){
    struct dirent* dint;
    struct stat st;
    char fname[260];
    
    while((dint = readdir(dirp)) != NULL){
        if(dint->d_type == DT_REG){
            snprintf(fname, sizeof(fname) - 10, "%.10s/%.240s", MOUNT_POINT, dint->d_name);
            if(stat(fname, &st) == 0){
                ESP_LOGI(TAG, "Found file: %s, size %ld", dint->d_name, st.st_size);
                file_count++;
                if(st.st_size % 54 == 0){
                    ESP_LOGI(TAG, "File %s may be high-def*", dint->d_name);
                } else if(st.st_size % 15 == 0){
                    ESP_LOGW(TAG, "File %s may be Lo-fi*", dint->d_name);
                }
            } else {
                ESP_LOGE(TAG, "Stat failed for %s with errno %d", fname, errno);
            }
        } else if(dint->d_type == DT_DIR){
            ESP_LOGI(TAG, "Ignoring directory: %s", dint->d_name);
        } else {
            ESP_LOGI(TAG, "Found unknown item: %s", dint->d_name);
        }
    }
}

//TODO: handle errors from here
void open_next_file(tv_status_t* pTV) {
    if(f) {
        fclose(f);
        f = NULL;
    }               
    char fname[260];
    if (getNextFilename(fname, sizeof(fname)) != NULL) {
        if (set_cur_file(fname)) {
            ESP_LOGI(TAG, "Opened file %s successfully", fname);

            memcpy(pTV->current_file, fname, 260); //TODO: this is already statically allocated...

        } else {
            ESP_LOGE(TAG, "Failed to open file %s", fname);
        }
    } else {
        ESP_LOGI(TAG, "No more files to open");
    }
    frame_number = 0;
}


esp_err_t init_sd(void){

    esp_err_t ret;


    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    ESP_LOGI(TAG, "Using SDMMC peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();


    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // Set bus width to use:

    slot_config.width = 4;

    slot_config.clk = GPIO_NUM_16;
    slot_config.cmd = GPIO_NUM_6;
    slot_config.d0  = GPIO_NUM_7;
    slot_config.d1  = GPIO_NUM_35;
    slot_config.d2  = GPIO_NUM_10;
    slot_config.d3  = GPIO_NUM_9;

    slot_config.cd  = GPIO_NUM_8;   // CD
    slot_config.wp  = GPIO_NUM_NC;  // WP, not used


    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. ");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));

            //check_sd_card_pins(&config, pin_count);

        }
        return ret;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    const char root_dir[] = "/sdcard/";

    dirp = opendir(root_dir);
    if(dirp == NULL){
        ESP_LOGE(TAG, "Failed to open directory /sdcard");
        return ESP_ERR_NOT_FOUND;
    }
    // Enumerate files in the directory
    enumerate_files();

    //rewind and open first file in preparation for reading
    rewinddir(dirp);

    return ESP_OK;
}



int getFrame(uint8_t * pFrame){

	

	if(feof(f)){
		printf("Seek start\n");

        // TODO: return NULL here and let caller handle end of file
		fseek(f, SEEK_SET, 0);
		frame_number = 0;
	}
	// read 5x5 LEDs worth of data = 5x5x4 bytes
	int r = fread( pFrame, 1, g_frame_size, f);


	
	//char num[16];
	//sprintf(num, "%d", frame_number);
    if((frame_number % 32) == 0){
         printf("Frame: %04ld\n", frame_number);
    }

	//OLED_WriteBig(num,2,0);
	
	frame_number++;

    if(feof(f)){
        printf("End of file reached\n");
        return -1;
    }
    if(ferror(f)){
        printf("Error reading file\n");
        return -2;
    }
	return r;
}

char* getNextFilename(char* name, size_t len){

    int loop = 0;
    struct dirent* dp;

    if(dirp == NULL){
        ESP_LOGE(TAG, "Unexpected dirp null in getNextFilename\n");
        return NULL;
    }

    while(loop < 2){

        dp = readdir(dirp);

        // back to start again, but don't loop forever
        if(dp == NULL){
            ESP_LOGI(TAG, "Rewinding directory\n");
            rewinddir(dirp);
            loop++;

        }else if(dp->d_type == DT_REG){

            ESP_LOGI(TAG, "Getting next file %s\n", dp->d_name);
            strncpy(name, dp->d_name, len);
            return name;
            
        }else{
            ESP_LOGI(TAG, "Found non-file item %s",dp->d_name);
        }  

    }

    return NULL;
}

int set_cur_file(char *fn){

    char filename[260];

    strcpy(filename, "/sdcard/");
    strcat(filename, fn);

    fclose(f);// penalty if f not open?

    f = fopen(filename, "r");

    if(f == NULL){
        ESP_LOGE(TAG, "File open of %s failed with errno %d\n", filename, errno);
    }

    return (f!=NULL);

}