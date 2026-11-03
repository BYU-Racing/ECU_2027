#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../can/CAN_message_t.h"
#include "../shared/ecu_assert.h"

#ifdef __cplusplus
extern "C" {
#endif


/* PEDALS */

/* This section deals with processing the values from the pedal sensors,
 * currently the two throttle sensors and the brake sensor. It also deals with
 * maintaining safety features of the sensors, like value timeouts and out of
 * range failures. */

/* If an implausibility occurs (2026 rules, T.4.2.4), we save its details so
* that if it lasts too long we can report what it was and where it came from.
* `active` replaces the old std::optional. */
typedef struct ImplausibilityDetails {
  bool active;
  LineInfo line_info;
  uint32_t happened_at_ms;
  AssertCode code;
} ImplausibilityDetails;

void pedals_poll(uint32_t current_time_ms);
void pedals_input_throttle_one_position(uint32_t current_time_ms, uint16_t value);
void pedals_input_throttle_two_position(uint32_t current_time_ms, uint16_t value);
void pedals_input_brake_position(uint32_t current_time_ms, uint16_t position);
void ecu_set_start_switch(void);
void ecu_reset_start_switch(void);
void ecu_update(uint32_t current_time_ms);
bool pedals_is_brake_pressed(void);
uint8_t pedals_get_throttle_value(void);
int16_t pedals_get_current_torque_amount(void);


/* ECU */
typedef struct {
  bool horn_on;
  bool brake_light_on;
} GpioState;

/* Handles startup and shutdown sequence that are controlled but the start switch and vsm CAN commands as well as the horn output. */
void ecu_handle_startup_sequence(uint32_t current_time_ms);


/* Feed every received CAN message through this. Checks for certain flags (vsm) and updates local variables */
void ecu_process_message(uint32_t current_time_ms, CAN_message_t msg);

/* The caller of this should continously call this until it returns
  * 1. For each message emitted, the caller should send
  * along the CAN bus. */
bool ecu_poll_can(uint32_t current_time_ms, CAN_message_t *out);

/* Handles the updating of horn and brake light GPIO variables */
GpioState ecu_poll_gpio_state(uint32_t current_time_ms);

/* Prints the current state and variables of ECU as well as the timing for each function in main */
void ecu_print_state();

/* Timing evals, holds the ms time of how long each section of the poll took */
typedef struct PollTiming {
  uint32_t throttle_1_timing;
  uint32_t throttle_2_timing;
  uint32_t brake_timing;
  uint32_t pump_timing;
  uint32_t can_bus_read_timing;
  uint32_t start_switch_timing;
  uint32_t can_bus_write_timing;
  uint32_t gpio_state_timing;
  uint32_t debug_timing;
} PollTiming;

extern struct PollTiming poll_timing;


/* PWM */
// TO DO: figure out values for the pump
//uint16_t pump_duty_cycle;

#ifdef __cplusplus
}
#endif