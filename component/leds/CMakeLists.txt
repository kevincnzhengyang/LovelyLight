idf_component_register(SRCS "board_init.c" "board_config.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash blufi button iap ld-mqtt)