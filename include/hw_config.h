#ifndef HW_CONFIG_H
#define HW_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "driver/pcnt.h"
#include "driver/i2c.h"
#include "driver/rmt.h"
#include "driver/uart.h"
#include "driver/gpio.h"

/*********************
 *      DEFINES
 *********************/

#define WS2812B_CHANNEL             RMT_CHANNEL_0
#define WS2812B_DO_PIN              2

#define BMS_TX                      GPIO_NUM_22
#define BMS_RX                      GPIO_NUM_23

#define I2C_SCL_PIN                 GPIO_NUM_12
#define I2C_SDA_PIN                 GPIO_NUM_13
#define I2C_NUM                     I2C_NUM_0
#define I2C_FREQ                    400000

#define RPM_PULSE_COUNTER_CHANNEL   PCNT_CHANNEL_0
#define RPM_PULSE_COUNTER_UNIT      PCNT_UNIT_0
#define RPM_PULSE_COUNTER_INPUT     GPIO_NUM_15
#define RPM_PULSE_COUNTER_CONTROL   -1

// PCA9531
#define SPEEDOMETER_PWM_PIN         0
#define BACKLIGHT_PWM_PIN           1
#define ENGINE_KILL_PIN             2
#define HEADLIGHT_0_PIN             3
#define HEADLIGHT_1_PIN             4
#define SWITCH_0_PIN                5
#define SWITCH_1_PIN                6

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //HW_CONFIG_H
