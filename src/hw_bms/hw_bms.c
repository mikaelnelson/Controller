
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
#include "hw_bms_protocol.h"

/*********************
 *      DEFINES
 *********************/
#define TAG                     "HW_BMS"

#define PKT_TX_START_IDX            0
#define PKT_TX_OP_IDX               1
#define PKT_TX_CMD_IDX              2
#define PKT_TX_SIZE_IDX             3
#define PKT_TX_DATA_IDX             4
#define PKT_TX_CHECKSUM_IDX(_size)  ( PKT_TX_DATA_IDX + (_size) )
#define PKT_TX_END_IDX(_size)       ( PKT_TX_CHECKSUM_IDX( (_size) + 2 ) )

#define PKT_TX_STATIC_SIZE          7
#define PKT_TX_DATA_SIZE(_data)     (((_data)[PKT_TX_SIZE_IDX]))
#define PKT_TX_SIZE(_data)          (PKT_TX_DATA_SIZE(_data) + PKT_TX_STATIC_SIZE)

#define PKT_RX_START_IDX            0
#define PKT_RX_CMD_IDX              1
#define PKT_RX_STATE_IDX            2
#define PKT_RX_SIZE_IDX             3
#define PKT_RX_DATA_IDX             4
#define PKT_RX_CHECKSUM_IDX(_size)  ( PKT_RX_DATA_IDX + (_size) )
#define PKT_RX_END_IDX(_size)       ( PKT_RX_CHECKSUM_IDX( (_size) + 2 ) )

#define PKT_RX_STATIC_SIZE          7
#define PKT_RX_DATA_SIZE(_data)     (((_data)[PKT_RX_SIZE_IDX]))
#define PKT_RX_SIZE(_data)          (PKT_RX_DATA_SIZE(_data) + PKT_RX_STATIC_SIZE)

#define RX_BUF_SZ                   128
#define TX_BUF_SZ                   16

#define START_BYTE                  0xDD
#define WRITE_BYTE                  0x5A
#define READ_BYTE                   0xA5
#define END_BYTE                    0x77

/**********************
 *      TYPEDEFS
 **********************/

typedef uint8_t cmd_t; enum {
    CMD_BASIC_INFO          = 0x03,
    CMD_CELL_VOLTAGES       = 0x04,
    CMD_HARDWARE_VER        = 0x05
    };


/**********************
 *      MACROS
 **********************/

/**********************
 *     GLOBALS
 **********************/
static TimerHandle_t            g_bms_request_timer;

/**********************
 *     CONSTANTS
 **********************/

/**********************
 *    PROTOTYPES
 **********************/
static bool send_packet( cmd_t cmd, uint8_t * data, uint8_t data_sz );
static bool recv_packet( cmd_t * cmd, uint8_t * data, uint8_t * data_sz );

static uint16_t calc_checksum( uint8_t * pkt_ptr, uint8_t pkt_sz );

_Noreturn static void bms_parse_task( void * params );
void bms_request( TimerHandle_t xTimer );
static bool parse_basic_status( uint8_t * data, uint32_t data_sz );

void hw_bms_init(void)
{
    ESP_LOGI(TAG, "BMS Intf Init");

    /* Configure parameters of an UART driver,
    * communication pins and install the driver */
    uart_config_t uart_config = {
            .baud_rate = BMS_BAUD_RATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
    };

    //Install UART driver, and get the queue.
    ESP_ERROR_CHECK(uart_driver_install( BMS_UART, 2 * RX_BUF_SZ, 0, 0, NULL, 0) );
    ESP_ERROR_CHECK(uart_param_config( BMS_UART, &uart_config) );

    //Set UART pins (using UART0 default pins ie no changes.)
    ESP_ERROR_CHECK(uart_set_pin(BMS_UART, BMS_TX, BMS_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void hw_bms_start( void )
{
    //Create a task to handler UART event from ISR
    xTaskCreate( bms_parse_task, "bms_parse_task", 4 * 1024, NULL, 12, NULL );

    g_bms_request_timer = xTimerCreate( "bms_request", pdMS_TO_TICKS(1000), pdTRUE, NULL, bms_request );
    xTimerStart( g_bms_request_timer, 0 );
}

void hw_bms_stop( void )
{
    // TODO: Stop Timer & Thread
}

_Noreturn static void bms_parse_task( void * params )
{
    bool        success;
    cmd_t       cmd;
    uint8_t     pkt_data[RX_BUF_SZ];
    uint8_t     pkt_data_sz;
    for(;;) {
        // Read Response portMAX_DELAY
        pkt_data_sz = sizeof(pkt_data);
        success = recv_packet( &cmd, pkt_data, &pkt_data_sz );

        if( !success ) {
            continue;
        }

        // Handle Command
        if( success ) {
            switch( cmd ) {
                case CMD_BASIC_INFO: {
                    hw_bms_basic_status_resp_t  resp;

                    success = hw_bms_basic_status_resp( &resp, pkt_data, pkt_data_sz );

                    if( success ) {
                        printf("total voltage: %f\n", resp.total_voltage);
//                        PUB_DBL("bms.total_voltage", (double)(resp->total_voltage * 0.01));
//                        PUB_DBL("bms.current", (double)(resp->current * 0.01));
//                        PUB_DBL("bms.residual_capacity", (double)(resp->residual_capacity * 0.01));
//                        PUB_DBL("bms.nominal_capacity", (double)(resp->nominal_capacity * 0.01));
//                        PUB_INT("bms.cycle_life", resp->cycle_life);
//                        PUB_INT("bms.balance_status", (resp->balance_status_high << 16) | resp->balance_status);
//
//                        PUB_BOOL("bms.protection_status.mosfet_software_lock", resp->protection_status.mosfet_software_lock);
//                        PUB_BOOL("bms.protection_status.ic_frontend_error", resp->protection_status.ic_frontend_error);
//                        PUB_BOOL("bms.protection_status.short_circuit", resp->protection_status.short_circuit);
//                        PUB_BOOL("bms.protection_status.discharge_overcurrent", resp->protection_status.discharge_overcurrent);
//                        PUB_BOOL("bms.protection_status.charge_overcurrent", resp->protection_status.charge_overcurrent);
//                        PUB_BOOL("bms.protection_status.discharge_low_temp", resp->protection_status.discharge_low_temp);
//                        PUB_BOOL("bms.protection_status.discharge_high_temp", resp->protection_status.discharge_high_temp);
//                        PUB_BOOL("bms.protection_status.charge_low_temp", resp->protection_status.charge_low_temp);
//                        PUB_BOOL("bms.protection_status.charge_high_temp", resp->protection_status.charge_high_temp);
//                        PUB_BOOL("bms.protection_status.battery_under_voltage", resp->protection_status.battery_under_voltage);
//                        PUB_BOOL("bms.protection_status.battery_over_voltage", resp->protection_status.battery_over_voltage);
//                        PUB_BOOL("bms.protection_status.cell_under_voltage", resp->protection_status.cell_under_voltage);
//                        PUB_BOOL("bms.protection_status.cell_over_voltage", resp->protection_status.cell_over_voltage);
//
//                        PUB_INT("bms.version", resp->version);
//                        PUB_INT("bms.rsoc", resp->rsoc);
//                        PUB_BOOL("bms.fet_control_status.is_charging", resp->fet_control_status.is_charging);
//                        PUB_BOOL("bms.fet_control_status.is_discharging", resp->fet_control_status.is_discharging);
//                        PUB_INT("bms.cell_series_count", resp->cell_series_count);
//
//                        for(int idx = 0; idx < resp->ntc_temperature.count; idx++) {
//                            char tag[32];
//                            snprintf(tag, sizeof(tag), "bms.ntc_temperature_%d", idx);
//
//                            PUB_DBL(tag, (double)(resp->ntc_temperature.elem[idx] * 0.01));
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
    }

    vTaskDelete(NULL);
}

void bms_request( TimerHandle_t xTimer )
{
    send_packet(CMD_BASIC_INFO, NULL, 0);
}

static bool send_packet( cmd_t cmd, uint8_t * data, uint8_t data_sz )
{
    bool        success;

    // Will packet fit buffer?
    success = ( data_sz < (TX_BUF_SZ - PKT_TX_STATIC_SIZE) );

    // Construct & Send Packet
    if( success ) {
        uint8_t     pkt_data[TX_BUF_SZ];
        uint16_t    checksum;

        //{ 0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77 };
        pkt_data[PKT_TX_START_IDX] = START_BYTE;
        pkt_data[PKT_TX_OP_IDX] = READ_BYTE;
        pkt_data[PKT_TX_CMD_IDX] = cmd;
        pkt_data[PKT_TX_SIZE_IDX] = data_sz;

        for( int idx = 0; idx < data_sz; idx++ ) {
            pkt_data[PKT_TX_DATA_IDX + idx] = data[idx];
        }

        checksum = calc_checksum( pkt_data, PKT_TX_SIZE(pkt_data) );

        pkt_data[PKT_TX_CHECKSUM_IDX(data_sz) + 0] = ( checksum >> 8 ) & 0x00FF;
        pkt_data[PKT_TX_CHECKSUM_IDX(data_sz) + 1] = ( checksum >> 0 ) & 0x00FF;

        pkt_data[PKT_TX_END_IDX(data_sz)] = END_BYTE;

        // Send Request Command
        uart_write_bytes( UART_NUM_1, (const char *)pkt_data, PKT_TX_SIZE(pkt_data) );
    }

    return success;
}

static bool recv_packet( cmd_t * cmd, uint8_t * data, uint8_t * data_sz )
{
    bool        success;
    uint8_t     pkt_data[RX_BUF_SZ];
    uint8_t     pkt_data_sz = 0;
    uint16_t    pkt_checksum = 0;

    // Validate Inputs
    success = (NULL != cmd) && (NULL != data) && (NULL != data_sz);

    // Receive Start Byte
    if( success ) {
        success = ( sizeof(uint8_t) == uart_read_bytes( BMS_UART, &pkt_data[PKT_RX_START_IDX], sizeof(uint8_t), portMAX_DELAY ) );
    }

    // Check Start Byte
    if( success ) {
        success = ( START_BYTE == pkt_data[PKT_RX_START_IDX] );
    }

    // Receive Command Byte
    if( success ) {
        success = ( sizeof(uint8_t) == uart_read_bytes( BMS_UART, &pkt_data[PKT_RX_CMD_IDX], sizeof(uint8_t), portMAX_DELAY ) );
    }

    // Receive State Byte
    if( success ) {
        success = ( sizeof(uint8_t) == uart_read_bytes( BMS_UART, &pkt_data[PKT_RX_STATE_IDX], sizeof(uint8_t), portMAX_DELAY ) );
    }

    // Receive Size Byte
    if( success ) {
        success = ( sizeof(uint8_t) == uart_read_bytes( BMS_UART, &pkt_data[PKT_RX_SIZE_IDX], sizeof(uint8_t), portMAX_DELAY ) );

        if( success ) {
            pkt_data_sz = PKT_RX_DATA_SIZE(pkt_data);
        }
    }

    // Receive Data
    if( success ) {
        success = ( (pkt_data_sz * sizeof(uint8_t)) == uart_read_bytes( BMS_UART, &pkt_data[PKT_RX_DATA_IDX], (pkt_data_sz * sizeof(uint8_t)), portMAX_DELAY ) );
    }

    // Receive Checksum
    if( success ) {
        // High Byte
        success = ( sizeof(uint8_t) == uart_read_bytes( BMS_UART, &pkt_data[PKT_RX_CHECKSUM_IDX(pkt_data_sz) + 1], sizeof(uint8_t), portMAX_DELAY ) );

        // Low Byte
        if( success ) {
            success = ( sizeof(uint8_t) == uart_read_bytes( BMS_UART, &pkt_data[PKT_RX_CHECKSUM_IDX(pkt_data_sz)], sizeof(uint8_t), portMAX_DELAY ) );
        }

        if( success ) {
            pkt_checksum = *((uint16_t *)&pkt_data[PKT_RX_CHECKSUM_IDX(pkt_data_sz)]);
        }
    }

    // Receive End Byte
    if( success ) {
        success = ( sizeof(uint8_t) == uart_read_bytes( BMS_UART, &pkt_data[PKT_RX_END_IDX(pkt_data_sz)], sizeof(uint8_t), portMAX_DELAY ) );
    }

    // Validate Checksum
    if( success ) {
        success = ( calc_checksum(pkt_data, PKT_RX_SIZE(pkt_data)) == pkt_checksum );
    }

    // Validate Output Buffer Size
    if( success ) {
        success = ( pkt_data_sz < *data_sz );
    }

    // Setup Return Buffers
    if( success ) {
        *cmd = pkt_data[PKT_RX_CMD_IDX];
        *data_sz = pkt_data_sz;
        memcpy(data, &pkt_data[PKT_RX_DATA_IDX], pkt_data_sz);
    }

    return success;
}

static uint16_t calc_checksum( uint8_t * pkt_ptr, uint8_t pkt_sz )
{
    uint16_t checksum = 0xFFFF;

    // Subtract Data (Calculate based on data between Start Byte and Checksum)
    // TODO: Clean up the magic numbers
    for( int i = 2; i < (pkt_sz - 3); i++ ) {
        checksum -= pkt_ptr[i];
    }

    checksum += 1;

    return checksum;
}