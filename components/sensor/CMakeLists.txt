idf_component_register(SRCS "sensors_task.cpp" "iot_button.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_rom esp_adc esp_timer board leds)