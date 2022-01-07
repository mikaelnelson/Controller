/*********************
 *      INCLUDES
 *********************/
#include "hw_config.h"

/**********************
 *      DEFINES
 *********************/
#define LED_CNT             12

// Assuming an RMT Clock of 20 MHz (50nS period)
#define RMT_CLOCK_DIV       (4)
#define RMT_CLOCK_PERIOD_NS (50)

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint8_t     red;
    uint8_t     green;
    uint8_t     blue;
} led_t;

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

/**********************
 *      MACROS
 **********************/

/**********************
*      EXTERNS
**********************/

/**********************
 *    PROTOTYPES
 **********************/
void send_led( led_t led );
void send_led_color_component( uint8_t color );
void send_reset( void );

void hw_ws2812b_ring_init( void )
{
    // Configure RMT
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(WS2812B_DO_PIN, WS2812B_CHANNEL);
    config.clk_div = RMT_CLOCK_DIV;

    rmt_config( &config );
    rmt_driver_install( WS2812B_CHANNEL, 0, 0 );

    led_t led = {
        .red = 0xFF,
        .green = 0x00,
        .blue = 0x00
    };

    send_led(led);
    send_led(led);
    send_led(led);
    send_led(led);
    send_led(led);
    send_led(led);
    send_led(led);
    send_led(led);
    send_led(led);
    send_led(led);
    send_led(led);
    send_led(led);

}

void hw_ws2812b_ring_start( void )
{
}

void hw_ws2812b_ring_stop( void )
{
}

void send_led( led_t led )
{
    send_led_color_component( led.blue );
    send_led_color_component( led.green );
    send_led_color_component( led.red );
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