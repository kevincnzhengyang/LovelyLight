/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-06 01:41:31
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-06 01:41:31
 * @FilePath    : /LovelyLight/component/board/task_map.cpp
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <string>
#include "esp_log.h"
#include "task_map.h"

static std::map<std::string, ActiveTask *>                      gTaskMap;
typedef std::map<std::string, ActiveTask *>::const_iterator  TaskMapIter;
typedef std::map<std::string, ActiveTask *>::value_type TaskMapValueType;

bool registTask(ActiveTask *task)
{
    if (NULL == task) return false;
    TaskMapIter iter = gTaskMap.find(task->_name);
    if (gTaskMap.end() != iter)
    {
        // existed, free old
        ActiveTask *old = iter->second;
        delete old;
    }
    gTaskMap.insert(TaskMapValueType(std::string(task->_name), task));
    return true;
}

void purgeTask(ActiveTask *task)
{
    if (NULL == task) return;
    TaskMapIter iter = gTaskMap.find(task->_name);
    if (gTaskMap.end() == iter)
    {
        // not existed
        return;
    }
    ActiveTask *old = iter->second;
    delete old;
    gTaskMap.erase(iter);
}

ActiveTask *getTaskByName(const char *tname)
{
    TaskMapIter iter = gTaskMap.find(tname);
    return (gTaskMap.end() == iter) ? NULL : iter->second;
}

size_t SendMessageTo(const char *name, const void * pvTxData,
        size_t xDataLengthBytes, uint32_t msWait)
{
    ActiveTask * task = getTaskByName(name);
    if (NULL == task) return 0;

    return task->putMessage(pvTxData, xDataLengthBytes, msWait);
}

void printTasks(void)
{
    // print information of all tasks
    // name status free state
    TaskStatus_t xTaskDetails;

    ESP_LOGI("Tasks", "core name id state priority ticks base highwater");
    for(TaskMapIter iter = gTaskMap.begin(); iter != gTaskMap.end(); iter++)
    {
        vTaskGetInfo(iter->second->getHandle(), &xTaskDetails, pdTRUE, eInvalid);
        ESP_LOGI("Tasks",
            "%d\t%s\t%d\t%d\t%d\t%d\t%p\t0x%x\n",
            xTaskDetails.xCoreID, xTaskDetails.pcTaskName, xTaskDetails.xTaskNumber,
            xTaskDetails.eCurrentState, xTaskDetails.uxCurrentPriority,
            xTaskDetails.ulRunTimeCounter, xTaskDetails.pxStackBase,
            xTaskDetails.usStackHighWaterMark);
    }
}
