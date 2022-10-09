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
#include "esp_random.h"

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

    LED_INFO("led preparing...");
    // prepare LEDs
    led_rgb_config_t rgb_config = LED_RGB_DEFAULT_CONFIG(CONFIG_PIN_LED_R,
            CONFIG_PIN_LED_G, CONFIG_PIN_LED_B);
    _leds = led_rgb_create(&rgb_config);

    if (!_leds) {
        LED_ERROR("install LED driver failed");
        return;
    }

    LED_INFO("running...");

    uint8_t cmd = (uint8_t)LedCmdType::LED_CMD_BUTT;
    uint32_t last = millis();
    // loop
    while (true)
    {
        uint32_t now = millis();
        if (now - last >= pdMS_TO_TICKS(5000))
        {
            LED_DEBUG("reset watch dog");
            last = now;
            esp_task_wdt_reset();  // feed watch dog
        }

        // recv command
        if (sizeof(uint8_t) == getMessage(&cmd, sizeof(uint8_t), 10))
        {
            LED_DEBUG("recv command %d", cmd);
            _handleCommand((LedCmdType)cmd);
        }

        // update light if in breath
        if (_switch_on && _breath_en) _breathLight();
        if (_switch_on && _random_en) _randomLight();

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
        delay(10);
    }
}

void LedsTask::_breathLight(void)
{
    // LED_DEBUG("in breath light");
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

        if (v >= 340)
        {
            _dir = 1;
        }
    }

    _leds->set_hsv(_leds, h, s, v);
}


void LedsTask::_randomLight(void)
{
    // LED_DEBUG("in random light");
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
        case LedCmdType::LED_SLEEP:
            LED_DEBUG("cmd Sleep");
            break;
        case LedCmdType::LED_WAKEUP:
            {
                LED_DEBUG("cmd Wakeup");
                if (!_switch_on)
                {
                    uint32_t v = 20;
                    while (v < 100)
                    {
                        _leds->set_hsv(_leds, 60, 100, v);
                        v++;
                        delay(10);
                    }
                    for (int i = 0; i < 3; i++)
                    {
                        esp_task_wdt_reset();  // feed watch dog
                        delay(5000);
                    }
                    esp_task_wdt_reset();  // feed watch dog
                    while (v > 2)
                    {
                        _leds->set_hsv(_leds, 60, 100, v);
                        v--;
                        delay(10);
                    }
                    _leds->set_rgb(_leds, 0, 0, 0);
                }
            }
            break;
        case LedCmdType::LED_TURNON:
            LED_DEBUG("cmd TurOn");
            if (!_switch_on)
            {
                _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                        (uint32_t)_led_color->g, (uint32_t)_led_color->b);
                _switch_on = true;
            }
            break;
        case LedCmdType::LED_TURNOFF:
            LED_DEBUG("cmd TurnOff");
            if (_switch_on)
            {
                _leds->set_rgb(_leds, 0, 0, 0);
                _switch_on = false;
            }
            break;
        case LedCmdType::LED_BREATH_ON:
            LED_DEBUG("cmd Breath On");
            if (!_switch_on)
            {
                _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                        (uint32_t)_led_color->g, (uint32_t)_led_color->b);
                _switch_on = true;
            }
            _breath_en = true;
            break;
        case LedCmdType::LED_BREATH_OFF:
            LED_DEBUG("cmd Breath Off");
            _breath_en = false;
            // lazy flush the interested v
            _leds->get_rgb(_leds, &_led_color->r, &_led_color->g, &_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_RANDOM_ON:
            LED_DEBUG("cmd Random On");
            if (!_switch_on)
            {
                _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                        (uint32_t)_led_color->g, (uint32_t)_led_color->b);
                _switch_on = true;
            }
            _randomLight();
            _leds->get_rgb(_leds, &_led_color->r, &_led_color->g, &_led_color->b);
            _lazy_en = true;
            // _random_en = true;
            break;
        case LedCmdType::LED_RANDOM_OFF:
            LED_DEBUG("cmd Random Off");
            _random_en = false;
            // lazy flush the interested h & s
            _leds->get_rgb(_leds, &_led_color->r, &_led_color->g, &_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_INCR_H:
            LED_DEBUG("cmd Increase H");
            {
                uint32_t h, s, v;
                _leds->get_hsv(_leds, &h, &s, &v);

                if (h < 360)
                {
                    h += 10;
                }

                _leds->set_hsv(_leds, h, s, v);
                _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
                _lazy_en = true;
            }
            break;
        case LedCmdType::LED_DECR_H:
            LED_DEBUG("cmd Decrease H");
            {
                uint32_t h, s, v;
                _leds->get_hsv(_leds, &h, &s, &v);

                if (h > 10)
                {
                    h -= 10;
                }

                _leds->set_hsv(_leds, h, s, v);
                _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
                _lazy_en = true;
            }
            break;
        case LedCmdType::LED_INCR_S:
            LED_DEBUG("cmd Increase S");
            {
                uint32_t h, s, v;
                _leds->get_hsv(_leds, &h, &s, &v);

                if (s < 360)
                {
                    s += 10;
                }

                _leds->set_hsv(_leds, h, s, v);
                _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
                _lazy_en = true;
            }
            break;
        case LedCmdType::LED_DECR_S:
            LED_DEBUG("cmd Decrease S");
            {
                uint32_t h, s, v;
                _leds->get_hsv(_leds, &h, &s, &v);

                if (s > 10)
                {
                    s -= 10;
                }

                _leds->set_hsv(_leds, h, s, v);
                _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
                _lazy_en = true;
            }
            break;
        case LedCmdType::LED_INCR_V:
            LED_DEBUG("cmd Increase V");
            {
                uint32_t h, s, v;
                _leds->get_hsv(_leds, &h, &s, &v);

                if (v < 360)
                {
                    v += 10;
                }

                _leds->set_hsv(_leds, h, s, v);
                _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
                _lazy_en = true;
            }
            break;
        case LedCmdType::LED_DECR_V:
            LED_DEBUG("cmd Decrease V");
            {
                uint32_t h, s, v;
                _leds->get_hsv(_leds, &h, &s, &v);

                if (v > 10)
                {
                    v -= 10;
                }

                _leds->set_hsv(_leds, h, s, v);
                _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
                _lazy_en = true;
            }
            break;
        case LedCmdType::LED_INCR_R:
            LED_DEBUG("cmd Increase R");
            if (_led_color->r < 245)
            {
                _led_color->r += 10;
            }

            _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                    (uint32_t)_led_color->g, (uint32_t)_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_DECR_R:
            LED_DEBUG("cmd Decrease R");
            if (_led_color->r > 20)
            {
                _led_color->r -= 10;
            }

            _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                    (uint32_t)_led_color->g, (uint32_t)_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_INCR_G:
            LED_DEBUG("cmd Increase G");
            if (_led_color->g < 245)
            {
                _led_color->g += 10;
            }

            _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                    (uint32_t)_led_color->g, (uint32_t)_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_DECR_G:
            LED_DEBUG("cmd Decrease G");
            if (_led_color->g > 20)
            {
                _led_color->g -= 10;
            }

            _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                    (uint32_t)_led_color->g, (uint32_t)_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_INCR_B:
            LED_DEBUG("cmd Increase B");
            if (_led_color->b < 245)
            {
                _led_color->b += 10;
            }

            _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                    (uint32_t)_led_color->g, (uint32_t)_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_DECR_B:
            LED_DEBUG("cmd Decrease B");
            if (_led_color->b > 20)
            {
                _led_color->b -= 10;
            }

            _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                    (uint32_t)_led_color->g, (uint32_t)_led_color->b);
            _lazy_en = true;
            break;
        case LedCmdType::LED_SWITCH:
            LED_DEBUG("cmd Switch");
            if (!_switch_on)
            {
                _leds->set_rgb(_leds, (uint32_t)_led_color->r,
                        (uint32_t)_led_color->g, (uint32_t)_led_color->b);
                _switch_on = true;
            }
            else
            {
                _leds->set_rgb(_leds, 0, 0, 0);
                _switch_on = false;
            }
            break;
        case LedCmdType::LED_ROLL_V:
            LED_DEBUG("cmd Roll V");
            if (!_switch_on) break;
            static bool dir_incr;
            uint32_t h, s, v;
            _leds->get_hsv(_leds, &h, &s, &v);

            if (dir_incr)
            {
                v += 10;
                _leds->set_hsv(_leds, h, s, v);
                if (v > 340)
                {
                    dir_incr = false;

                }
            }
            else
            {
                v -= 10;
                _leds->set_hsv(_leds, h, s, v);
                if (v < 20)
                {
                    dir_incr = true;

                }
            }

            _leds->get_rgb(_leds, &_led_color->r, &_led_color->r, &_led_color->r);
            _lazy_en = true;
            break;
        default:
            LED_WARN("unknow command type %d", cmd);
            break;
    }
}
