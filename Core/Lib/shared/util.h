#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ADC Drivers
bool     adc_inputs_start(void);
uint16_t read_throttle_1(void);
uint16_t read_throttle_2(void);
uint16_t read_brake(void);
uint16_t adc_convert_to_percent(uint16_t raw_value);
uint16_t adc_convert_to_voltage(uint16_t raw_value);

// Triggers
typedef struct {
    bool     active;
    uint32_t started_at;
    uint32_t target_duration;
} Trigger;
void trigger_start(Trigger *trigger, uint32_t current_time_ms, uint32_t target_duration);
void trigger_start_if_stopped(Trigger *trigger, uint32_t current_time_ms, uint32_t target_duration);
bool trigger_started(const Trigger *trigger);
void prime_trigger(Trigger *trigger, uint32_t current_time_ms);
bool trigger_reached(Trigger *trigger, uint32_t current_time_ms);
void trigger_cancel(Trigger *trigger);

// Timer
typedef struct {
    uint32_t last_fired_at;
    uint32_t duration_ms;
} Timer;
void timer_start(Timer *timer, uint32_t current_time, uint32_t duration_ms);
bool timer_should_fire(Timer *timer, uint32_t current_time);
uint32_t timer_time_until_next_firing(const Timer *timer, uint32_t current_time_ms);

// PID (proportional integral derivative)
/* A classic PID implementation, with an additional windup cap. The windup
    * cap prevents both positive and negative windup. The cap itself should be positive. */
typedef struct {
    double p_term;
    double i_term;
    double d_term;
    double windup_cap;
    double last_error;
    double integral;
    uint32_t last_time_ms;
} Pid;
void pid_init(Pid *pid, uint32_t current_time_ms, double pid_p_term, double pid_i_term, double pid_d_term, double pid_windup_cap);
double pid_next_value(Pid *pid, uint32_t current_time_ms, double target, double reading);

// Utilities
//TO DO: implement later
// #define PRINTF(...) printf(__VA_ARGS__)

#define UNUSED(x) ((void)(x))

uint32_t str_hash(const char *str);
int64_t map_range(int32_t input_32, int32_t old_min_32, int32_t old_max_32, int32_t new_min_32, int32_t new_max_32);

#ifdef __cplusplus
}
#endif