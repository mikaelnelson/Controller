/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/**********************
 *      DEFINES
 *********************/
#define START_IDX           0
#define CMD_IDX             1
#define STATE_IDX           2
#define SIZE_IDX            3
#define DATA_IDX            4

#define BASIC_STATUS_TOTAL_VOLTAGE_OFST         0
#define BASIC_STATUS_CURRENT_OFST               2
#define BASIC_STATUS_RESIDUAL_CAPACITY_OFST     4
#define BASIC_STATUS_NOMINAL_CAPACITY_OFST      6
#define BASIC_STATUS_CYCLE_LIFE_OFST            8
#define BASIC_STATUS_PRODUCT_DATE_OFST          10
#define BASIC_STATUS_BALANCE_STATUS_OFST        12
#define BASIC_STATUS_BALANCE_STATUS_HIGH_OFST   14
#define BASIC_STATUS_PROTECTION_STATUS_OFST     16
#define BASIC_STATUS_VERSION_OFST               17
#define BASIC_STATUS_RSOC_OFST                  18
#define BASIC_STATUS_FET_CONTROL_STATUS_OFST    19
#define BASIC_STATUS_CELL_SERIES_COUNT_OFST     20
#define BASIC_STATUS_NTC_TEMPERATURE_CNT_OFST   21
#define BASIC_STATUS_NTC_TEMPERATURE_TBL_OFST   22


#define BASIC_STATUS_STATIC_SIZE                22
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

static uint16_t calc_checksum( uint8_t * data );





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
        checksum -= data[DATA_IDX + i];
    }

    checksum += 1;

    return checksum;
}