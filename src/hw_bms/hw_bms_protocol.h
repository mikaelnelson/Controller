#ifndef HW_BMS_PROTOCOL_H
#define HW_BMS_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
#define HW_BMS_PROTOCOL_NTC_TEMPERATURE_COUNT_MAX       3

typedef struct {
    float               total_voltage;
    float               current;
    float               residual_capacity;
    float               nominal_capacity;
    uint16_t            cycle_life;
    uint16_t            product_date;
    uint32_t            balance_status;
    struct {
        bool            mosfet_software_lock;
        bool            ic_frontend_error;
        bool            discharge_overcurrent;
        bool            charge_overcurrent;
        bool            discharge_low_temp;
        bool            discharge_high_temp;
        bool            charge_low_temp;
        bool            discharge_high_temp;
        bool            battery_under_voltage;
        bool            battery_over_voltage;
        bool            cell_under_voltage;
        bool            cell_over_voltage;
    } protection_status;

    uint8_t             version;
    uint8_t             rsoc;

    struct {
        bool            is_charging;
        bool            is_discharging;
    } fet_control_status;

    uint8_t             cell_series_count;
    uint8_t             ntc_temperature_cnt;
    float               ntc_temperature[HW_BMS_PROTOCOL_NTC_TEMPERATURE_COUNT_MAX];
    } hw_bms_basic_status_resp_t;


/**********************
 *      MACROS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

bool hw_bms_basic_status_resp( hw_bms_basic_status_resp_t * resp, const uint8_t * data, size_t data_sz );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //HW_BMS_PROTOCOL_H
