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

#define MAX_LAZY_TIMES          500     // about 15000ms

#define LED_TAG "LEDs"
#define LED_DEBUG(fmt, ...)  ESP_LOGD(LED_TAG, fmt, ##__VA_ARGS__)
#define LED_INFO(fmt, ...)   ESP_LOGI(LED_TAG, fmt, ##__VA_ARGS__)
#define LED_WARN(fmt, ...)   ESP_LOGW(LED_TAG, fmt, ##__VA_ARGS__)
#define LED_ERROR(fmt, ...)  ESP_LOGE(LED_TAG, fmt, ##__VA_ARGS__)

LedsTask::LedsTask(const char *tName, uint32_t stackDepth, UBaseType_t priority,
        const BaseType_t coreId, size_t msgBuffSize) :
    ActiveTask {tName, stackDepth, priority, coreId, msgBuffSize},
    _switch_on {false},
    _breath_en {false},
    _random_en {false},
    _dir {0},
    _lazy_en {false},
    _lazy_counter {0},
    _leds {NULL},
    _led_color (NULL)
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
        if (NULL == _led_color)
        {
            LED_ERROR("Failed to get led_color %d", err);
            return;
        }
        else
        {
            LED_DEBUG("get led_color at %p", _led_color);
        }
        _led_color->r = _led_color->g = _led_color->b = 120;
        err = flushPublishData(bb_led);
        if (ESP_OK != err)
        {
            LED_ERROR("Failed to flush led_color %d", err);
            return;
        }
    }

    // prepare LEDs
    led_rgb_config_t rgb_config = LED_RGB_DEFAULT_CONFIG(CONFIG_PIN_LED_R,
            CONFIG_PIN_LED_G, CONFIG_PIN_LED_B);
    _leds = led_rgb_create(&rgb_config);

    if (!_leds) {
        LED_ERROR("install LED driver failed");
        return;
    }
    LED_INFO("led prepared");

    // apply value
    _leds->set_rgb(_leds, (uint32_t)_led_color->r, (uint32_t)_led_color->g,
            (uint32_t)_led_color->b);

    LED_INFO("running...");

    uint8_t cmd = (uint8_t)LedCmdType::LED_CMD_BUTT;
    // loop
    while (true)
    {
        // recv command
        if (sizeof(uint8_t) == getMessage(&cmd, sizeof(uint8_t), 10))
        {
            _handleCommand((LedCmdType)cmd);
        }

        // update light if in breath
        if (_switch_on && _breath_en) _breathLight();
        if (_switch_on && _random_en) _randomLight();

        delay(20);

        if (_lazy_en)
        {
            if (_lazy_counter++ > MAX_LAZY_TIMES)
            {
                // lazy flush data to NVS
                err = flushPublishData(bb_led);
                if (ESP_OK != err)
                {
                    LED_ERROR("Failed to lazy flush led_color %d", err);
                }
                _lazy_en = false;
                _lazy_counter = 0;
            }
        }
    }
}

void LedsTask::_breathLight(void)
{
    uint32_t h, s, v;
    _leds->get_hsv(_leds, &h, &s, &v);

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
}


void LedsTask::_randomLight(void)
{
    uint32_t h, s, v;
    _leds->get_hsv(_leds, &h, &s, &v);
    /**< Set a random color */
    h = esp_random() / 11930465;
    s = esp_random() / 42949673;
    s = s < 40 ? 40 : s;
    _leds->set_hsv(_leds, h, s, v);
}


void LedsTask::_handleCommand(LedCmdType cmd)
{
    switch (cmd)
    {
        case LedCmdType::LED_WAKEUP:
            {
                uint32_t v = 120;
                while (v < 250)
                {
                    _leds->set_hsv(_leds, 88, 255, v);
                    v += 10;
                    delay(20);
                }
                _leds->set_rgb(_leds, 0, 0, 0);
            }
            break;
        case LedCmdType::LED_TURNON:
            if (!_switch_on)
            {
                _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                        (uint32_t)_led_color->g, (uint32_t)_led_color->b);
                _switch_on = true;
            }
            break;
        case LedCmdType::LED_TURNOFF:
            if (_switch_on)
            {
                _leds->set_rgb(_leds, 0, 0, 0);
                _switch_on = false;
            }
            break;
        case LedCmdType::LED_BREATH_ON:
            if (!_switch_on)
            {
                _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                        (uint32_t)_led_color->g, (uint32_t)_led_color->b);
                _switch_on = true;
            }
            _breath_en = true;
            break;
        case LedCmdType::LED_BREATH_OFF:
            _breath_en = false;
            // lazy flush the interested v
            _leds->get_rgb(_leds, &_led_color->r, &_led_color->g, &_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_RANDOM_ON:
            if (!_switch_on)
            {
                _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                        (uint32_t)_led_color->g, (uint32_t)_led_color->b);
                _switch_on = true;
            }
            _random_en = true;
            break;
        case LedCmdType::LED_RANDOM_OFF:
            _random_en = false;
            // lazy flush the interested h & s
            _leds->get_rgb(_leds, &_led_color->r, &_led_color->g, &_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_INCR_H:
            {
                uint32_t h, s, v;
                _leds->get_hsv(_leds, &h, &s, &v);

                if (h < 245)
                {
                    h += 10;
                }

                _leds->set_hsv(_leds, h, s, v);
                _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
                _lazy_en = true;
            }
            break;
        case LedCmdType::LED_DECR_H:
            {
                uint32_t h, s, v;
                _leds->get_hsv(_leds, &h, &s, &v);

                if (h > 20)
                {
                    h -= 10;
                }

                _leds->set_hsv(_leds, h, s, v);
                _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
                _lazy_en = true;
            }
            break;
        case LedCmdType::LED_INCR_S:
            {
                uint32_t h, s, v;
                _leds->get_hsv(_leds, &h, &s, &v);

                if (s < 245)
                {
                    s += 10;
                }

                _leds->set_hsv(_leds, h, s, v);
                _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
                _lazy_en = true;
            }
            break;
        case LedCmdType::LED_DECR_S:
            {
                uint32_t h, s, v;
                _leds->get_hsv(_leds, &h, &s, &v);

                if (s > 20)
                {
                    s -= 10;
                }

                _leds->set_hsv(_leds, h, s, v);
                _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
                _lazy_en = true;
            }
            break;
        case LedCmdType::LED_INCR_V:
            {
                uint32_t h, s, v;
                _leds->get_hsv(_leds, &h, &s, &v);

                if (v < 245)
                {
                    v += 10;
                }

                _leds->set_hsv(_leds, h, s, v);
                _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
                _lazy_en = true;
            }
            break;
        case LedCmdType::LED_DECR_V:
            {
                uint32_t h, s, v;
                _leds->get_hsv(_leds, &h, &s, &v);

                if (v > 20)
                {
                    v -= 10;
                }

                _leds->set_hsv(_leds, h, s, v);
                _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
                _lazy_en = true;
            }
            break;
        case LedCmdType::LED_INCR_R:
            if (_led_color->r < 245)
            {
                _led_color->r += 10;
            }

            _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                    (uint32_t)_led_color->g, (uint32_t)_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_DECR_R:
            if (_led_color->r > 20)
            {
                _led_color->r -= 10;
            }

            _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                    (uint32_t)_led_color->g, (uint32_t)_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_INCR_G:
            if (_led_color->g < 245)
            {
                _led_color->g += 10;
            }

            _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                    (uint32_t)_led_color->g, (uint32_t)_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_DECR_G:
            if (_led_color->g > 20)
            {
                _led_color->g -= 10;
            }

            _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                    (uint32_t)_led_color->g, (uint32_t)_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_INCR_B:
            if (_led_color->b < 245)
            {
                _led_color->b += 10;
            }

            _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                    (uint32_t)_led_color->g, (uint32_t)_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_DECR_B:
            if (_led_color->b > 20)
            {
                _led_color->b -= 10;
            }

            _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                    (uint32_t)_led_color->g, (uint32_t)_led_color->b);
            _lazy_en = true;
            break;
        default:
            LED_WARN("unknow command type %d", cmd);
            break;
    }
}
