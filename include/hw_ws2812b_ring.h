#ifndef HW_WS2812B_RING_H
#define HW_WS2812B_RING_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/
#define HW_WS2812B_RING_LED_CNT         12

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint8_t     red;
    uint8_t     green;
    uint8_t     blue;
} hw_ws2812b_ring_led_t;

/**********************
 *      MACROS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void hw_ws2812b_ring_init( void );
void hw_ws2812b_ring_start( void );
void hw_ws2812b_ring_stop( void );

void hw_ws2812b_ring_set_led( int idx, hw_ws2812b_ring_led_t led );
void hw_ws2812b_ring_set_leds( int start_idx, hw_ws2812b_ring_led_t * led_tbl, int led_tbl_cnt );
void hw_ws2812b_ring_update();
void hw_ws2812b_ring_reset();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //HW_WS2812B_RING_H
