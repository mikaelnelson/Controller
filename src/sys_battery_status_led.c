/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include <pubsub.h>

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include "sys_battery_status_led.h"

#include "hw_ws2812b_ring.h"

/**********************
 *      DEFINES
 *********************/
#define BATTERY_CAPACITY_MIN    0
#define BATTERY_CAPACITY_MAX    100

/**********************
 *      TYPEDEFS
 **********************/

typedef uint8_t state_t8; enum {
    STATE_IDLE,
    STATE_OFF,
    STATE_CAPACITY,
    STATE_CHARGING,
    STATE_ERROR
};

typedef struct {
    uint8_t     low;
    uint8_t     high;
} lookup_entry_t;

/**********************
 *       CONSTS
 **********************/
static const hw_ws2812b_ring_led_t  gc_led_red      = { .red = 0xFF, .green = 0x00, .blue = 0x00 };
static const hw_ws2812b_ring_led_t  gc_led_green    = { .red = 0x00, .green = 0xFF, .blue = 0x00 };
static const hw_ws2812b_ring_led_t  gc_led_blue     = { .red = 0x00, .green = 0x00, .blue = 0xFF };
static const hw_ws2812b_ring_led_t  gc_led_black    = { .red = 0x00, .green = 0x00, .blue = 0x00 };
static const hw_ws2812b_ring_led_t  gc_led_white    = { .red = 0xFF, .green = 0xFF, .blue = 0xFF };

static const hw_ws2812b_ring_led_t  gc_led_green_50    = { .red = 0x00, .green = 0x80, .blue = 0x00 };
static const hw_ws2812b_ring_led_t  gc_led_green_25    = { .red = 0x00, .green = 0x40, .blue = 0x00 };

static const lookup_entry_t gc_led_lookup_tbl[HW_WS2812B_RING_LED_CNT] =
{
    { 0,  8 },      /* LED 1 */
    { 8,  16 },     /* LED 2 */
    { 16, 24 },     /* LED 3 */
    { 24, 32 },     /* LED 4 */
    { 32, 40 },     /* LED 5 */
    { 40, 48 },     /* LED 6 */
    { 48, 56 },     /* LED 7 */
    { 56, 64 },     /* LED 8 */
    { 64, 72 },     /* LED 9 */
    { 72, 80 },     /* LED 10 */
    { 80, 88 },     /* LED 11 */
    { 88, 100 },    /* LED 12 */
};

/**********************
 *     GLOBALS
 **********************/
static TimerHandle_t            g_battery_status_led_timer;
static state_t8                 g_state = STATE_IDLE;
static SemaphoreHandle_t        g_state_mutex;
static int                      g_battery_capacity = BATTERY_CAPACITY_MAX;

/**********************
 *      MACROS
 **********************/

/**********************
*      EXTERNS
**********************/

/**********************
 *    PROTOTYPES
 **********************/
_Noreturn static void battery_status_led_task( void *params );
void battery_status_led_timer( TimerHandle_t xTimer );

void battery_status_capacity_update( void );
void battery_status_charging_update( void );
void battery_status_error_update( void );

void sys_battery_status_led_init( void )
{
    g_state = STATE_IDLE;
    g_state_mutex = xSemaphoreCreateMutex();
}

void sys_battery_status_led_start( void )
{
    xTaskCreate( battery_status_led_task, "battery_status_led_task", 4 * 1024, NULL, 12, NULL );

    g_battery_status_led_timer = xTimerCreate( "battery_status_led_timer", 33 / portTICK_RATE_MS, pdTRUE, NULL, battery_status_led_timer );
    xTimerStart( g_battery_status_led_timer, 0 );
}

void sys_battery_status_led_stop( void )
{
}


_Noreturn static void battery_status_led_task( void *params )
{
    bool                is_charging = false;
    double              total_capacity = 0;
    double              current_capacity = 0;
    ps_msg_t          * msg = NULL;
    ps_subscriber_t   * s;

    // Create Subscriber
    s = ps_new_subscriber(5, NULL);

    ps_subscribe( s, "bms.residual_capacity" );
    ps_subscribe( s, "bms.nominal_capacity" );
    ps_subscribe( s, "bms.current" );

    while(true) {
        // Handle Messages
        msg = ps_get(s, -1);

        if( msg ) {
            if(0 == strcmp("bms.residual_capacity", msg->topic)) {
                current_capacity = msg->dbl_val;
            }
            else if(0 == strcmp("bms.nominal_capacity", msg->topic)) {
                total_capacity = msg->dbl_val;
            }
            else if(0 == strcmp("bms.current", msg->topic)) {
                // positive = discharging, negative = charging
                is_charging = ( msg->dbl_val >= 0.0f );
            }

            // Calculate Battery Capacity
            if( total_capacity > 0 ) {
                g_battery_capacity = ( 100 * current_capacity ) / ( total_capacity );

                if( g_battery_capacity > BATTERY_CAPACITY_MAX ) {
                    g_battery_capacity = BATTERY_CAPACITY_MAX;
                }

                if( g_battery_capacity < BATTERY_CAPACITY_MIN ) {
                    g_battery_capacity = BATTERY_CAPACITY_MIN;
                }
            }

            ps_unref_msg(msg);
        }

        // Determine State
        xSemaphoreTake(g_state_mutex, portMAX_DELAY);

        if( 0 == total_capacity ) {
            // If Total Capacity is 0, Error
            g_state = STATE_ERROR;
        }
        else if( is_charging ) {
            // In A Charging State
            g_state = STATE_CHARGING;
        }
        else {
            // In a Discharging State
            g_state = STATE_CAPACITY;
        }

        xSemaphoreGive( g_state_mutex );
    }

    ps_free_subscriber(s);
}

void battery_status_led_timer( TimerHandle_t xTimer )
{
    state_t8        state;

    // Get State
    xSemaphoreTake(g_state_mutex, portMAX_DELAY);
    state = g_state;
    xSemaphoreGive(g_state_mutex);

    // Handle State Machine
    switch( state ) {
        default:
        case STATE_IDLE:
            break;

        case STATE_OFF:
            hw_ws2812b_ring_reset();
            state = STATE_IDLE;
            break;

        case STATE_CAPACITY:
            battery_status_capacity_update();
            break;

        case STATE_CHARGING:
            battery_status_charging_update();
            break;

        case STATE_ERROR:
            battery_status_error_update();
            break;
    }

    // Update State
    xSemaphoreTake(g_state_mutex, portMAX_DELAY);
    g_state = state;
    xSemaphoreGive(g_state_mutex);
}


void battery_status_capacity_update( void )
{
    int led_max = 0;
    for( led_max = 0; led_max < HW_WS2812B_RING_LED_CNT; led_max++ ) {
        if( ( g_battery_capacity >= gc_led_lookup_tbl[led_max].low ) && ( g_battery_capacity < gc_led_lookup_tbl[led_max].high ) ) {
            break;
        }
    }

    for( int idx = 0; ( idx <= led_max ) && ( idx < HW_WS2812B_RING_LED_CNT ); idx++ ) {
        hw_ws2812b_ring_set_led( idx, gc_led_green );
    }

    for( int idx = ( led_max + 1 ); idx < HW_WS2812B_RING_LED_CNT; idx++ ) {
        hw_ws2812b_ring_set_led( idx, gc_led_black );
    }

    hw_ws2812b_ring_update();
}

void battery_status_charging_update( void )
{
    hw_ws2812b_ring_led_t  charge_led;

    charge_led.red = 0x00;
    charge_led.blue = 0x00;
    charge_led.green = 5;

    int led_max = 0;
    for( led_max = 0; led_max < HW_WS2812B_RING_LED_CNT; led_max++ ) {
        if( ( g_battery_capacity >= gc_led_lookup_tbl[led_max].low ) && ( g_battery_capacity < gc_led_lookup_tbl[led_max].high ) ) {
            break;
        }
    }

    // Fill 50% Green LED
    for( int idx = 0; idx < led_max && ( idx < HW_WS2812B_RING_LED_CNT ); idx++ ) {
        hw_ws2812b_ring_set_led( idx, charge_led );
    }

    // Fill Fade Green LED
    if( led_max < HW_WS2812B_RING_LED_CNT ) {
        uint32_t time = pdTICKS_TO_MS( xTaskGetTickCount() );

        float time_chunk = ( ( time % 2000 ) / 2000.0f);
        float sine = sin(M_PI * time_chunk );

        charge_led.green = 0x20 + ( 0xDF * sine );

        hw_ws2812b_ring_set_led( led_max, charge_led );
    }

    // Clear remaining LEDs
    for( int idx = led_max + 1; idx < HW_WS2812B_RING_LED_CNT; idx++ ) {
        hw_ws2812b_ring_set_led( idx, gc_led_black );
    }

    hw_ws2812b_ring_update();
}

void battery_status_error_update( void )
{
    hw_ws2812b_ring_led_t  color;

    color.red = 5;
    color.blue = 0x00;
    color.green = 0x00;

    uint32_t time = pdTICKS_TO_MS( xTaskGetTickCount() );

    float time_chunk = ( ( time % 2000 ) / 2000.0f);
    float sine = sin(M_PI * time_chunk );

    color.red = 0x20 + ( 0xDF * sine );

    for( int idx = 0; idx < HW_WS2812B_RING_LED_CNT; idx++ ) {
        hw_ws2812b_ring_set_led( idx, color );
    }

    hw_ws2812b_ring_update();
}