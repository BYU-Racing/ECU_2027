#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "../can/CAN_message_t.h"

#ifdef __cplusplus
extern "C" {
#endif


/* PEDALS */

/* This section deals with processing the values from the pedal sensors,
 * currently the two throttle sensors and the brake sensor. It also deals with
 * maintaining safety features of the sensors, like value timeouts and out of
 * range failures. */

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

/* Feed every received CAN message through this */
void ecu_process_message(uint32_t current_time_ms, CAN_message_t msg);

/* The caller of this should continously call this until it returns
  * 1. For each message emitted, the caller should send
  * along the CAN bus. */
bool ecu_poll_can(uint32_t current_time_ms, CAN_message_t *out);

GpioState ecu_poll_gpio_state(uint32_t current_time_ms);

#ifdef __cplusplus
}
#endif