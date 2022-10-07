/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-05 18:58:37
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-05 19:13:47
 * @FilePath    : /LovelyLight/main/main.cpp
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-05 17:44:27
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-05 17:44:28
 * @FilePath    : /LovelyLight/app/test/main.cpp
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "black_board.h"
#include "task_map.h"
#include "led_task.h"


#define APP_TAG "App"
#define APP_DEBUG(fmt, ...)  ESP_LOGD(APP_TAG, fmt, ##__VA_ARGS__)
#define APP_INFO(fmt, ...)   ESP_LOGI(APP_TAG, fmt, ##__VA_ARGS__)
#define APP_WARN(fmt, ...)   ESP_LOGW(APP_TAG, fmt, ##__VA_ARGS__)
#define APP_ERROR(fmt, ...)  ESP_LOGE(APP_TAG, fmt, ##__VA_ARGS__)

extern "C" void app_main()
{
    // set log level
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("LEDs", ESP_LOG_DEBUG);
    esp_log_level_set("BlackBoard", ESP_LOG_DEBUG);

    APP_INFO("Startup..");
    APP_INFO("Free Heap Size: %d bytes", esp_get_free_heap_size());
    APP_INFO("IDF version: %s", esp_get_idf_version());

    // create event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /**< Initialize NVS */
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    APP_INFO("NVS flash init");

    initBlackBoard(1024);
    APP_INFO("BlackBoard init");

    LedsTask *ledsTask = new LedsTask("ledsTask", 4096, 1, 0);
    assert("Failed to create leds task" && NULL != ledsTask);
    APP_INFO("LED Task ready");

    assert(registTask(ledsTask));
    APP_INFO("LED Task registed");

    ledsTask->begin();
    delay(2000);

    // uint8_t cmd = (uint8_t)LedCmdType::LED_WAKEUP;
    // assert(sizeof(uint8_t) == sendMessageTo("ledsTask", &cmd, sizeof(uint8_t), 20));

    while (1) {
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
