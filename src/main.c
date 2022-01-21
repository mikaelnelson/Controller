#include <pubsub.h>

#include "hw_rpm_pulse_counter.h"
#include "hw_ws2812b_ring.h"
#include "hw_bms.h"

#include "sys_battery_status_led.h"
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

    sys_battery_status_led_init();

  /*
   * Start
   */
  hw_rpm_pulse_counter_start();
    hw_bms_start();
    sys_battery_status_led_start();
}