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

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} led_color_t;

#define SIZE_LED_COLOR_T    sizeof(led_color_t)

class LedsTask: public ActiveTask
{
    public:
        LedsTask(const char *tName, uint32_t stackDepth, UBaseType_t priority,
                const BaseType_t coreId = tskNO_AFFINITY,
                size_t msgBuffSize = 256);

        ~LedsTask(void);

    protected:
        void run(void);

    private:
        bool                _breath_en;
        uint8_t                   _dir;

        led_rgb_t               *_leds;
        led_color_t        *_led_color;
        led_color_t    *_led_color_wbk;

        void _breath_light(void);
}
