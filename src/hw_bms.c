
/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <pubsub.h>

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "hw_config.h"
#include "hw_bms.h"
#include "hw_bms_protocol.nail.h"

/*********************
 *      DEFINES
 *********************/
#define TAG                 "HW_BMS"

#define START_IDX           0
#define CMD_IDX             1
#define STATE_IDX           2
#define SIZE_IDX            3
#define DATA_IDX            4

#define HEADER_SZ           ( 4 )
#define FOOTER_SZ           ( 3 )
#define DATA_MAX_SZ         ( 256 )
#define RX_BUF_SZ           ( HEADER_SZ + FOOTER_SZ + DATA_MAX_SZ )

#define CMD_BUF_SZ          ( HEADER_SZ + FOOTER_SZ )

#define START_BYTE          0xDD
#define WRITE_BYTE          0x5A
#define READ_BYTE           0xA5
#define END_BYTE            0x77

enum {
    CMD_BASIC_INFO          = 0x03,
    CMD_CELL_VOLTAGES       = 0x04,
    CMD_HARDWARE_VER        = 0x05
    };

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *     GLOBALS
 **********************/
static TimerHandle_t            g_bms_request_timer;
static NailArena                g_rx_arena;

/**********************
 *     CONSTANTS
 **********************/

/**********************
 *    PROTOTYPES
 **********************/
static void send_request( uint8_t cmd );
static uint16_t calc_checksum( uint8_t * data );

_Noreturn static void bms_parse_task( void * params );
void bms_request( TimerHandle_t xTimer );
static bool parse_basic_status( uint8_t * data, uint32_t data_sz );

void hw_bms_init(void)
{
    ESP_LOGI(TAG, "BMS Intf Init");

    /* Configure parameters of an UART driver,
    * communication pins and install the driver */
    uart_config_t uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
    };

    //Install UART driver, and get the queue.
    ESP_ERROR_CHECK(uart_driver_install( UART_NUM_1, 2 * RX_BUF_SZ, 0, 0, NULL, 0) );
    ESP_ERROR_CHECK(uart_param_config( UART_NUM_1, &uart_config) );

    //Set UART pins (using UART0 default pins ie no changes.)
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, BMS_TX, BMS_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void hw_bms_start( void )
{
    //Create a task to handler UART event from ISR
    xTaskCreate( bms_parse_task, "bms_parse_task", 4 * 1024, NULL, 12, NULL );

    g_bms_request_timer = xTimerCreate( "bms_request", 1000 / portTICK_RATE_MS, pdTRUE, NULL, bms_request );
    xTimerStart( g_bms_request_timer, 0 );
}

void hw_bms_stop( void )
{

}

_Noreturn static void bms_parse_task( void * params )
{
    uint8_t     bms_data[RX_BUF_SZ];
    int         data_sz = 0;
    jmp_buf     err;

    // Initialize Parser
    NailArena_init(&g_rx_arena, 512, &err);

    for(;;) {

        // Read Response
        data_sz = uart_read_bytes( UART_NUM_1, bms_data, RX_BUF_SZ, 1000 / portTICK_RATE_MS );
        if( data_sz <= 0 ) {
            continue;
        }

        // TODO: Before we parse it, validate checksum

        switch( bms_data[CMD_IDX] ) {

            case CMD_BASIC_INFO: {
                basic_status_resp_t * resp;

                resp = parse_basic_status_resp_t( &g_rx_arena, &bms_data[DATA_IDX], bms_data[SIZE_IDX] );

                if( resp ) {
                    PUB_DBL("bms.total_voltage", (double)(resp->total_voltage * 0.01));
                    PUB_DBL("bms.current", (double)(resp->current * 0.01));
                    PUB_DBL("bms.residual_capacity", (double)(resp->residual_capacity * 0.01));
                    PUB_DBL("bms.nominal_capacity", (double)(resp->nominal_capacity * 0.01));
                    PUB_INT("bms.cycle_life", resp->cycle_life);
                    PUB_INT("bms.balance_status", (resp->balance_status_high << 16) | resp->balance_status);

                    PUB_BOOL("bms.protection_status.mosfet_software_lock", resp->protection_status.mosfet_software_lock);
                    PUB_BOOL("bms.protection_status.ic_frontend_error", resp->protection_status.ic_frontend_error);
                    PUB_BOOL("bms.protection_status.short_circuit", resp->protection_status.short_circuit);
                    PUB_BOOL("bms.protection_status.discharge_overcurrent", resp->protection_status.discharge_overcurrent);
                    PUB_BOOL("bms.protection_status.charge_overcurrent", resp->protection_status.charge_overcurrent);
                    PUB_BOOL("bms.protection_status.discharge_low_temp", resp->protection_status.discharge_low_temp);
                    PUB_BOOL("bms.protection_status.discharge_high_temp", resp->protection_status.discharge_high_temp);
                    PUB_BOOL("bms.protection_status.charge_low_temp", resp->protection_status.charge_low_temp);
                    PUB_BOOL("bms.protection_status.charge_high_temp", resp->protection_status.charge_high_temp);
                    PUB_BOOL("bms.protection_status.battery_under_voltage", resp->protection_status.battery_under_voltage);
                    PUB_BOOL("bms.protection_status.battery_over_voltage", resp->protection_status.battery_over_voltage);
                    PUB_BOOL("bms.protection_status.cell_under_voltage", resp->protection_status.cell_under_voltage);
                    PUB_BOOL("bms.protection_status.cell_over_voltage", resp->protection_status.cell_over_voltage);

                    PUB_INT("bms.version", resp->version);
                    PUB_INT("bms.rsoc", resp->rsoc);
                    PUB_BOOL("bms.fet_control_status.is_charging", resp->fet_control_status.is_charging);
                    PUB_BOOL("bms.fet_control_status.is_discharging", resp->fet_control_status.is_discharging);
                    PUB_INT("bms.cell_series_count", resp->cell_series_count);

                    for(int idx = 0; idx < resp->ntc_temperature.count; idx++) {
                        char tag[32];
                        snprintf(tag, sizeof(tag), "bms.ntc_temperature_%d", idx);

                        PUB_DBL(tag, (double)(resp->ntc_temperature.elem[idx] * 0.01));
                    }
                }
            } break;

            case CMD_CELL_VOLTAGES:
                break;

            case CMD_HARDWARE_VER:
                break;

            default:
                break;
        }
    }

    NailArena_release( &g_rx_arena );

    vTaskDelete(NULL);
}

void bms_request( TimerHandle_t xTimer )
{
    send_request(CMD_BASIC_INFO);
}

static void send_request( uint8_t cmd )
{
    uint8_t cmd_data[CMD_BUF_SZ];
    uint16_t checksum;

    //{ 0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77 };
    cmd_data[0] = START_BYTE;
    cmd_data[1] = READ_BYTE;
    cmd_data[2] = cmd;
    cmd_data[3] = 0;

    checksum = calc_checksum( cmd_data );
    cmd_data[4] = ( checksum >> 8 ) & 0x00FF;
    cmd_data[5] = ( checksum >> 0 ) & 0x00FF;

    cmd_data[6] = END_BYTE;

    // Send Request Command
    uart_write_bytes( UART_NUM_1, (const char *)cmd_data, sizeof(cmd_data) );
}

static uint16_t calc_checksum( uint8_t * data )
{
    uint16_t checksum = 0xFFFF;
    uint8_t length = data[SIZE_IDX];

    // Subtract Command/Status
    checksum -= data[2];

    // Subtract Length
    checksum -= length;

    // Subtract Data
    for( int i = 0; i < length; i++ ) {
        checksum -= data[4 + i];
    }

    checksum += 1;

    return checksum;
}

