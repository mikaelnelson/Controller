/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <pubsub.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include "hw_config.h"
#include "hw_motor_controller.h"
#include "hw_motor_controller_protocol.h"

/**********************
 *      DEFINES
 *********************/
#define TAG                 "HW_MOTOR_CONTROLLER"

#define RX_BUF_SZ           ( 128 )

/**********************
 *      TYPEDEFS
 **********************/
typedef uint8_t cmd_t; enum {
    CMD_MONITOR_1           = 58,
    CMD_MONITOR_2           = 59,
    CMD_MONITOR_3           = 60
    };

/**********************
 *       CONSTS
 **********************/

/**********************
 *     GLOBALS
 **********************/
static TimerHandle_t            g_motor_controller_request_timer;
static TaskHandle_t             g_motor_controller_parse_task;

/**********************
 *      MACROS
 **********************/

/**********************
*      EXTERNS
**********************/

/**********************
 *    PROTOTYPES
 **********************/
_Noreturn static void motor_controller_parse_task(void * params );
void motor_controller_request( TimerHandle_t xTimer );
static void send_request( uint8_t cmd );
static uint8_t calc_checksum( const uint8_t * data, size_t data_sz );

void hw_motor_controller_init( void )
{
    g_motor_controller_parse_task = NULL;

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
            .baud_rate = MOTOR_CONTROLLER_BAUD_RATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
    };

    //Install UART driver, and get the queue.
    ESP_ERROR_CHECK(uart_driver_install( MOTOR_CONTROLLER_UART, 2 * RX_BUF_SZ, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(MOTOR_CONTROLLER_UART, &uart_config));

    //Set UART pins (using UART0 default pins ie no changes.)
    ESP_ERROR_CHECK(uart_set_pin(MOTOR_CONTROLLER_UART, MOTOR_CONTROLLER_TX, MOTOR_CONTROLLER_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void hw_motor_controller_start( void )
{
    //Create a task to handler UART if not already created
    if( NULL == g_motor_controller_parse_task) {
        xTaskCreate( motor_controller_parse_task, "motor_controller_parse_task", 4 * 1024, NULL, 12, &g_motor_controller_parse_task );
    }

    // Start Request Timer
    g_motor_controller_request_timer = xTimerCreate( "motor_controller_request_timer", pdMS_TO_TICKS(1000), pdTRUE, NULL, motor_controller_request );
    xTimerStart( g_motor_controller_request_timer, 0 );
}

void hw_motor_controller_stop( void )
{
    // Stop Request Timer
    xTimerStop( g_motor_controller_request_timer, portMAX_DELAY );
}

_Noreturn static void motor_controller_parse_task(void * params )
{
    uint8_t         data[RX_BUF_SZ];

    for(;;) {
        cmd_t           command = 0;
        size_t          data_sz = 0;
        const uint8_t * data_ptr = NULL;
        uint8_t         checksum = 0;
        bool            success;

        // Read Response
        data_sz = uart_read_bytes( MOTOR_CONTROLLER_UART, data, RX_BUF_SZ, portMAX_DELAY );
        success = ( data_sz >= 3 );

        // Parse Packet
        if( !success ) {
            continue;
        }
        else {
            // TODO: This is ugly, fix magic numbers

            // Byte 0 : Command
            command = data[0];

            // Byte 1 : Data Size
            data_sz = data[1];

            // Byte 2 : Payload Start
            data_ptr = &data[2];

            // Byte N - 1 : Checksum
            checksum = data[data_sz - 1];

            // Validate Packet
            success = ( calc_checksum( data_ptr, data_sz ) == checksum );
        }

        // Parse Packet
        if( success ) {
            switch( command ) {
                case CMD_MONITOR_1: {
                    hw_montor_controller_monitor_1_resp_t   resp;
                    if( hw_motor_controller_monitor_1_resp( &resp, data_ptr, data_sz ) ) {
                        PUB_INT("motor_controller.throttle_position", resp.throttle_pedal);
                        PUB_INT("motor_controller.brake_position", resp.brake_pedal);
                        PUB_DBL("motor_controller.current_voltage", (double)(resp.battery_voltage));
                        PUB_DBL("motor_controller.motor_temperature", (double)(resp.motor_temperature));
                        PUB_DBL("motor_controller.controller_temperature", (double)(resp.controller_temperature));
                    }
                } break;
            }
        }
    }

    g_motor_controller_parse_task = NULL;
    vTaskDelete(NULL);
}

void motor_controller_request( TimerHandle_t xTimer )
{
    send_request(CMD_MONITOR_1);
}


static void send_request( uint8_t cmd )
{
    uint8_t         data[3];

    // TODO: This is ugly, fix magic numbers
    
    // Byte 0 : Command
    data[0] = cmd;

    // Byte 1 : Data Size
    data[1] = 0;

    // Byte 2 : Checksum
    data[2] = calc_checksum( data, sizeof(data) - 1);

    // Send Data
    uart_write_bytes( MOTOR_CONTROLLER_UART, (const char *)data, sizeof(data) );
}


static uint8_t calc_checksum( const uint8_t * data, size_t data_sz )
{
    uint8_t checksum = 0;

    // Calculate Checksum
    for( int idx = 0; idx < data_sz; idx++ ) {
        checksum += data[idx];
    }

    return checksum;
}