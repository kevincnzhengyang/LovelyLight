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

#include "xtensa/core-macros.h"
#include "esp_partition.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"

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

static i2s_chan_handle_t                rx_chan;        // I2S rx channel handler

static void i2s_init(void)
{
    i2s_chan_config_t rx_chan_cfg =
    {
        .id = I2S_NUM_1,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 3,
        .dma_frame_num = 300,
        .auto_clear = false,
    };

    ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &rx_chan));

    i2s_std_config_t rx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,    // some codecs may require mclk signal, this example doesn't need it
            .bclk = (gpio_num_t)CONFIG_PIN_DMIC_SCK,
            .ws   = (gpio_num_t)CONFIG_PIN_DMIC_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = (gpio_num_t)CONFIG_PIN_DMIC_SDO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &rx_std_cfg));

    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
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
    ASR_INFO("running...");

    uint32_t last = millis();
    uint32_t t_cmd = 0;
    while (true)
    {
        uint32_t now = millis();
        if (now - last >= pdMS_TO_TICKS(5000))
        {
            ASR_DEBUG("reset watch dog");
            last = now;
            esp_task_wdt_reset();  // feed watch dog
        }

        i2s_channel_read(rx_chan, buffer, size * 2 * sizeof(int), &read_len, portMAX_DELAY);

        for (int x = 0; x < size * 2 / 4; x++)
        {
            int s1 = ((buffer[x * 4] + buffer[x * 4 + 1]) >> 13) & 0x0000FFFF;
            int s2 = ((buffer[x * 4 + 2] + buffer[x * 4 + 3]) << 3) & 0xFFFF0000;
            buffer[x] = s1 | s2;
        }

        if (enable_wn)
        {
            int r = g_wakenet->detect(g_model_wn_data, (int16_t *)buffer);
            if (r)
            {
                ASR_DEBUG("%s DETECTED", g_wakenet->get_word_name(g_model_wn_data, r));
                _generateCommand(-1); // wakeup
                enable_wn = false;
                t_cmd = now;    // update last command timestap
            }
        }
        else
        {
            // ASR_DEBUG("wait for command");
            int command_id = g_multinet->detect(g_model_mn_data, (int16_t *)buffer);
            if (command_id > -1)
            {
                ASR_DEBUG("MN test successfully, Commands ID: %d", command_id);
                _generateCommand(command_id);
                t_cmd = now;    // update last command timestap
            }
            else
            {
                ASR_DEBUG("MN test successfully, empty %d", command_id);
                if (now - t_cmd > pdMS_TO_TICKS(10000))
                {
                    ASR_DEBUG("expired, sleep");
                    enable_wn = true;
                }
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
