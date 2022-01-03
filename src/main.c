#include "hw_rpm_pulse_counter.h"

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
  hw_rpm_pulse_counter_init();


  /*
   * Start
   */
    hw_rpm_pulse_counter_start();
}