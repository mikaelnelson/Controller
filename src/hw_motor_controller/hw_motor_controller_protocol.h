#ifndef HW_MOTOR_CONTROLLER_PROTOCOL_H
#define HW_MOTOR_CONTROLLER_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint8_t     throttle_pedal;
    uint8_t     brake_pedal;
    bool        brake_switch;
    bool        foot_switch;
    bool        reverse_switch;
    bool        reversed;
    bool        hall_a;
    bool        hall_b;
    bool        hall_c;
    uint8_t     battery_voltage;
    uint8_t     motor_temperature;
    uint8_t     controller_temperature;
    bool        setting_direction;
    bool        actual_direction;
    bool        brake_switch_2;
    bool        low_speed_switch;
    } hw_montor_controller_monitor_1_resp_t;


/**********************
 *      MACROS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

bool hw_motor_controller_monitor_1_resp( hw_montor_controller_monitor_1_resp_t * resp, const uint8_t * data, size_t data_sz );


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //HW_MOTOR_CONTROLLER_PROTOCOL_H
