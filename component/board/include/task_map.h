/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-06 01:41:43
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-06 01:41:43
 * @FilePath    : /LovelyLight/component/board/include/task_map.h
 * @Description : task map
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#pragma once

#include <map>
#include "active_task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Due to STL Container implementation,
 *
 * hereifafter only ONE writer allowed to use registerTask and/or purgeTask
 */

bool registTask(ActiveTask *task);

void purgeTask(ActiveTask *task);

ActiveTask *getTaskByName(const char *tname);

size_t sendMessageTo(const char *name, const void * pvTxData,
        size_t xDataLengthBytes, uint32_t msWait);

void printTasks(void);

#ifdef __cplusplus
}
#endif
