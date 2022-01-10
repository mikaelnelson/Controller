#ifndef SYS_BATTERY_STATUS_LED_H
#define SYS_BATTERY_STATUS_LED_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void sys_battery_status_led_init( void );

void sys_battery_status_led_start( void );

void sys_battery_status_led_stop( void );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //SYS_BATTERY_STATUS_LED_H
