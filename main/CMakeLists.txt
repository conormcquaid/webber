idf_component_register(SRCS "parse_post.c" "websock.c" "server.c" "wifi_man.c" "main.c" "creds.c" "nvs_mgr.c"
                    INCLUDE_DIRS ".")


spiffs_create_partition_image( spart ../spiff FLASH_IN_PROJECT)