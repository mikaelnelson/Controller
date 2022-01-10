/*********************
 *      INCLUDES
 *********************/
#include <pubsub.h>
#include <string.h>

#include "hw_ws2812b_ring.h"
#include "hw_config.h"

/**********************
 *      DEFINES
 *********************/
// Assuming an RMT Clock of 20 MHz (50nS period)
#define RMT_CLOCK_DIV       (4)
#define RMT_CLOCK_PERIOD_NS (50)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *       CONSTS
 **********************/

static const rmt_item32_t   gc_bit_tbl[2] = {
    /*  duration                    level   duration                    level   */
    {{{ 350/RMT_CLOCK_PERIOD_NS,    1,      1000/RMT_CLOCK_PERIOD_NS,   0       }}},  /* Low (0) */
    {{{ 1000/RMT_CLOCK_PERIOD_NS,   1,      350/RMT_CLOCK_PERIOD_NS,    0       }}},  /* High (1) */
};

static const rmt_item32_t   gc_reset =
    /*  duration                    level   duration                    level   */
    {{{ 150000/RMT_CLOCK_PERIOD_NS, 0,      150000/RMT_CLOCK_PERIOD_NS, 0       }}};

/**********************
 *     GLOBALS
 **********************/
static hw_ws2812b_ring_led_t    g_led_tbl[HW_WS2812B_RING_LED_CNT] = { 0 };
static SemaphoreHandle_t        g_led_tbl_lock;

/**********************
 *      MACROS
 **********************/

/**********************
*      EXTERNS
**********************/

/**********************
 *    PROTOTYPES
 **********************/
void send_led( hw_ws2812b_ring_led_t led );
void send_led_color_component( uint8_t color );
void send_reset( void );

void hw_ws2812b_ring_init( void )
{
    // Configure RMT
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(WS2812B_DO_PIN, WS2812B_CHANNEL);
    config.clk_div = RMT_CLOCK_DIV;

    rmt_config( &config );
    rmt_driver_install( WS2812B_CHANNEL, 0, 0 );

    // Setup LED Table
    g_led_tbl_lock = xSemaphoreCreateMutex();

    for( int idx = 0; idx < HW_WS2812B_RING_LED_CNT; idx++ ) {
        g_led_tbl[idx] = (hw_ws2812b_ring_led_t){
            .red = 0x00,
            .green = 0x00,
            .blue = 0x00
        };
    }

    // Reset Ring
    hw_ws2812b_ring_reset();
}

void hw_ws2812b_ring_start( void )
{
}

void hw_ws2812b_ring_stop( void )
{
    vSemaphoreDelete( g_led_tbl_lock );
}

void hw_ws2812b_ring_set_led( int idx, hw_ws2812b_ring_led_t led )
{
    xSemaphoreTake( g_led_tbl_lock, portMAX_DELAY );

    if( idx < HW_WS2812B_RING_LED_CNT ) {
        g_led_tbl[idx] = led;
    }

    xSemaphoreGive( g_led_tbl_lock );
}

void hw_ws2812b_ring_set_leds( int start_idx, hw_ws2812b_ring_led_t * led_tbl, int led_tbl_cnt )
{
    xSemaphoreTake( g_led_tbl_lock, portMAX_DELAY );

    for( int idx = 0; start_idx < HW_WS2812B_RING_LED_CNT; start_idx++, idx++) {
        g_led_tbl[start_idx] = led_tbl[idx];
    }

    xSemaphoreGive( g_led_tbl_lock );
}

void hw_ws2812b_ring_update()
{
    xSemaphoreTake( g_led_tbl_lock, portMAX_DELAY );

    // Reset LEDs
    send_reset();

    // Send LEDs
    for( int idx = 0; idx < HW_WS2812B_RING_LED_CNT; idx++) {
        send_led( g_led_tbl[idx] );
    }

    xSemaphoreGive( g_led_tbl_lock );
}

void hw_ws2812b_ring_reset()
{
    xSemaphoreTake( g_led_tbl_lock, portMAX_DELAY );

    // Reset LED Table
    for( int idx = 0; idx < HW_WS2812B_RING_LED_CNT; idx++ ) {
        g_led_tbl[idx] = (hw_ws2812b_ring_led_t){
                .red = 0x00,
                .green = 0x00,
                .blue = 0x00
        };
    }

    // Reset LEDs
    send_reset();

    xSemaphoreGive( g_led_tbl_lock );
}

void send_led( hw_ws2812b_ring_led_t led )
{
    send_led_color_component( led.green );
    send_led_color_component( led.red );
    send_led_color_component( led.blue );
}

void send_led_color_component( uint8_t color )
{
    int idx;

    for(idx = 0; idx < 8; idx++) {
        int bit = ( color >> ( 7 - idx ) ) & 0x01;
        rmt_write_items(WS2812B_CHANNEL, &gc_bit_tbl[bit], 1, true );
    }
}


void send_reset( void )
{
    rmt_write_items(WS2812B_CHANNEL, &gc_reset, 1, true );
}