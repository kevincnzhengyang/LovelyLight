/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-07 19:13:18
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-07 19:13:18
 * @FilePath    : /LovelyLight/component/asr/include/asr_task.h
 * @Description : automatice speech recognition
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "active_task.h"

class AsrTask: public ActiveTask
{
    public:
        AsrTask(const char *tName, uint32_t stackDepth, UBaseType_t priority,
                const BaseType_t coreId = tskNO_AFFINITY,
                size_t msgBuffSize = 0);

        ~AsrTask(void);

    protected:
        void run(void);

    private:
        void _generateCommand(int cmd_id);
};
