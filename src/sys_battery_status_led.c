/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>
#include <pubsub.h>
#include <math.h>

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

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

/**********************
 *     GLOBALS
 **********************/
static state_t8    g_state = STATE_CHARGING;
static uint8_t     g_battery_capacity = 100;

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
void battery_status_capacity_update( void );
void battery_status_charging_update( void );
void battery_status_error_update( void );

void sys_battery_status_led_init( void )
{
}

void sys_battery_status_led_start( void )
{
    xTaskCreate( battery_status_led_task, "battery_status_led_task", 4 * 1024, NULL, 12, NULL );
}

void sys_battery_status_led_stop( void )
{
}


_Noreturn static void battery_status_led_task( void *params )
{
    while(true) {
        // Handle State Machine
        switch(g_state) {
            default:
            case STATE_IDLE:
                break;

            case STATE_OFF:
                hw_ws2812b_ring_reset();
                g_state = STATE_IDLE;
                break;

            case STATE_CAPACITY:
                battery_status_capacity_update();
                vTaskDelay( 1000/portTICK_PERIOD_MS );
                g_battery_capacity--;
                if(g_battery_capacity == BATTERY_CAPACITY_MIN) {
                    g_battery_capacity = BATTERY_CAPACITY_MAX;
                }
                break;

            case STATE_CHARGING:
                vTaskDelay( 10/portTICK_PERIOD_MS );
                battery_status_charging_update();
                break;

            case STATE_ERROR:
                battery_status_error_update();
                break;
        }
    }
}


void battery_status_capacity_update( void )
{
    int led_max = 0.5f + (float)( ( g_battery_capacity * HW_WS2812B_RING_LED_CNT ) / ( BATTERY_CAPACITY_MAX - BATTERY_CAPACITY_MIN ) );

    for( int idx = 0; ( idx < led_max ) && ( idx < HW_WS2812B_RING_LED_CNT ); idx++ ) {
        hw_ws2812b_ring_set_led( idx, gc_led_green );
    }

    for( int idx = led_max; idx < HW_WS2812B_RING_LED_CNT; idx++ ) {
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

    int led_max = 0.5f + (float)( ( g_battery_capacity * HW_WS2812B_RING_LED_CNT ) / ( BATTERY_CAPACITY_MAX - BATTERY_CAPACITY_MIN ) );

    // Fill 50% Green LED
    for( int idx = 0; ( idx < ( led_max - 1 ) ) && ( idx < HW_WS2812B_RING_LED_CNT ); idx++ ) {
        hw_ws2812b_ring_set_led( idx, charge_led );
    }

    // Fill Fade Green LED
    if( (led_max - 1) < HW_WS2812B_RING_LED_CNT ) {

        uint32_t time = pdTICKS_TO_MS( xTaskGetTickCount() );



        float time_chunk = ( ( time % 2000 ) / 2000.0f);
        printf("tm_chunk: %f\n", time_chunk);
        float sine = sin(M_PI * time_chunk );
        printf("sine: %f\n", sine);

        charge_led.green = 0x20 + ( 0xDF * sine );
        hw_ws2812b_ring_set_led( (led_max - 1), charge_led );

        printf("green: %d\n", charge_led.green);
    }

    // Clear remaining LEDs
    for( int idx = led_max; idx < HW_WS2812B_RING_LED_CNT; idx++ ) {
        hw_ws2812b_ring_set_led( idx, gc_led_black );
    }

    hw_ws2812b_ring_update();
}

void battery_status_error_update( void )
{

}