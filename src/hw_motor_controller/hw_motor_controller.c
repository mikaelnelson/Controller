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
#include "protocol.nail.h"

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
static NailArena                g_rx_arena;
static NailArena                g_tx_arena;

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

void hw_motor_controller_init( void )
{
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
    jmp_buf     err;

    // Initialize Nail Arenas
    NailArena_init(&g_tx_arena, 128, &err);
    NailArena_init(&g_rx_arena, 128, &err);

    //Create a task to handler UART event from ISR
    xTaskCreate( motor_controller_parse_task, "motor_controller_parse_task", 4 * 1024, NULL, 12, NULL );

    g_motor_controller_request_timer = xTimerCreate( "motor_controller_request", pdMS_TO_TICKS(1000), pdTRUE, NULL, motor_controller_request );
    xTimerStart( g_motor_controller_request_timer, 0 );
}

void hw_motor_controller_stop( void )
{
    // TODO: Stop Thread

    // Release Nail Arenas
    NailArena_release( &g_tx_arena );
    NailArena_release( &g_rx_arena );
}

_Noreturn static void motor_controller_parse_task(void * params )
{
    bool                        success;
    uint8_t                     data[RX_BUF_SZ];
    int                         data_sz = 0;
    motor_controller_resp_t   * resp;

    for(;;) {
        // Read Response
        data_sz = uart_read_bytes( MOTOR_CONTROLLER_UART, data, RX_BUF_SZ, 1000 / portTICK_RATE_MS );
        success = ( data_sz > 0 );

        if( !success ) {
            continue;
        }

        // Parse Command Response
        if( success ) {
            resp = parse_motor_controller_resp_t( &g_rx_arena, data, data_sz );
            success = (NULL != resp);
        }

        // Handle Command
        if( success ) {
            switch( resp->command ) {
                case CMD_MONITOR_1: {
                    motor_controller_monitor_1_resp_t * monitor_1_resp;

                    monitor_1_resp = parse_motor_controller_monitor_1_resp_t( &g_rx_arena, resp->data.elem, resp->data.count );
                    success = ( NULL != monitor_1_resp );

                    if( success ) {
                        PUB_INT("motor_controller.throttle_position", monitor_1_resp->throttle_pedal);
                        PUB_INT("motor_controller.brake_position", monitor_1_resp->brake_pedal);
                        PUB_DBL("motor_controller.current_voltage", (double)(monitor_1_resp->battery_voltage));
                        PUB_DBL("motor_controller.motor_temperature", (double)(monitor_1_resp->motor_temperature));
                        PUB_DBL("motor_controller.controller_temperature", (double)(monitor_1_resp->controller_temperature));

                    }
                } break;

                default:
                    break;
            }
        }
    }

    vTaskDelete(NULL);
}

void motor_controller_request( TimerHandle_t xTimer )
{
    send_request(CMD_MONITOR_1);
}


static void send_request( uint8_t cmd )
{
    bool                            success;
    NailOutStream                   out_stream;
    motor_controller_cmd_req_t      cmd_req;
    const uint8_t                 * data_ptr = NULL;
    size_t                          data_sz;

    success = ( 0 == NailOutStream_init( &out_stream, 32 ) );

    // Setup Command
    if( success ) {
        cmd_req.command = cmd;

        // Generate Command
        success = ( 0 == gen_motor_controller_cmd_req_t( &g_tx_arena, &out_stream, &cmd_req ) );
    }

    // Get Data
    if( success ) {
        data_ptr = NailOutStream_buffer( &out_stream, &data_sz );

        success = ( NULL != data_ptr );
    }

    // Send Request Command
    if( success ) {
        uart_write_bytes( MOTOR_CONTROLLER_UART, (const char *)data_ptr, data_sz );
    }

    NailOutStream_release( &out_stream );
}
