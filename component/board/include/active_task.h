/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-05 23:37:27
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-05 23:48:46
 * @FilePath    : /LovelyLight/component/board/include/ActiveTask.h
 * @Description : Active Task
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/message_buffer.h"
#include "freertos/semphr.h"

class ActiveTask
{
    public:
        ActiveTask(const char *tName, uint32_t stackDepth, UBaseType_t priority,
                const BaseType_t coreId = tskNO_AFFINITY,
                size_t msgBuffSize = 256);

        virtual ~ActiveTask();

        TaskHandle_t getHandle(void);

        MessageBufferHandle_t getMessageBuffer(void);

        void begin(void);

        size_t putMessage(const void * pvTxData, size_t xDataLengthBytes,
                uint32_t msWait);

        char                    *_name;     // name of task

    protected:
        size_t getMessage(void * pvRxData, size_t xBufferLengthBytes,
                uint32_t msWait);

        virtual void run(void) = 0;

    private:
        static void taskFunction(void *param)
        {
            ActiveTask *task = (ActiveTask *)param;
            task->run();
        }

        uint32_t           _stackDepth;     // stack depth of freeRTOS task
        UBaseType_t          _priority;     // priority
        TaskHandle_t       _taskHandle;     // task
        const BaseType_t       _coreId;     // cpu core id

        MessageBufferHandle_t _msgBuff;
        size_t            _msgBuffSize;
        SemaphoreHandle_t       _mutex;
};
