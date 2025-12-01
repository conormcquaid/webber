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
static const char* gnvfTAG = "gnvf";

#define MOUNT_POINT "/sdcard"

static tv_runtime_status_t* pRuntime = NULL;

DIR* dirp;
FILE* f = NULL;
int file_count = 0;
static uint32_t frame_number = 0;
static struct stat st;
char filepath[260];
static int gnvfCount = 0; // get_next_valid_file file count


/*
Finds and opens the next valid file.
If loop is false, stops at end of current directory, otherwise will loop 
back to start, facilitataing 'find previous'
If resolution is not 0, the found file must be a multiple of that resolution
Returns 1 if a file was found and opened, else 0
*/

int get_next_valid_file(bool loop, int resolution){

    bool file_found = false;
    bool looped = false;

    while(!file_found && !looped){

        struct dirent* dint = readdir(dirp);
        if(dint == NULL){
            if(loop){
                looped = true;
                rewinddir(dirp);
                dint = readdir(dirp);
                if(dint == NULL){
                    ESP_LOGI(gnvfTAG,   "==>No files found on sdcard\n");
                    return 0;
                }
            } else {
                ESP_LOGI(gnvfTAG,   "==>No more files found on sdcard\n");
                return 0;
            }
        }
        if(dint->d_type == DT_REG){

            snprintf(filepath, sizeof(filepath) - 10, "%.10s/%.240s", MOUNT_POINT, dint->d_name);

            if(stat(filepath, &st) == 0){  //<== filepath!!

                if(resolution){
                    if(st.st_size % resolution == 0){
                        file_found = true;
                    } 
                } else {
                    file_found = true;  
                }
                if(file_found){
                    
                    ESP_LOGI(gnvfTAG, "Found valid file %s, size %ld\n", dint->d_name, st.st_size);
                    pRuntime->file_size_bytes = st.st_size;
                    pRuntime->frame_number = 0;
                    //pRuntime->file_frame_count = 0;
                    strncpy(pRuntime->current_file, dint->d_name, 260);

                    if(f){
                        fclose(f);
                        f = NULL;
                        ESP_LOGI(gnvfTAG, " Closed previous file");
                    }
                    
                    f = fopen(filepath, "r");
                    if(f == NULL){
                        ESP_LOGI(gnvfTAG,  "==>Failed to open file %s\n", dint->d_name);
                        file_found = false;
                    } else {
                        ESP_LOGI(gnvfTAG, "Opened file %s, size %ld\n", dint->d_name, st.st_size);
                        //fclose(f);
                        return 1;
                    }
                    
                } 
            } 
    
        }
    }
    return 0; // no luck
}

int get_previous_valid_file(bool loop, int resolution){
    if(gnvfCount == 0){
        ESP_LOGI(gnvfTAG, "No previous file: count is zero");
        return 0; 
    }
    if(gnvfCount == 1){
        ESP_LOGI(gnvfTAG, "No previous file: count is one");
        return 0; 
    }
    for(int i = 0; i < (gnvfCount - 1); i++){
        get_next_valid_file(loop, resolution);
    }
    return 1;
}   

int get_next_random_file(bool loop, int resolution){
    if(gnvfCount < 2){
        ESP_LOGI(gnvfTAG, "No random file: count is less than two");
        return 0; 
    }
    int r = rand() % (gnvfCount-1); // don't want to get the same file again
    for(int i = 0; i < r; i++){
        get_next_valid_file(loop, resolution);
    }
    return 1;
}

void enumerate_files(void){
    struct dirent* dint;
    //char fname[260];
    
    while((dint = readdir(dirp)) != NULL){
        if(dint->d_type == DT_REG){
            snprintf(filepath, sizeof(filepath) - 10, "%.10s/%.240s", MOUNT_POINT, dint->d_name);
            if(stat(filepath, &st) == 0){
                ESP_LOGI(TAG, "Found file: %s, size %ld", dint->d_name, st.st_size);
                file_count++;
                if(st.st_size % 54 == 0){
                    ESP_LOGI(TAG, "File %s may be high-def*", dint->d_name);
                } else if(st.st_size % 15 == 0){
                    ESP_LOGW(TAG, "File %s may be Lo-fi*", dint->d_name);
                }
            } else {
                ESP_LOGE(TAG, "Stat failed for %s with errno %d", filepath, errno);
            }
        } else if(dint->d_type == DT_DIR){
            ESP_LOGI(TAG, "Ignoring directory: %s", dint->d_name);
        } else {
            ESP_LOGI(TAG, "Found unknown item: %s", dint->d_name);
        }
    }
}

//TODO: handle errors from here
void open_next_file(tv_runtime_status_t* pTV) {
    if(f) {
        fclose(f);
        f = NULL;
    }               
    char fname[260];
    if (getNextFilename(fname, sizeof(fname)) != NULL) {
        if (set_cur_file(fname)) {
            ESP_LOGI(TAG, "Opened file %s successfully", fname);

            if(stat(filepath, &st) == 0){

                pTV->file_size_bytes = st.st_size;
                pTV->file_frame_count = st.st_size / 54; // TODO: this belongs elsewhere, or how do I know resolution?
            }else{
                ESP_LOGI(TAG, "Stat failed for %s: error %d", fname, errno);
            }
            
            
            memcpy(pTV->current_file, fname, 260); //TODO: this is already statically allocated...

        } else {
            ESP_LOGE(TAG, "Failed to open file %s", fname);
        }
    } else {
        ESP_LOGI(TAG, "No more files to open");
    }
    frame_number = 0;
}


esp_err_t init_sd(tv_runtime_status_t* pRT){

    pRuntime = pRT;

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
    //enumerate_files();

    //rewind 
    rewinddir(dirp);


    // int gnvfCount = 0;
    // while((get_next_valid_file(false, 54)) != 0){
    //     ESP_LOGI(gnvfTAG, "[%02d] %s\n", gnvfCount, pRuntime->current_file);
    //     gnvfCount++;
    // }
    // rewinddir(dirp);

    // get_next_valid_file(true, 54);
    // ESP_LOGI(gnvfTAG, "%s should be the first file", pRuntime->current_file);

    // get_next_valid_file(true, 54);
    // ESP_LOGI(gnvfTAG, "%s should be the second file", pRuntime->current_file);

    // for(int i = 0; i <(gnvfCount-1); i++){
    //     get_next_valid_file(true, 54);
    // }
    // ESP_LOGI(gnvfTAG, "%s should be the first file again", pRuntime->current_file);


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
    // if((frame_number % 32) == 0){
    //     uint8_t pct = (100.0 * frame_number) / pRuntime->file_frame_count ;
    //      printf("Frame: %04ld / %04ld %d %%\n", pRuntime->frame_number, pRuntime->file_frame_count, pct);
    //      //pRuntime->frame_number = frame_number;
    // }

	//OLED_WriteBig(num,2,0);
	
	frame_number++;

    if(feof(f)){
        printf("End of file reached\n");
        return -1   ;
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

    strcpy(filepath, "/sdcard/");
    strcat(filepath, fn);

    fclose(f);// penalty if f not open?

    f = fopen(filepath, "r");

    if(f == NULL){
        ESP_LOGE(TAG, "File open of %s failed with errno %d\n", filepath, errno);
    }

    return (f!=NULL);

}