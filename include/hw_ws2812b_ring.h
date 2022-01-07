#ifndef HW_WS2812B_RING_H
#define HW_WS2812B_RING_H

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

void hw_ws2812b_ring_init( void );

void hw_ws2812b_ring_start( void );

void hw_ws2812b_ring_stop( void );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //HW_WS2812B_RING_H
