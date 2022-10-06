/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-05 23:51:28
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-05 23:51:29
 * @FilePath    : /LovelyLight/component/board/include/board_data.h
 * @Description : data structure definitions
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifndef delay
#define delay(ms)       vTaskDelay(pdMS_TO_TICKS(ms))
#endif

#ifndef millis
#define millis()        xTaskGetTickCount()
#endif

/**
 *  error code
 */
#define APP_ERR_BASE                0x100000  /*!< Starting number of APP error codes */
#define BB_LACK_SPACE               (APP_ERR_BASE+0x01)
#define BB_KEY_DUPLICATED           (APP_ERR_BASE+0x02)
#define BB_KEY_NOTEXIST             (APP_ERR_BASE+0x03)
#define BB_FLUSH_TEMP               (APP_ERR_BASE+0x04)

/**
 * Macros from Arduino
 */

// sign function
#define _sign(a) ( ( (a) < 0 )  ?  -1   : ( (a) > 0 ) )
#define _round(x) ((x)>=0?(long)((x)+0.5f):(long)((x)-0.5f))
#define _constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define _sqrt(a) (_sqrtApprox(a))
#define _isset(a) ( (a) != (NOT_SET) )
#define _UNUSED(v) (void) (v)

// utility defines
#define _2_SQRT3 1.15470053838f
#define _SQRT3 1.73205080757f
#define _1_SQRT3 0.57735026919f
#define _SQRT3_2 0.86602540378f
#define _SQRT2 1.41421356237f
#define _120_D2R 2.09439510239f
#define _PI 3.14159265359f
#define _PI_2 1.57079632679f
#define _PI_3 1.0471975512f
#define _2PI 6.28318530718f
#define _3PI_2 4.71238898038f
#define _PI_6 0.52359877559f

#define NOT_SET -12345.0
#define _HIGH_IMPEDANCE 0
#define _HIGH_Z _HIGH_IMPEDANCE
#define _ACTIVE 1

// from Arduino
#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)


#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif /* MAX */

#ifndef CLAMP
#define CLAMP(v, l, h)  ((v) < (l) ? (l) : ((v) > (h) ? (h) : (v)))
#endif
