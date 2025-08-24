idf_component_register(SRCS "ui/ui_state.c" "logo.c" "oled_i2c.c" "sddcard.c" "super.c"  "rotary_encoder.c" "parse_post.c" "websock.c" "server.c" "wifi_man.c" "main.c" "creds.c" "nvs_mgr.c"
                    INCLUDE_DIRS "." "../include" )

spiffs_create_partition_image( spart ../spiff FLASH_IN_PROJECT)