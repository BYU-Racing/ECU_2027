#include "ecu_logic.h"

#include <stddef.h>
#include <stdlib.h>

#include "ecu_assert.h"
#include "can_serde.h"
#include "constants.h"
#include "util.h"

#define TORQUE_MEMORY_LENGTH (sizeof(torque_memory) / sizeof(torque_memory[0]))
#define HORN_TIME_MS 2000

/* ====================================================================== */
/* Pedals state                                                            */
/* ====================================================================== */

/* If an implausibility occurs (2026 rules, T.4.2.4), we save its details so
 * that if it lasts too long we can report what it was and where it came from.
 * `active` replaces the old std::optional. */
typedef struct ImplausibilityDetails {
    bool active;
    LineInfo line_info;
    uint32_t happened_at_ms;
    AssertCode code;
} ImplausibilityDetails;

static ImplausibilityDetails pedals_implausibility;

/* Message timeouts (if it's been too long since a pedal message, we panic). */
static Trigger pedals_too_long_since_throttle_1;
static Trigger pedals_too_long_since_throttle_2;
static Trigger pedals_too_long_since_brake;

/* Last readings. The `_valid` flags replace the old std::optional: a real
 * reading of 0 (sensor shorted to ground) must still be range-checked, so 0
 * can't be used to mean "no reading yet". */
static bool     throttle_1_valid;
static uint16_t throttle_1_position;
static bool     throttle_2_valid;
static uint16_t throttle_2_position;

/* Intermediate result after mapping both sensors to [0, 100] and averaging. */
static uint8_t mapped_throttle;

/* Current brake position. Raw sensor reading. */
static uint16_t brake_position;

/* Throttle smoothing memory, shifted every SMOOTH_PERIOD_MS. */
static uint8_t torque_memory[3];
static Timer   torque_memory_pacing = { 0, SMOOTH_PERIOD_MS };
static uint8_t smoothed_throttle;

/* PID with the target as the current target torque, and the measurement as the
 * last output. Equivalent to the old Pid(0, 0.0, 7.8, 0.0, 20.0). */
static Pid    throttle_pid;
static Timer  throttle_pid_pacing = { 0, PID_PERIOD_MS };
static double throttle_last_output;
static double throttle_pid_output;

/* ====================================================================== */
/* ECU state                                                               */
/* ====================================================================== */

static bool car_fully_on;
static bool horn_on;
static Trigger horn_off_trigger;
static bool start_switch_on;
/* Brake and throttle cannot be pressed at the same time. 2026 rules, EV.4.7. */
static bool throttle_and_brake_pressed;
static bool brake_light_on;
/* When the brake is pressed and the enable switch is flipped, we still wait
 * 2 seconds before enabling the motor. This is that countdown. */
static Trigger ecu_startup_countdown;
/* Paces how often we send motor control commands. */
static Timer ecu_motor_control_pacing = { 0, 15 };
static bool precharge_complete;
static uint32_t vsm_last_rx_ms;
static bool     vsm_seen;

/* ====================================================================== */
/* Pedals                                                                  */
/* ====================================================================== */

static void pedals_maybe_recompute_mapped_throttle(uint32_t current_time_ms);
static void pedals_smooth_throttle(uint32_t current_time_ms);
static void pedals_throttle_post_processing(uint32_t current_time_ms);
static void pedals_note_implausibility(uint32_t current_time_ms, LineInfo line_info, AssertCode code);

/* Currently a stub. Implement the torque mapping here. Returns Nm, _not_ 0.1*Nm. */
static double map_throttle_to_torque(uint16_t throttle_percent) {
    int64_t result = map_range(throttle_percent, 0, 100, 0, MAX_TORQUE);
    return (double)(result) / 10.0;
}

void pedals_poll(uint32_t current_time_ms) {
    /* 2026 rules, T.4.2.11(b): if no message has arrived for too long, panic.
     * We proactively start these triggers and cancel them whenever a message
     * arrives (see `pedals_input_throttle_one_position`). */
    trigger_start_if_stopped(&pedals_too_long_since_throttle_1, current_time_ms, PEDAL_TIMEOUT_MS);
    trigger_start_if_stopped(&pedals_too_long_since_throttle_2, current_time_ms, PEDAL_TIMEOUT_MS);
    trigger_start_if_stopped(&pedals_too_long_since_brake, current_time_ms, PEDAL_TIMEOUT_MS);

    if (trigger_reached(&pedals_too_long_since_throttle_1, current_time_ms)) {
        SAFETY_ASSERT(false, ASSERT_CODE_PEDAL_TIMEOUT);
    }
    if (trigger_reached(&pedals_too_long_since_throttle_2, current_time_ms)) {
        SAFETY_ASSERT(false, ASSERT_CODE_PEDAL_TIMEOUT);
    }
    if (trigger_reached(&pedals_too_long_since_brake, current_time_ms)) {
        SAFETY_ASSERT(false, ASSERT_CODE_PEDAL_TIMEOUT);
    }

    if (pedals_implausibility.active && (current_time_ms - pedals_implausibility.happened_at_ms > ALLOWED_IMPLAUSILIBTY_LENGTH_MS)) {
        ecu_assert_failed(ASSERT_LEVEL_SAFETY, pedals_implausibility.line_info, pedals_implausibility.code);
    }

    pedals_smooth_throttle(current_time_ms);
    pedals_throttle_post_processing(current_time_ms);
}

void pedals_input_throttle_one_position(uint32_t current_time_ms, uint16_t value) {
    throttle_1_position = value;
    throttle_1_valid = true;
    /* Reset the timeout for receiving a message. */
    trigger_cancel(&pedals_too_long_since_throttle_1);
    pedals_maybe_recompute_mapped_throttle(current_time_ms);
}

void pedals_input_throttle_two_position(uint32_t current_time_ms, uint16_t value) {
    throttle_2_position = value;
    throttle_2_valid = true;
    trigger_cancel(&pedals_too_long_since_throttle_2);
    pedals_maybe_recompute_mapped_throttle(current_time_ms);
}

void pedals_input_brake_position(uint32_t current_time_ms, uint16_t position) {
    SAFETY_ASSERT(position >= BRAKE_PRESSURE_MIN, ASSERT_CODE_BRAKE_SENSOR_FAULT); 
    brake_position = position;    trigger_cancel(&pedals_too_long_since_brake);
    pedals_maybe_recompute_mapped_throttle(current_time_ms);
    brake_light_on = pedals_is_brake_pressed();
}

void ecu_set_start_switch(void) {
    start_switch_on = true;
}

void ecu_reset_start_switch(void) {
    start_switch_on = false;
}


uint8_t pedals_get_throttle_value(void) {
    return smoothed_throttle;
}

bool pedals_is_brake_pressed(void) {
    return brake_position >= BRAKE_CONSIDERED_PRESSED;
}

int16_t pedals_get_current_torque_amount(void) {
    double cascadia_format = throttle_pid_output * 10.0;
    /* The PID may undershoot, so clamp it if it gets below 0. */
    if (cascadia_format < 0.0) cascadia_format = 0.0;
    /* unsmoothed, 0.1 Nm */
    double request = map_throttle_to_torque(mapped_throttle) * 10.0;
    if (cascadia_format > request){
        cascadia_format = request;
    }
    if (cascadia_format > (double)MAX_TORQUE) {
        cascadia_format = (double)MAX_TORQUE;
    }

    SAFETY_ASSERT(cascadia_format <= INT16_MAX, ASSERT_CODE_INTEGER_OVERFLOW);
    return (int16_t)cascadia_format;
}

static void pedals_maybe_recompute_mapped_throttle(uint32_t current_time_ms) {
    if (!throttle_1_valid || !throttle_2_valid) {
        return;
    }

    uint16_t throttle1 = throttle_1_position;
    uint16_t throttle2 = throttle_2_position;

    /* Make sure the throttle values are in range. These are garbage values,
     * so we return early and don't use them. */
    if (throttle1 < THROTTLE1_MIN_OUT_OF_RANGE || throttle1 > THROTTLE1_MAX_OUT_OF_RANGE ||
        throttle2 < THROTTLE2_MIN_OUT_OF_RANGE || throttle2 > THROTTLE2_MAX_OUT_OF_RANGE) {
        pedals_note_implausibility(current_time_ms, CAPTURE_LINE_INFO(), ASSERT_CODE_THROTTLE_OUT_OF_RANGE);
        return;
    }

    int64_t throttle_1_percent = map_range(throttle1, THROTTLE1_LOW, THROTTLE1_HIGH, 0, 100);
    int64_t throttle_2_percent = map_range(throttle2, THROTTLE2_LOW, THROTTLE2_HIGH, 0, 100);

    /* Values may go slightly below 0 or above 100, so saturate (anything
     * significantly out of range returned early above). */
    if (throttle_1_percent < 0) throttle_1_percent = 0;
    if (throttle_1_percent > 100) throttle_1_percent = 100;
    if (throttle_2_percent < 0) throttle_2_percent = 0;
    if (throttle_2_percent > 100) throttle_2_percent = 100;

    /* Make sure the two throttle values haven't diverged too far. */
    if (llabs(throttle_1_percent - throttle_2_percent) >= THROTTLE_DISAGREE) {
        pedals_note_implausibility(current_time_ms, CAPTURE_LINE_INFO(), ASSERT_CODE_THROTTLE_SENSORS_DIVERGED);
        return;
    }

    /* good sample, implausibility over */
    pedals_implausibility.active = false;
    mapped_throttle = (uint8_t)((throttle_1_percent + throttle_2_percent) / 2);
}

static void pedals_smooth_throttle(uint32_t current_time_ms) {
    if (timer_should_fire(&torque_memory_pacing, current_time_ms)) {
        /* Only cycle memory when it's been long enough. */
        for (size_t i = TORQUE_MEMORY_LENGTH - 1; i >= 1; i--) {
            torque_memory[i] = torque_memory[i - 1];
        }
        torque_memory[0] = mapped_throttle;
    }

    uint32_t total_torque = 0;
    for (size_t i = 0; i < TORQUE_MEMORY_LENGTH; i++) {
        total_torque += torque_memory[i];
    }

    uint32_t averaged = total_torque / TORQUE_MEMORY_LENGTH;
    SAFETY_ASSERT(averaged <= UINT8_MAX, ASSERT_CODE_INTEGER_OVERFLOW);

    smoothed_throttle = (uint8_t)averaged;
}

/* This function is responsible for the "feel" of the throttle: torque mapping and PID. */
static void pedals_throttle_post_processing(uint32_t current_time_ms) {
    if (timer_should_fire(&throttle_pid_pacing, current_time_ms)) {
        double torque_target = map_throttle_to_torque(smoothed_throttle);

        throttle_pid_output = pid_next_value(&throttle_pid, current_time_ms, torque_target, throttle_last_output);
        throttle_last_output = throttle_pid_output;
    }
}

/* Records that an implausibility happened. We don't panic immediately, as the
 * rules allow tolerating one for up to 100 ms (2026 rules, T.4.2.5). */
static void pedals_note_implausibility(uint32_t current_time_ms, LineInfo line_info, AssertCode code) {
    if (pedals_implausibility.active) {
        return;
    }

    pedals_implausibility.active = true;
    pedals_implausibility.happened_at_ms = current_time_ms;
    pedals_implausibility.line_info = line_info;
    pedals_implausibility.code = code;
}

/* ====================================================================== */
/* ECU                                                                     */
/* ====================================================================== */

static void ecu_handle_startup_sequence(uint32_t current_time_ms) {
    if (!vsm_seen || (current_time_ms - vsm_last_rx_ms) > VSM_TIMEOUT_MS) {
        precharge_complete = false;
    }

    bool car_fully_on_before = car_fully_on;

    if (!car_fully_on) {
        /* To start the motor we need:
         *   1. The brake is down.
         *   2. The start switch is on.
         *   3. The inverter has reached Pre-charge Complete.
         * Then we wait two seconds, aborting if a precondition stops holding. */
        if (pedals_is_brake_pressed() && mapped_throttle <= 5 && start_switch_on && precharge_complete) {
            if (!trigger_started(&ecu_startup_countdown)) {
                trigger_start(&ecu_startup_countdown, current_time_ms, STARTUP_DELAY_MS);
            } else if (trigger_reached(&ecu_startup_countdown, current_time_ms)) {
                car_fully_on = true;
            }
        } else {
            /* A precondition failed, so reset the countdown. */
            trigger_cancel(&ecu_startup_countdown);
        }
    }

    /* catches precharge not completing */
    static uint32_t vsm_last_rx_ms; 
    static bool vsm_seen;     
    if (!vsm_seen || (current_time_ms - vsm_last_rx_ms) > VSM_TIMEOUT_MS) {
        precharge_complete = false;
    }
    if (!start_switch_on || !precharge_complete) { 
        car_fully_on = false; 
    }

    if (!car_fully_on_before && car_fully_on) {
        // Starts PID timer
        pid_init(&throttle_pid, current_time_ms, 0.0, 7.8, 0.0, 20.0);

        /* The car just turned on, so sound the horn for 2 seconds. */
        horn_on = true;
        trigger_start(&horn_off_trigger, current_time_ms, HORN_TIME_MS);
    }

    /* Turn the horn off afterwards. */
    if (trigger_reached(&horn_off_trigger, current_time_ms)) {
        horn_on = false;
    }
}

void ecu_update(uint32_t current_time_ms){ 
    ecu_handle_startup_sequence(current_time_ms); 
    pedals_poll(current_time_ms); 
}

void ecu_process_message(uint32_t current_time_ms, CAN_message_t msg) {
    /* If we received a CAN message, update the corresponding value. */
    if ((MessageId)msg.id == MESSAGE_ID_INTERNAL_STATES) {
        /* To check VSM internal state. */
        VsmState vsm_state = parse_motor_internal_states(msg).vsm_state;
        precharge_complete =
            vsm_state == VSM_STATE_PRE_CHARGE_COMPLETE ||
            vsm_state == VSM_STATE_VSM_WAIT ||
            vsm_state == VSM_STATE_VSM_READY ||
            vsm_state == VSM_STATE_MOTOR_RUNNING;

            vsm_seen = true;
            vsm_last_rx_ms = current_time_ms;
    }
}

GpioState ecu_poll_gpio_state(uint32_t current_time_ms) {
    (void)current_time_ms;
    GpioState result = { .horn_on = horn_on, .brake_light_on = brake_light_on };
    return result;
}

bool ecu_poll_can(uint32_t current_time_ms, CAN_message_t *out) {
    /* The engine is enabled if all preconditions of `car_fully_on` passed and
     * the brake is up. We don't enable the inverter until the driver lifts
     * off the brake. */
    if (throttle_and_brake_pressed) {
        /* The driver previously pressed throttle and brake together; this
         * deals with re-enabling once the throttle is low enough again. */
        if (pedals_get_throttle_value() <= 5 && !pedals_is_brake_pressed()) {
            throttle_and_brake_pressed = false;
        }
    }
    /* Brake and throttle cannot be pressed at the same time. 2026 rules, EV.4.7. */
    if (mapped_throttle >= 25 && pedals_is_brake_pressed()) { 
        throttle_and_brake_pressed = true; 
    }


    bool inverter_enabled = false;
    int16_t torque_to_use = 0;

    if (car_fully_on && !throttle_and_brake_pressed) {
        inverter_enabled = true;
        torque_to_use = pedals_get_current_torque_amount();
    }

    /* Pace how often we send a motor command. We still send these when the
     * inverter is off, so the motor gets shutdown messages. */
    static bool last_inverter_enabled;
    bool force_send = last_inverter_enabled && !inverter_enabled;
    if (force_send || timer_should_fire(&ecu_motor_control_pacing, current_time_ms)) {
        last_inverter_enabled = inverter_enabled;

        MotorControlCommand cmd = {0};
        cmd.torque = torque_to_use;
        cmd.speed = 0;
        cmd.direction = MOTOR_DIRECTION_FORWARD;
        cmd.enable_inverter = inverter_enabled;
        cmd.inverter_discharge = false;
        cmd.override_speed = false;
        cmd.torque_limit = 0;

        *out = create_motor_control_command(cmd);
        return true;
    }

    /* No message generated; tell the caller to stop polling. */
    return false;
}

// Prints pedal state and values
void pedals_print_state(void) {
    PRINTF("Pedals:\n");
    PRINTF("  throttle_1_position: %u\n", throttle_1_position);
    PRINTF("  throttle_2_position: %u\n", throttle_2_position);
    PRINTF("  mapped_throttle: %u\n", mapped_throttle);
    PRINTF("  smoothed_throttle: %u\n", smoothed_throttle);
    PRINTF("  brake_position: %u\n", brake_position);
    PRINTF("  pedals_implausibility: %s\n", pedals_implausibility.active ? "yes" : "no");
}

// Prints ecu state and variables
void ecu_print_state(void) {
    PRINTF("Ecu:\n");
    PRINTF("  car_fully_on: %s\n", car_fully_on ? "yes" : "no");
    PRINTF("  start_switch_on: %s\n", start_switch_on ? "yes" : "no");
    PRINTF("  precharge_complete: %s\n", precharge_complete ? "yes" : "no");
    pedals_print_state();
}