/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-06 14:56:38
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-06 14:56:38
 * @FilePath    : /LovelyLight/component/board/include/back_board.h
 * @Description : Back Board for inner data publish, loading and storing
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#pragma once

#include <string.h>
#include <stdlib.h>

#include "esp_err.h"

/**
 * just simple singlton but not thread safe
 *
 * assume to be constructed by app_main
 *
 * and also assume NVS init flash already done
 */

#ifdef __cplusplus
extern "C" {
#endif

void initBlackBoard(size_t buffSize);

void finiBlackBoard(void);

void *getPublishData(const char *key);

esp_err_t registBlackBoardData(const char *key, size_t dataSize,
        bool persisted, uint32_t msWait = 20);

esp_err_t flushPublishData(const char *key);

#define getBlackBoardData(key, T)  ((T *)getPublishData(key))

#define registPersistData(key, dataSize) registBlackBoardData(key, dataSize, true)

#define registTemperaryData(key, dataSize) registBlackBoardData(key, dataSize, false)

#ifdef __cplusplus
}
#endif
