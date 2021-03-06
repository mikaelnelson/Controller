#include <pubsub.h>

#include "hw_rpm_pulse_counter.h"
#include "hw_ws2812b_ring.h"
#include "hw_bms/hw_bms.h"
#include "hw_motor_controller/hw_motor_controller.h"

#include "sys_battery_status_led.h"

// pio run --target upload
void app_main()
{
    /*
     * Todo: Start monitoring task with state machine. This
     * state machine should allow a run state and a sleep state.
     * The sleep state should disable all unneccissary tasks.
     *
     * Until implemented, assume run state.
     * */

    /*
     * Initialization
     */
    ps_init();
    hw_rpm_pulse_counter_init();
    hw_ws2812b_ring_init();
    hw_bms_init();
    hw_motor_controller_init();
    //hw_tcl59108_init();
    //hw_pca9531_init();

    sys_battery_status_led_init();
    //sys_backlight_init();
    //sys_speedometer_init();
    //sys_display_init();
    //sys_switch_init();

    /*
     * Start
     */
    hw_rpm_pulse_counter_start();
    hw_bms_start();
    hw_motor_controller_start();

    sys_battery_status_led_start();
}