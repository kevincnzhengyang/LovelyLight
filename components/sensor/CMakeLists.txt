idf_component_register(SRCS "sensors_task.cpp" "iot_button.c"
                    INCLUDE_DIRS "include"
                    REQUIRES board esp_adc_cal leds)