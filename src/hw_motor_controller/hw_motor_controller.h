#ifndef HW_MOTOR_CONTROLLER_H
#define HW_MOTOR_CONTROLLER_H

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

void hw_motor_controller_init( void );
void hw_motor_controller_start( void );
void hw_motor_controller_stop( void );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //HW_MOTOR_CONTROLLER_H
