
/*********************
 *      INCLUDES
 *********************/
#include <pubsub.h>
#include <string.h>
#include <math.h>

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "hw_config.h"
#include "hw_rpm_pulse_counter.h"


/*********************
 *      DEFINES
 *********************/
#define TAG                 "RPM_PULSE_COUNTER"
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *     GLOBALS
 **********************/
 static TimerHandle_t       m_pulse_timer;


/**********************
 *     CONSTANTS
 **********************/

#define SAMPLING_WINDOW_MS  (1000)   //100ms

/**********************
 *    PROTOTYPES
 **********************/
void pulse_timer_fn( TimerHandle_t xTimer );


void hw_rpm_pulse_counter_init(void)
{
    ESP_LOGI(TAG, "RPM Pulse Counter Init");

    // Configure Pulse Counter
    pcnt_config_t pcnt_config = {
            .pulse_gpio_num     = RPM_PULSE_COUNTER_INPUT,
            .ctrl_gpio_num      = RPM_PULSE_COUNTER_CONTROL,
            .channel            = RPM_PULSE_COUNTER_CHANNEL,
            .unit               = RPM_PULSE_COUNTER_UNIT,
            .pos_mode           = PCNT_COUNT_INC,
            .neg_mode           = PCNT_COUNT_DIS,
            .lctrl_mode         = PCNT_MODE_KEEP,
            .hctrl_mode         = PCNT_MODE_KEEP,
            .counter_h_lim      = 10000,
            .counter_l_lim      = 0,
    };

    pcnt_unit_config( &pcnt_config );

    // Pause & Clear Counter
    pcnt_counter_pause(RPM_PULSE_COUNTER_UNIT);
    pcnt_counter_clear(RPM_PULSE_COUNTER_UNIT);

    // Filter Pulses Shorter Than 200ns
    // Filter uses 80MHz clock (12.5nS)
    pcnt_set_filter_value(RPM_PULSE_COUNTER_UNIT, 16);
    pcnt_filter_enable(RPM_PULSE_COUNTER_UNIT);

    // Setup Timer
    m_pulse_timer = xTimerCreate( "pulse_timer", SAMPLING_WINDOW_MS / portTICK_RATE_MS, pdTRUE, NULL, pulse_timer_fn );

}

void hw_rpm_pulse_counter_start(void)
{
    // Clear & Resume Counter
    pcnt_counter_clear(RPM_PULSE_COUNTER_UNIT);
    pcnt_counter_resume(RPM_PULSE_COUNTER_UNIT);

    // Start Timer
    xTimerStart( m_pulse_timer, 0 );
}

void hw_rpm_pulse_counter_stop(void)
{
    // Stop Timer
    xTimerStop( m_pulse_timer, 0 );

    // Pause & Clear Counter
    pcnt_counter_pause(RPM_PULSE_COUNTER_UNIT);
    pcnt_counter_clear(RPM_PULSE_COUNTER_UNIT);
}

void pulse_timer_fn( TimerHandle_t xTimer )
{
    int16_t         pulse_count = 0;

    // Get Pulse Count
    if( pcnt_get_counter_value( RPM_PULSE_COUNTER_UNIT, &pulse_count ) == ESP_OK ) {
        // Determine RPM from Pulse Count
        // 4 pulses per rotation
        double rpm = ( pulse_count * ( ( 1000 / SAMPLING_WINDOW_MS ) * 60 ) / 4);

        double wheel_rpm = (double)( rpm * 24 ) / 68;
        double wheel_speed_mph = ( wheel_rpm * 15.5f * M_PI * 60 ) / (63360);

        PUB_INT("rpm_pulse_counter", rpm);
        // ESP_LOGI(TAG, "RPM Pulse Counter: %f (%f rpm, %f mph)", rpm, wheel_rpm, wheel_speed_mph);
    }

    // Clear Counter
    pcnt_counter_clear(RPM_PULSE_COUNTER_UNIT);
}