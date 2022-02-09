/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <endian.h>

#include "hw_bms_protocol.h"

/**********************
 *      DEFINES
 *********************/
#define BASIC_STATUS_TOTAL_VOLTAGE_OFST         0
#define BASIC_STATUS_CURRENT_OFST               2
#define BASIC_STATUS_RESIDUAL_CAPACITY_OFST     4
#define BASIC_STATUS_NOMINAL_CAPACITY_OFST      6
#define BASIC_STATUS_CYCLE_LIFE_OFST            8
#define BASIC_STATUS_PRODUCT_DATE_OFST          10
#define BASIC_STATUS_BALANCE_STATUS_OFST        12
#define BASIC_STATUS_BALANCE_STATUS_HIGH_OFST   14
#define BASIC_STATUS_PROTECTION_STATUS_OFST     16
#define BASIC_STATUS_VERSION_OFST               18
#define BASIC_STATUS_RSOC_OFST                  19
#define BASIC_STATUS_FET_CONTROL_STATUS_OFST    20
#define BASIC_STATUS_CELL_SERIES_COUNT_OFST     21
#define BASIC_STATUS_NTC_TEMPERATURE_CNT_OFST   22
#define BASIC_STATUS_NTC_TEMPERATURE_TBL_OFST   23


#define BASIC_STATUS_STATIC_SIZE                23
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *       CONSTS
 **********************/

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


bool hw_bms_basic_status_resp_parse( hw_bms_basic_status_resp_t * resp, const uint8_t * data, size_t data_sz )
{
    bool        success;

    // Validate Input
    success = (NULL != resp) && (NULL != data);

    // Parse Packet
    if( success ) {
        resp->total_voltage = ( be16toh(*((uint16_t *)&data[BASIC_STATUS_TOTAL_VOLTAGE_OFST])) / 100.0f);
        resp->current = ( be16toh(*((uint16_t *)&data[BASIC_STATUS_CURRENT_OFST])) / 100.0f);
        resp->residual_capacity = ( be16toh(*((uint16_t *)&data[BASIC_STATUS_RESIDUAL_CAPACITY_OFST])) / 100.0f);
        resp->nominal_capacity = ( be16toh(*((uint16_t *)&data[BASIC_STATUS_NOMINAL_CAPACITY_OFST])) / 100.0f);
        resp->cycle_life = be16toh(*((uint16_t *)&data[BASIC_STATUS_CYCLE_LIFE_OFST]));
        resp->product_date = be16toh(*((uint16_t *)&data[BASIC_STATUS_PRODUCT_DATE_OFST]));
        resp->balance_status =  (be16toh(*((uint16_t *)&data[BASIC_STATUS_BALANCE_STATUS_OFST])) << 0) |
                                (be16toh(*((uint16_t *)&data[BASIC_STATUS_BALANCE_STATUS_HIGH_OFST])) << 16);

        uint16_t protection_status = be16toh(*((uint16_t *)&data[BASIC_STATUS_PROTECTION_STATUS_OFST]));
        resp->protection_status.mosfet_software_lock    = ((protection_status >> 12) & 0x01);
        resp->protection_status.ic_frontend_error       = ((protection_status >> 11) & 0x01);
        resp->protection_status.short_circuit           = ((protection_status >> 10) & 0x01);
        resp->protection_status.discharge_overcurrent   = ((protection_status >> 9) & 0x01);
        resp->protection_status.charge_overcurrent      = ((protection_status >> 8) & 0x01);
        resp->protection_status.discharge_low_temp      = ((protection_status >> 7) & 0x01);
        resp->protection_status.discharge_high_temp     = ((protection_status >> 6) & 0x01);
        resp->protection_status.charge_low_temp         = ((protection_status >> 5) & 0x01);
        resp->protection_status.charge_high_temp        = ((protection_status >> 4) & 0x01);
        resp->protection_status.battery_under_voltage   = ((protection_status >> 3) & 0x01);
        resp->protection_status.battery_over_voltage    = ((protection_status >> 2) & 0x01);
        resp->protection_status.cell_under_voltage      = ((protection_status >> 1) & 0x01);
        resp->protection_status.cell_over_voltage       = ((protection_status >> 0) & 0x01);

        resp->version = data[BASIC_STATUS_VERSION_OFST];
        resp->rsoc = data[BASIC_STATUS_RSOC_OFST];

        uint8_t fet_control_status = data[BASIC_STATUS_FET_CONTROL_STATUS_OFST];
        resp->fet_control_status.is_charging            = ((fet_control_status >> 1) & 0x01);
        resp->fet_control_status.is_discharging         = ((fet_control_status >> 0) & 0x01);

        resp->cell_series_count = data[BASIC_STATUS_CELL_SERIES_COUNT_OFST];
        resp->ntc_temperature_cnt = data[BASIC_STATUS_NTC_TEMPERATURE_CNT_OFST];

        for( int idx = 0; (idx < HW_BMS_PROTOCOL_NTC_TEMPERATURE_COUNT_MAX) && (idx < resp->ntc_temperature_cnt); idx++ ) {
            uint16_t ntc_temperature = be16toh(*((uint16_t *)&data[BASIC_STATUS_NTC_TEMPERATURE_TBL_OFST + sizeof(uint16_t)]));
            resp->ntc_temperature[idx] = (ntc_temperature / 10.0f) - 273.15f;
        }
    }

    return success;
}