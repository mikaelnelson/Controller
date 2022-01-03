
/*********************
 *      INCLUDES
 *********************/
#include <pubsub.h>
#include <string.h>

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "hw_rpm_pulse_counter.h"

#include "driver/pcnt.h"

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
#define COUNTER_UNIT        PCNT_UNIT_0

#define PULSE_INPUT         15

#define SAMPLING_WINDOW_MS  (100)   //100ms

/**********************
 *    PROTOTYPES
 **********************/
void pulse_timer_fn( TimerHandle_t xTimer );


void hw_rpm_pulse_counter_init(void)
{
    ESP_LOGI(TAG, "RPM Pulse Counter Init");

    // Configure Pulse Counter
    pcnt_config_t pcnt_config = {
            .pulse_gpio_num     = PULSE_INPUT,
            .ctrl_gpio_num      = -1, // Ignore Control Pin
            .channel            = PCNT_CHANNEL_0,
            .unit               = COUNTER_UNIT,
            .pos_mode           = PCNT_COUNT_INC,
            .neg_mode           = PCNT_COUNT_DIS,
            .lctrl_mode         = PCNT_MODE_KEEP,
            .hctrl_mode         = PCNT_MODE_KEEP,
            .counter_h_lim      = 10000,
            .counter_l_lim      = 0,
    };

    pcnt_unit_config( &pcnt_config );

    // Pause & Clear Counter
    pcnt_counter_pause(COUNTER_UNIT);
    pcnt_counter_clear(COUNTER_UNIT);

    // Setup Timer
    m_pulse_timer = xTimerCreate( "pulse_timer", SAMPLING_WINDOW_MS / portTICK_RATE_MS, pdTRUE, NULL, pulse_timer_fn );

}

void hw_rpm_pulse_counter_start(void)
{
    // Clear & Resume Counter
    pcnt_counter_clear(COUNTER_UNIT);
    pcnt_counter_resume(COUNTER_UNIT);

    // Start Timer
    xTimerStart( m_pulse_timer, 0 );
}

void hw_rpm_pulse_counter_stop(void)
{
    // Stop Timer
    xTimerStop( m_pulse_timer, 0 );

    // Pause & Clear Counter
    pcnt_counter_pause(COUNTER_UNIT);
    pcnt_counter_clear(COUNTER_UNIT);
}

void pulse_timer_fn( TimerHandle_t xTimer )
{
    int16_t         pulse_count = 0;

    // Get Pulse Count
    if( pcnt_get_counter_value( COUNTER_UNIT, &pulse_count ) == ESP_OK ) {
        // Determine RPM from Pulse Count
        int32_t rpm = pulse_count * ( ( 1000 / SAMPLING_WINDOW_MS ) * 60 );

        PUB_INT("rpm_pulse_counter", rpm);
    }

    // Clear Counter
    pcnt_counter_clear(COUNTER_UNIT);
}