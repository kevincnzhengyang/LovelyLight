/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-07 00:01:44
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-07 00:01:44
 * @FilePath    : /LovelyLight/component/leds/led_task.cpp
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include "esp_err.h"
#include "esp_log.h"

#include "led_task.h"

#define LED_TAG "LEDs"
#define LED_DEBUG(fmt, ...)  ESP_LOGD(LED_TAG, fmt, ##__VA_ARGS__)
#define LED_INFO(fmt, ...)   ESP_LOGI(LED_TAG, fmt, ##__VA_ARGS__)
#define LED_WARN(fmt, ...)   ESP_LOGW(LED_TAG, fmt, ##__VA_ARGS__)
#define LED_ERROR(fmt, ...)  ESP_LOGE(LED_TAG, fmt, ##__VA_ARGS__)

LedsTask::LedsTask(const char *tName, uint32_t stackDepth, UBaseType_t priority,
        const BaseType_t coreId = tskNO_AFFINITY,
        size_t msgBuffSize = 256) :
    ActiveTask {tName, stackDepth, priority, coreId, msgBuffSize},
    _breath_en {true},
    _dir {0},
    _leds {NULL},
    _led_color (NULL),
    _led_color_wbk {NULL}
{
    LED_INFO("Task Created");
}

LedsTask::~LedsTask(void)
{
    LED_INFO("Task Destroyed");
}

void LedsTask::run(void)
{
    // load data from NVS
    LED_INFO("loading...");

    const char bb_led[] = "led_color\0";
    esp_err_t err;
    _led_color = getBlackBoardData(bb_led, led_color_t);
    if (NULL == _led_color)
    {
        // try to register
        err = registPersistData(bb_led, SIZE_LED_COLOR_T);
        if (ESP_OK != err)
        {
            LED_ERROR("Failed to register led_color %d", err);
            return;
        }
        // init value
        _led_color = getBlackBoardData(bb_led, led_color_t);
        _led_color->r = _led_color->g = _led_color->b = 120;
        err = flushPublishData(bb_led);
        if (ESP_OK != err)
        {
            LED_ERROR("Failed to flush led_color %d", err);
            return;
        }
    }

    // apply value
    _leds->set_rgb(_leds, _led_color->r, _led_color->g, _led_color->b);

    // prepare LEDs
    led_rgb_config_t rgb_config = LED_RGB_DEFAULT_CONFIG(CONFIG_PIN_LED_R,
            CONFIG_PIN_LED_G, CONFIG_PIN_LED_B);
    _leds = led_rgb_create(&rgb_config);

    if (!_leds) {
        LED_ERROR("install LED driver failed");
        return;
    }

    LED_INFO("running...");

    // loop
    while (true)
    {
        // recv command

        // update light if in breath
        if (_breath_en) _breath_light();
    }

}

void LedsTask::_breath_light(void)
{
    uint32_t h, s, v;
    g_leds->get_hsv(g_leds, &h, &s, &v);

    if (_dir)
    {
        v--;

        if (v < 20)
        {
            _dir = 0;
        }
    }
    else
    {
        v++;

        if (v >= 60)
        {
            _dir = 1;
        }
    }

    _leds->set_hsv(_leds, h, s, v);
    delay(20);
}
