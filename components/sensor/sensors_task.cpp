/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-08 20:34:54
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-08 20:34:54
 * @FilePath    : /LovelyLight/components/sensor/sensors_task.cpp
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include "esp_err.h"
#include "esp_log.h"

#include "sensors_task.h"
#include "task_map.h"
#include "led_task.h"

#define SENSORS_TAG "Sensors"
#define SENSORS_DEBUG(fmt, ...)  ESP_LOGD(SENSORS_TAG, fmt, ##__VA_ARGS__)
#define SENSORS_INFO(fmt, ...)   ESP_LOGI(SENSORS_TAG, fmt, ##__VA_ARGS__)
#define SENSORS_WARN(fmt, ...)   ESP_LOGW(SENSORS_TAG, fmt, ##__VA_ARGS__)
#define SENSORS_ERROR(fmt, ...)  ESP_LOGE(SENSORS_TAG, fmt, ##__VA_ARGS__)

#define DEFAULT_VREF    1100        /**< Use adc2_vref_to_gpio() to obtain a better estimate */
#define NO_OF_SAMPLES   16          /**< Multisampling */

enum chrg_state_t {
    CHRG_STATE_UNKNOW   = 0x00,
    CHRG_STATE_CHARGING,
    CHRG_STATE_FULL,
};

xQueueHandle      g_event_queue = NULL;     // gpio event queue

int32_t                  g_vol_bat = 0;
esp_adc_cal_characteristics_t *g_adc_chars = NULL;
bool            g_vibration_en = true;     // vibration

static void adc_check_efuse()
{
    /**< Check TP is burned into eFuse */
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        SENSORS_INFO("ADC eFuse Two Point: Supported");
    } else {
        SENSORS_INFO("ADC eFuse Two Point: NOT supported");
    }

    /**< Check Vref is burned into eFuse */
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        SENSORS_INFO("ADC eFuse Vref: Supported");
    } else {
        SENSORS_INFO("ADC eFuse Vref: NOT supported");
    }
}

static void t_battery_handler(TimerHandle_t xTimer)
{
    // SENSORS_DEBUG("check battery");
    static uint32_t sample_index = 0;
    static uint16_t filter_buf[NO_OF_SAMPLES] = {0};

    uint32_t adc_reading = adc1_get_raw((adc1_channel_t)CONFIG_PIN_BAT_VLTG);
    filter_buf[sample_index++] = adc_reading;

    if (NO_OF_SAMPLES == sample_index)
    {
        sample_index = 0;
    }

    uint32_t sum = 0;

    for (int i = 0; i < NO_OF_SAMPLES; i++)
    {
        sum += filter_buf[i];
    }

    sum /= NO_OF_SAMPLES;
    /**< Convert adc_reading to voltage in mV */
    g_vol_bat = esp_adc_cal_raw_to_voltage(sum, g_adc_chars);
    /**< The resistance on the hardware has decreased twice */
    g_vol_bat *= 2;
    // SENSORS_DEBUG("check battery %d", g_vol_bat);
}

static void t_vibration_handler(void *arg)
{
    // SENSORS_DEBUG("check vibration");
    static int8_t last_level = 1;
    int8_t level;

    int32_t gpio_num = CONFIG_PIN_VIAB_INT;
    level = gpio_get_level((gpio_num_t)CONFIG_PIN_VIAB_INT);
    // SENSORS_DEBUG("check vibration %d", level);

    /**< Find a falling edge */
    if ((g_vibration_en) && (0 == level) && (1 == last_level)) {
        SENSORS_DEBUG("detect vibration");
        g_vibration_en = false;
        xQueueOverwrite(g_event_queue, &gpio_num);
    }

    last_level = level;
}

static void pressButton(void *arg)
{
    uint8_t cmd = (uint8_t)LedCmdType::LED_ROLL_V;
    sendMessageTo("ledsTask", &cmd, sizeof(uint8_t), 10);

    SENSORS_DEBUG("Send command Roll V to Leds");
}

SensorsTask::SensorsTask(const char *tName, uint32_t stackDepth, UBaseType_t priority,
        const BaseType_t coreId,
        size_t msgBuffSize) :
    ActiveTask {tName, stackDepth, priority, coreId, msgBuffSize},
    _btn_handle {NULL}
{
    SENSORS_INFO("Task Created");
}

SensorsTask::~SensorsTask()
{
    if (NULL != _btn_handle) iot_button_delete(_btn_handle);
    SENSORS_INFO("Task Destroyed");
}

void SensorsTask::run(void)
{
    _initBatterySensor();
    SENSORS_INFO("bettery sensor init");
    _initVibrationSensor();
    SENSORS_INFO("vibration sensor init");
    _initButton();
    SENSORS_INFO("button init");

    SENSORS_INFO("running ...");

    uint32_t count = 0;
    uint32_t last = millis();
    while (true)
    {
        uint32_t now = millis();
        if (now - last >= pdMS_TO_TICKS(5000))
        {
            SENSORS_DEBUG("reset watch dog");
            last = now;
            esp_task_wdt_reset();  // feed watch dog
        }

        uint32_t io_num;
        uint8_t level;

        if (xQueueReceive(g_event_queue, &io_num, pdMS_TO_TICKS(10)))
        {
            level = gpio_get_level((gpio_num_t)io_num);
            SENSORS_DEBUG("GPIO[%d] intr, val: %d\n", io_num, level);

            if (io_num == CONFIG_PIN_VIAB_INT)
            {
                uint8_t cmd = (uint8_t)LedCmdType::LED_SWITCH;
                sendMessageTo("ledsTask", &cmd, sizeof(uint8_t), 10);
                SENSORS_DEBUG("Send Command Switch to Leds");
                vTaskDelay(pdMS_TO_TICKS(250));
                g_vibration_en = true;
            }
        }

        if (++count >= 1000)
        {
            _getBatteryInfo();
            count = 0;
        }
        delay(100);
    }
}

void SensorsTask::_initBatterySensor()
{
    /**< Check if Two Point or Vref are burned into eFuse */
    adc_check_efuse();
    SENSORS_INFO("efuse checked");

    /**< Configure ADC */
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten((adc1_channel_t)CONFIG_PIN_BAT_VLTG, ADC_ATTEN_DB_11);
    SENSORS_INFO("config ADC to %d", CONFIG_PIN_BAT_VLTG);

    /**< Characterize ADC */
    g_adc_chars = (esp_adc_cal_characteristics_t*)calloc(1,
            sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1,
            ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, g_adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        SENSORS_INFO("ADC Characterized using Two Point Value");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        SENSORS_INFO("ADC Characterized using eFuse Vref");
    } else {
        SENSORS_INFO("ADC Characterized using Default Vref");
    }

    /**< Configure GPIO for read charge status */
    gpio_config_t io_conf =
    {
        .pin_bit_mask = (((uint64_t) 1) << CONFIG_PIN_BAT_CHRG) \
            | (((uint64_t) 1) << CONFIG_PIN_BAT_STBY),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    SENSORS_DEBUG("GPIO for battery configured");

    TimerHandle_t timer_bat = xTimerCreate("adc measure",
                       pdMS_TO_TICKS(2000),
                       pdTRUE,
                       NULL,
                       t_battery_handler);
    xTimerStart(timer_bat, 0);
    SENSORS_DEBUG("Timer for battery created");
}

void SensorsTask::_initVibrationSensor()
{
    gpio_config_t io_conf =
    {
        .pin_bit_mask = (((uint64_t) 1) << CONFIG_PIN_VIAB_INT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE,
    };
    gpio_config(&io_conf);


    /**< create a queue to handle gpio event from isr */
    g_event_queue = xQueueCreate(1, sizeof(uint32_t));
    assert (NULL != g_event_queue);

    /**< install gpio isr service */
    esp_timer_create_args_t periodic_timer_args = {
        .callback = &t_vibration_handler,
        .arg = NULL,
        /* name is optional, but may help identify the timer when debugging */
        .name = "periodic"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    /* Start the timers */
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 5000));

}

void SensorsTask::_initButton()
{
    _btn_handle = iot_button_create((gpio_num_t)CONFIG_PIN_BUTTON,
            (button_active_t)0);
    assert ("Failed to create button" && NULL != _btn_handle);

    iot_button_set_evt_cb(_btn_handle, BUTTON_CB_TAP, pressButton, NULL);
}

void SensorsTask::_getBatteryInfo()
{
    int32_t vol = g_vol_bat;
    uint8_t chrg_state = ((uint8_t)!gpio_get_level((gpio_num_t)CONFIG_PIN_BAT_CHRG) << 1) \
            | ((uint8_t)!gpio_get_level((gpio_num_t)CONFIG_PIN_BAT_STBY));

    vol = vol > 4200 ? 4200 : vol;
    vol = vol < 3600 ? 3600 : vol;
    vol -= 3600;
    int32_t level = vol / 6;

    chrg_state_t state = chrg_state_t::CHRG_STATE_UNKNOW;
    if (0x02 == chrg_state)
    {
        state = chrg_state_t::CHRG_STATE_CHARGING;
    }
    else if (0x01 == chrg_state)
    {
        state = chrg_state_t::CHRG_STATE_FULL;
    }
    SENSORS_INFO("battery %dmv level %d, state %d", g_vol_bat, level, state);
}