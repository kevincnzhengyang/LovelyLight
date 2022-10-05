/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-05 18:58:37
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-05 19:13:47
 * @FilePath    : /LovelyLight/main/main.cpp
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-05 17:44:27
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-05 17:44:28
 * @FilePath    : /LovelyLight/app/test/main.cpp
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


extern "C" void app_main()
{
    int i = 0;
    while (1) {
        printf("[%d] Hello world!\n", i);
        i++;
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
