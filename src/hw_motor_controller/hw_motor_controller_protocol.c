/*********************
 *      INCLUDES
 *********************/
#include "hw_motor_controller_protocol.h"

/**********************
 *      DEFINES
 *********************/

#define MONITOR_1_RESP_THROTTLE_PEDAL_OFST          0
#define MONITOR_1_RESP_BRAKE_PEDAL_OFST             1
#define MONITOR_1_RESP_BRAKE_SWITCH_OFST            2
#define MONITOR_1_RESP_FOOT_SWITCH_OFST             3
#define MONITOR_1_RESP_REVERSE_SWITCH_OFST          4
#define MONITOR_1_RESP_REVERSED_OFST                5
#define MONITOR_1_RESP_HALL_A_OFST                  6
#define MONITOR_1_RESP_HALL_B_OFST                  7
#define MONITOR_1_RESP_HALL_C_OFST                  8
#define MONITOR_1_RESP_BATTERY_VOLTAGE_OFST         9
#define MONITOR_1_RESP_MOTOR_TEMPERATURE_OFST       10
#define MONITOR_1_RESP_CONTROLLER_TEMPERATURE_OFST  11
#define MONITOR_1_RESP_SETTING_DIRECTION_OFST       12
#define MONITOR_1_RESP_ACTUAL_DIRECTION_OFST        13
#define MONITOR_1_RESP_BRAKE_SWITCH_2_OFST          14
#define MONITOR_1_RESP_LOW_SPEED_SWITCH_OFST        15

#define MONITOR_1_RESP_SIZE                         16

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *       CONSTS
 **********************/

/**********************
 *     GLOBALS
 **********************/

/**********************
 *      MACROS
 **********************/
#define BOOL( _val )    ( ( (_val) == 0 ) ? false : true )

/**********************
*      EXTERNS
**********************/

/**********************
 *    PROTOTYPES
 **********************/

bool hw_motor_controller_monitor_1_resp( hw_montor_controller_monitor_1_resp_t * resp, const uint8_t * data, size_t data_sz )
{
    bool        success = false;

    // Validate Resp & Data & Size
    success = ( ( NULL != resp ) &&
                ( NULL != data ) &&
                ( MONITOR_1_RESP_SIZE == data_sz ));

    // Parse Packet & Validate
    if( success ) {
        resp->throttle_pedal            = data[MONITOR_1_RESP_THROTTLE_PEDAL_OFST];
        resp->brake_pedal               = data[MONITOR_1_RESP_BRAKE_PEDAL_OFST];
        resp->brake_switch              = BOOL(data[MONITOR_1_RESP_BRAKE_SWITCH_OFST]);
        resp->foot_switch               = BOOL(data[MONITOR_1_RESP_FOOT_SWITCH_OFST]);
        resp->reverse_switch            = BOOL(data[MONITOR_1_RESP_REVERSE_SWITCH_OFST]);
        resp->reversed                  = BOOL(data[MONITOR_1_RESP_REVERSED_OFST]);
        resp->hall_a                    = BOOL(data[MONITOR_1_RESP_HALL_A_OFST]);
        resp->hall_b                    = BOOL(data[MONITOR_1_RESP_HALL_B_OFST]);
        resp->hall_c                    = BOOL(data[MONITOR_1_RESP_HALL_C_OFST]);
        resp->battery_voltage           = data[MONITOR_1_RESP_BATTERY_VOLTAGE_OFST];
        resp->motor_temperature         = data[MONITOR_1_RESP_MOTOR_TEMPERATURE_OFST];
        resp->controller_temperature    = data[MONITOR_1_RESP_CONTROLLER_TEMPERATURE_OFST];
        resp->setting_direction         = BOOL(data[MONITOR_1_RESP_SETTING_DIRECTION_OFST]);
        resp->actual_direction          = BOOL(data[MONITOR_1_RESP_ACTUAL_DIRECTION_OFST]);
        resp->brake_switch_2            = BOOL(data[MONITOR_1_RESP_BRAKE_SWITCH_2_OFST]);
        resp->low_speed_switch          = BOOL(data[MONITOR_1_RESP_LOW_SPEED_SWITCH_OFST]);
    }

    return success;
}