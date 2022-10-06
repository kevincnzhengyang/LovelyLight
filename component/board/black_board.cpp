/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-06 14:56:48
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-06 14:56:48
 * @FilePath    : /LovelyLight/component/board/black_board.cpp
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */

#include <map>

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "board_data.h"
#include "black_board.h"

#define BB_TAG "BlackBoard"
#define BB_DEBUG(fmt, ...)  ESP_LOGD(BB_TAG, fmt, ##__VA_ARGS__)
#define BB_INFO(fmt, ...)   ESP_LOGI(BB_TAG, fmt, ##__VA_ARGS__)
#define BB_WARN(fmt, ...)   ESP_LOGW(BB_TAG, fmt, ##__VA_ARGS__)
#define BB_ERROR(fmt, ...)  ESP_LOGE(BB_TAG, fmt, ##__VA_ARGS__)

typedef struct
{
    bool                     persisted;
    size_t                    dataSize;
    uint8_t                     *rdPtr;
} BB_DataBlock;

typedef struct
{
    size_t                    buffSize;
    uint8_t                      *base;
    uint8_t                     *wrPtr;
    SemaphoreHandle_t          wrMutex;
    nvs_handle_t                  hNVS;
} BB_DataBuffer;

static std::map<std::string, BB_DataBlock>                     gDataMap;
typedef std::map<std::string, BB_DataBlock>::const_iterator DataMapIter;
typedef std::map<std::string, BB_DataBlock>::value_type   DataValueType;

static BB_DataBuffer gDataBuffer;

static bool hasEnoughSpace(size_t s)
{
    return s > gDataBuffer.buffSize ? false :
        s <= (gDataBuffer.buffSize - (gDataBuffer.wrPtr - gDataBuffer.base));
}

void initBlackBoard(size_t buffSize)
{
    if (NULL != gDataBuffer.base) return; // already inited
    gDataBuffer.buffSize = buffSize;
    gDataBuffer.base = (uint8_t *)malloc(buffSize);
    assert("Failed to malloc for Black Board" && NULL != gDataBuffer.base);
    gDataBuffer.wrPtr = gDataBuffer.base;
    gDataBuffer.wrMutex = xSemaphoreCreateMutex();
    assert("Failed to create mutex for Black Board" && NULL != gDataBuffer.wrMutex);

    // open NVS
    esp_err_t err = nvs_open("BlackBoard", NVS_READWRITE, &gDataBuffer.hNVS);
    assert("Failed to open NVS" && ESP_OK == err);

    // find blackboard namespace in partition NVS_DEFAULT_PART_NAME (“nvs”)
    nvs_iterator_t head = nvs_entry_find(NULL, "BlackBoard", NVS_TYPE_BLOB);
    nvs_iterator_t entry = nullptr;
    nvs_entry_info_t info;

    BB_INFO("open NVS ok");

    size_t cap = gDataBuffer.buffSize;
    size_t length = 0;
    while (NULL != (entry = nvs_entry_next(head)))
    {
        nvs_entry_info(entry, &info);
        BB_INFO("loading %s from NVS", info.key);
        err = nvs_get_blob(gDataBuffer.hNVS, info.key,
                gDataBuffer.wrPtr, &length);
        if (ESP_OK != err)
        {
            BB_ERROR("load %s failed due to %d", info.key, err);
            assert(false);
        }

        err = registBlackBoardData(info.key, length, true, 10);
        if (ESP_OK != err)
        {
            BB_ERROR("register %s failed due to %d", info.key, err);
            assert(false);
        }
        cap -= length;
        length = cap;
    }
    BB_INFO("init ok");
}

void finiBlackBoard(void)
{
    if (NULL == gDataBuffer.base) return;
    nvs_close(gDataBuffer.hNVS);
    free(gDataBuffer.base);
    gDataBuffer.wrPtr = gDataBuffer.base = NULL;
    vSemaphoreDelete(gDataBuffer.wrMutex);
}

void *getPublishData(const char *key)
{
    DataMapIter iter = gDataMap.find(key);
    if (gDataMap.end() == iter) return NULL;

    return (void *)iter->second.rdPtr;
}

esp_err_t registBlackBoardData(const char *key, size_t dataSize,
        bool persisted, uint32_t msWait)
{
    // prepare block
    BB_DataBlock db = {
        .persisted = persisted,
        .dataSize = dataSize,
        .rdPtr = gDataBuffer.wrPtr,
    };
    if (!hasEnoughSpace(dataSize))  return BB_LACK_SPACE;
    DataMapIter iter = gDataMap.find(key);
    if (gDataMap.end() != iter) return BB_KEY_DUPLICATED;

    xSemaphoreTake(gDataBuffer.wrMutex, pdMS_TO_TICKS(msWait));
    db.rdPtr = gDataBuffer.wrPtr;   // assgin again
    gDataBuffer.wrPtr += dataSize;
    gDataMap.insert(DataValueType(std::string(key), db));
    xSemaphoreGive(gDataBuffer.wrMutex);
    return ESP_OK;
}

esp_err_t flushPublishData(const char *key)
{
    DataMapIter iter = gDataMap.find(key);
    if (gDataMap.end() == iter) return BB_KEY_NOTEXIST;

    BB_DataBlock db = iter->second;
    if (!db.persisted) return BB_FLUSH_TEMP;

    // save data to NVS
    esp_err_t err = nvs_set_blob(gDataBuffer.hNVS, key, db.rdPtr, db.dataSize);
    return ESP_OK != err ? err : nvs_commit(gDataBuffer.hNVS);
}
