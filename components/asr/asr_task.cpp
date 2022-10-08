/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-07 19:13:08
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-07 19:13:08
 * @FilePath    : /LovelyLight/component/asr/asr_task.cpp
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "xtensa/core-macros.h"
#include "esp_partition.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "dl_lib_coefgetter_if.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"

#include "task_map.h"
#include "asr_task.h"
#include "led_task.h"

#define ASR_TAG "ASR"
#define ASR_DEBUG(fmt, ...)  ESP_LOGD(ASR_TAG, fmt, ##__VA_ARGS__)
#define ASR_INFO(fmt, ...)   ESP_LOGI(ASR_TAG, fmt, ##__VA_ARGS__)
#define ASR_WARN(fmt, ...)   ESP_LOGW(ASR_TAG, fmt, ##__VA_ARGS__)
#define ASR_ERROR(fmt, ...)  ESP_LOGE(ASR_TAG, fmt, ##__VA_ARGS__)

static const esp_mn_iface_t *g_multinet                    = &MULTINET_MODEL;
static model_iface_data_t *g_model_mn_data                 = NULL;
static const model_coeff_getter_t *g_model_mn_coeff_getter = &MULTINET_COEFF;

static const esp_wn_iface_t *g_wakenet                     = &WAKENET_MODEL;
static model_iface_data_t *g_model_wn_data                 = NULL;
static const model_coeff_getter_t *g_model_wn_coeff_getter = &WAKENET_COEFF;

static void i2s_init(void)
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER|I2S_MODE_RX),           // the mode must be set according to DSP configuration
        .sample_rate = 16000,                            // must be the same as DSP configuration
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,    // must be the same as DSP configuration
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,    // must be the same as DSP configuration
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
        .dma_buf_count = 3,
        .dma_buf_len = 300,
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = CONFIG_PIN_DMIC_SCK,  // IIS_SCLK
        .ws_io_num = CONFIG_PIN_DMIC_WS,    // IIS_LCLK
        .data_out_num = -1,                // IIS_DSIN
        .data_in_num = CONFIG_PIN_DMIC_SDO  // IIS_DOUT
    };
    i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_1, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_1);
}

AsrTask::AsrTask(const char *tName, uint32_t stackDepth, UBaseType_t priority,
        const BaseType_t coreId,
        size_t msgBuffSize) :
    ActiveTask {tName, stackDepth, priority, coreId, msgBuffSize}
{
    ASR_INFO("Task Created");
}

AsrTask::~AsrTask(void)
{
    ASR_INFO("Task Destroyed");
}

void AsrTask::run(void)
{
    ASR_INFO("Init MN Model...");
    // Initialize NN model
    g_model_wn_data = g_wakenet->create(g_model_wn_coeff_getter, DET_MODE_90);
    int wn_num = g_wakenet->get_word_num(g_model_wn_data);

    for (int i = 1; i <= wn_num; i++) {
        char *name = g_wakenet->get_word_name(g_model_wn_data, i);
        ASR_INFO("keywords: %s (index = %d)", name, i);
    }

    float wn_threshold = 0;
    int wn_sample_rate = g_wakenet->get_samp_rate(g_model_wn_data);
    int audio_wn_chunksize = g_wakenet->get_samp_chunksize(g_model_wn_data);
    ASR_INFO("keywords_num = %d, threshold = %f, sample_rate = %d, chunksize = %d, sizeof_uint16 = %d",
            wn_num, wn_threshold, wn_sample_rate, audio_wn_chunksize, sizeof(int16_t));

    g_model_mn_data = g_multinet->create(g_model_mn_coeff_getter, 4000);
    int audio_mn_chunksize = g_multinet->get_samp_chunksize(g_model_mn_data);
    int mn_num = g_multinet->get_samp_chunknum(g_model_mn_data);
    int mn_sample_rate = g_multinet->get_samp_rate(g_model_mn_data);
    ASR_INFO("keywords_num = %d , sample_rate = %d, chunksize = %d, sizeof_uint16 = %d",
            mn_num,  mn_sample_rate, audio_mn_chunksize, sizeof(int16_t));

    int size = audio_wn_chunksize;

    if (audio_mn_chunksize > audio_wn_chunksize) {
        size = audio_mn_chunksize;
    }

    int *buffer = (int *)malloc(size * 2 * sizeof(int));
    bool enable_wn = true;

    ASR_INFO("Init I2S...");
    i2s_init();

    size_t read_len = 0;

    int intv = 10;
    uint32_t last = millis();
    ASR_INFO("running...");

    while (true)
    {
        i2s_read(I2S_NUM_1, buffer, size * 2 * sizeof(int), &read_len, portMAX_DELAY);

        for (int x = 0; x < size * 2 / 4; x++)
        {
            int s1 = ((buffer[x * 4] + buffer[x * 4 + 1]) >> 13) & 0x0000FFFF;
            int s2 = ((buffer[x * 4 + 2] + buffer[x * 4 + 3]) << 3) & 0xFFFF0000;
            buffer[x] = s1 | s2;
        }

        uint32_t now = millis();
        if (now - last >= 9000)
        {
            // ASR_DEBUG("reset watch dog");
            last = now;
            esp_task_wdt_reset();  // feed watch dog
        }

        if (enable_wn)
        {

            // ASR_DEBUG("wait for wakeup");
            int r = g_wakenet->detect(g_model_wn_data, (int16_t *)buffer);
            if (r)
            {
                ASR_DEBUG("%s DETECTED", g_wakenet->get_word_name(g_model_wn_data, r));
                _generateCommand(-1); // wakeup
                enable_wn = false;
                last = now;
                intv = 10;  // reset intv
                continue;
            }
            else
            {
                // increase intv with step
                // intv = (intv >= 1000) ? 1000 : (intv+10);  // max delay 1s
                // delay(intv);
            }
        }
        else
        {
            int command_id = g_multinet->detect(g_model_mn_data, (int16_t *)buffer);
            if (command_id > -1)
            {
                ASR_DEBUG("MN test successfully, Commands ID: %d", command_id);
                _generateCommand(command_id);
                last = now;
            }
            else
            {
                // ASR_DEBUG("MN test successfully, empty");
                if (now - last >= 6000)
                {
                    // no command in 6s
                    ASR_DEBUG("stop multinet");
                    _generateCommand(-2); // sleep
                    enable_wn = true;
                }
                // delay(10);
            }
        }
    }
}

void AsrTask::_generateCommand(int cmd_id)
{
    uint8_t cmd = (uint8_t)LedCmdType::LED_CMD_BUTT;
    switch (cmd_id)
    {
        case -2:
            cmd = (uint8_t)LedCmdType::LED_SLEEP;
            break;
        case -1:
            cmd = (uint8_t)LedCmdType::LED_WAKEUP;
            break;
        case 0:
        case 1:
            cmd = (uint8_t)LedCmdType::LED_TURNON;
            break;
        case 2:
        case 3:
            cmd = (uint8_t)LedCmdType::LED_TURNOFF;
            break;
        case 4:
            cmd = (uint8_t)LedCmdType::LED_RANDOM_ON;
            break;
        case 5:
            cmd = (uint8_t)LedCmdType::LED_RANDOM_OFF;
            break;
        case 6:
            cmd = (uint8_t)LedCmdType::LED_BREATH_ON;
            break;
        case 7:
            cmd = (uint8_t)LedCmdType::LED_BREATH_OFF;
            break;
        case 8:
            cmd = (uint8_t)LedCmdType::LED_INCR_V;
            break;
        case 9:
            cmd = (uint8_t)LedCmdType::LED_DECR_V;
            break;
        case 10:
            cmd = (uint8_t)LedCmdType::LED_DECR_S;
            break;
        case 11:
            cmd = (uint8_t)LedCmdType::LED_INCR_S;
            break;
        default:
            break;
    }
    if ((uint8_t)LedCmdType::LED_CMD_BUTT != cmd)
    {
        sendMessageTo("ledsTask", &cmd, sizeof(uint8_t), 10);
        ASR_INFO("send cmd %d to Leds", cmd);
    }
}
