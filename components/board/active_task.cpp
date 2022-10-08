/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-06 00:43:02
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-06 00:43:03
 * @FilePath    : /LovelyLight/component/board/active_task.cpp
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include "active_task.h"

#include <string.h>

ActiveTask::ActiveTask(const char *tName, uint32_t stackDepth,
        UBaseType_t priority, const BaseType_t coreId,
        size_t msgBuffSize):
            _stackDepth {stackDepth},
            _priority {priority},
            _coreId {coreId},
            _msgBuffSize {msgBuffSize}
{
    _name = strdup(tName);
}

ActiveTask::~ActiveTask()
{
    free(_name);
    vTaskDelete(_taskHandle);
    vMessageBufferDelete(_taskHandle);
}

TaskHandle_t ActiveTask::getHandle(void)
{
    return _taskHandle;
}

MessageBufferHandle_t ActiveTask::getMessageBuffer(void)
{
    return _msgBuff;
}

void ActiveTask::begin(void)
{
    if (_msgBuffSize > 4)
    {
        _msgBuff = xMessageBufferCreate(_msgBuffSize);
        assert("Failed to create message buffer" && _msgBuff != NULL);

        _mutex = xSemaphoreCreateMutex();
        assert("Failed to create mutex" && _mutex != NULL);
    }

    BaseType_t result = xTaskCreatePinnedToCore(taskFunction, _name,
            _stackDepth, this, _priority, &_taskHandle, _coreId);
    assert("Failed to create task" && result == pdPASS);
}

size_t ActiveTask::putMessage(const void * pvTxData, size_t xDataLengthBytes,
        uint32_t msWait)
{
    xSemaphoreTake(_mutex, pdMS_TO_TICKS(msWait));
    size_t size = xMessageBufferSend(_msgBuff, pvTxData, xDataLengthBytes,
            pdMS_TO_TICKS(msWait));
    xSemaphoreGive(_mutex);
    return size;
}

size_t ActiveTask::getMessage(void * pvRxData, size_t xBufferLengthBytes,
        uint32_t msWait)
{
    return xMessageBufferReceive(_msgBuff, pvRxData, xBufferLengthBytes,
            pdMS_TO_TICKS(msWait));
}
