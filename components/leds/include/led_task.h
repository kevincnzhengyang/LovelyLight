/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-06 23:49:00
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-06 23:49:00
 * @FilePath    : /LovelyLight/component/leds/include/led_task.h
 * @Description : task for led
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#include "board_data.h"
#include "black_board.h"
#include "active_task.h"
#include "led_rgb.h"

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} led_color_t;

#define SIZE_LED_COLOR_T    sizeof(led_color_t)

enum LedCmdType
{
    LED_SLEEP,
    LED_WAKEUP,
    LED_TURNON,
    LED_TURNOFF,
    LED_BREATH_ON,
    LED_BREATH_OFF,
    LED_RANDOM_ON,
    LED_RANDOM_OFF,
    LED_INCR_H,
    LED_DECR_H,
    LED_INCR_S,
    LED_DECR_S,
    LED_INCR_V,
    LED_DECR_V,
    LED_INCR_R,
    LED_DECR_R,
    LED_INCR_G,
    LED_DECR_G,
    LED_INCR_B,
    LED_DECR_B,
    LED_CMD_BUTT
};

class LedsTask: public ActiveTask
{
    public:
        LedsTask(const char *tName, uint32_t stackDepth, UBaseType_t priority,
                const BaseType_t coreId = tskNO_AFFINITY,
                size_t msgBuffSize = (sizeof(char) + 4)*8);

        ~LedsTask(void);

    protected:
        void run(void);

    private:
        bool                _switch_on;
        bool                _breath_en;
        bool                _random_en;
        uint8_t                   _dir;
        bool                  _lazy_en;
        uint32_t         _lazy_counter;

        led_rgb_t               *_leds;
        led_color_t        *_led_color;

        void _breathLight(void);

        void _randomLight(void);

        void _handleCommand(LedCmdType cmd);
};
