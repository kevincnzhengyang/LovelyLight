/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-08 20:34:45
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-08 20:34:45
 * @FilePath    : /LovelyLight/components/sensor/include/sensors_task.h
 * @Description : vibration sensor and button
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include <freertos/portmacro.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
// #include "esp_adc/adc_continuous.h"

#include "active_task.h"
#include "iot_button.h"

class SensorsTask: public ActiveTask
{
    public:
        SensorsTask(const char *tName, uint32_t stackDepth, UBaseType_t priority,
                const BaseType_t coreId = tskNO_AFFINITY,
                size_t msgBuffSize = 0);

        ~SensorsTask();

    protected:
        void run(void);

    private:
        button_handle_t    _btn_breath;
        button_handle_t    _btn_random;
        button_handle_t     _btn_light;

        void      _initBatterySensor();

        void    _initVibrationSensor();

        void             _initButton();

        void     _getBatteryInfo(void);
};
