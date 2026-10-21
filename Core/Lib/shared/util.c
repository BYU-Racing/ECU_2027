#include "util.h"
#include "main.h"

// ADC Drivers
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

#define ADC_MAX_VALUE 4095u
#define ADC_VOLTAGE_REF 3300u
#define ADC_PERCENTAGE_REF 100u

#define ADC1_CHANNEL_COUNT 2
#define ADC2_CHANNEL_COUNT 1

enum { IDX_THROTTLE_1 = 0, IDX_THROTTLE_2 = 1};

static uint16_t adc_buf[ADC1_CHANNEL_COUNT];

bool adc_inputs_start(void) {
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
        return false;
    }

    if (HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) != HAL_OK) {
        return false;
    }

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, ADC1_CHANNEL_COUNT) != HAL_OK) {
        return false;
    }

    if (HAL_ADC_Start(&hadc2) != HAL_OK) {
        return false;
    }

    return true;
}

/* The DMA updates the buffer in the background, so read through a volatile pointer. */
static uint16_t adc_get(int i) { 
    return *(volatile uint16_t *)&adc_buf[i]; 
}

uint16_t read_throttle_1(void) {
    return adc_get(IDX_THROTTLE_1); 
}

uint16_t read_throttle_2(void) { 
    return adc_get(IDX_THROTTLE_2); 
}
uint16_t read_brake(void)      {
    return (uint16_t) HAL_ADC_GetValue(&hadc2);
}

// Returns the percantage in 0-100, given the raw ADC value.
uint16_t adc_convert_to_percent(uint16_t raw_value) {
    if (raw_value > ADC_MAX_VALUE) {
        return ADC_PERCENTAGE_REF;
    } else {
        return (raw_value * ADC_PERCENTAGE_REF) / ADC_MAX_VALUE;
    }
}

// Returns the voltage in millivolts, given the raw ADC value.
uint16_t adc_convert_to_voltage(uint16_t raw_value) {
    if (raw_value > ADC_MAX_VALUE) {
        return ADC_VOLTAGE_REF;
    } else {
        return (raw_value * ADC_VOLTAGE_REF) / ADC_MAX_VALUE;
    }
}

// Trigger
void trigger_start(Trigger *trigger, uint32_t current_time_ms, uint32_t target_duration){
    trigger->target_duration = target_duration;
    trigger->started_at = current_time_ms;
    trigger->active = true;
}

void trigger_start_if_stopped(Trigger *trigger, uint32_t current_time_ms, uint32_t target_duration) {
    if (!trigger->active) {
        trigger_start(trigger, current_time_ms, target_duration);
    }
}

bool trigger_started(const Trigger *trigger) {
    return trigger->active;
}

/* Makes it so the next time trigger_reached is called, it returns true. */
void prime_trigger(Trigger *trigger, uint32_t current_time_ms) {
    trigger_start(trigger, current_time_ms, 0);
}

bool trigger_reached(Trigger *trigger, uint32_t current_time_ms) {
   if (!trigger->active) {
        return false;
    }
    if ((current_time_ms - trigger->started_at) >= trigger->target_duration){      
        /* Reset the trigger, so that trigger_started is false. */
        trigger->active = false;
        return true;
    }
    return false;
}

void trigger_cancel(Trigger *trigger) {
    trigger->active = false;
}


/* Timer */
void timer_start(Timer *timer, uint32_t current_time, uint32_t duration_ms){
    timer->last_fired_at = current_time;
    timer->duration_ms = duration_ms;
}

/* Important: this will reset the timer when this returns true. */
bool timer_should_fire(Timer *timer, uint32_t current_time) {
    if ((current_time - timer->last_fired_at) >= timer->duration_ms) {
        timer->last_fired_at = current_time;
        return true;
    } else {
        return false;
    }
}

/* Tells us how long until this timer will fire again, or 0 if
 * it's due to fire. */
uint32_t timer_time_until_next_firing(const Timer *timer, uint32_t current_time_ms) {
    if (current_time_ms - timer->last_fired_at < timer->duration_ms) {
        return timer->duration_ms - (current_time_ms - timer->last_fired_at);
    } else {
        return 0;
    }
}

/* Pid */
void pid_init(Pid *pid, uint32_t current_time_ms, double p_term, double i_term, double d_term, double windup_cap){
    pid->last_time_ms = current_time_ms;
    pid->p_term = p_term;
    pid->i_term = i_term;
    pid->d_term = d_term;
    pid->windup_cap = windup_cap;
    pid->last_error = 0.0;
    pid->integral = 0.0;
}

double pid_next_value(Pid *pid, uint32_t current_time_ms, double target, double reading) {
    /* Convert the difference (in milliseconds) to seconds. */
    double delta_time = (double)(current_time_ms - pid->last_time_ms) / 1000.0;
    double error = target - reading;

    double p = error;

    pid->integral += error * delta_time;
    /* Prevent the integral from winding up too much. */
    if (pid->integral > pid->windup_cap) pid->integral = pid->windup_cap;
    if (pid->integral < -pid->windup_cap) pid->integral = -pid->windup_cap;
    double i = pid->integral;

    double d;
    if (delta_time == 0) {
        /* Avoid dividing by 0. */
        d = 0.0;
    } else {
        d = (error - pid->last_error) / delta_time;
    }
    pid->last_error = error;

    pid->last_time_ms = current_time_ms;

    return p * pid->p_term + i * pid->i_term + d * pid->d_term;
}

/* Used to condense the file name into something short enough we can send
 * along the CAN bus in the case of a fault. */
uint32_t str_hash(const char *str) {
    /* djb2 hash. */
    uint32_t hash = 5381;
    uint32_t c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/* Generic function to remap one numerical range to another. */
int64_t map_range(int32_t input_32, int32_t old_min_32, int32_t old_max_32, int32_t new_min_32, int32_t new_max_32) {
    if (old_min_32 == old_max_32) {
         return new_min_32; 
    }

    /* Widen all values to 64-bit ints to avoid any issues with integer overflow. */
    int64_t input = input_32;
    int64_t old_min = old_min_32;
    int64_t old_max = old_max_32;
    int64_t new_min = new_min_32;
    int64_t new_max = new_max_32;

    int64_t old_difference = old_max - old_min;
    int64_t new_difference = new_max - new_min;

    int64_t input_at_origin = input - old_min;
    /* Why not `(input_at_origin / old_difference) * new_difference`? Because
     * integer division floors, so we'd lose a lot of precision. */
    int64_t output_at_origin = (input_at_origin * new_difference) / old_difference;
    int64_t output_shifted = output_at_origin + new_min;

    return output_shifted;
}

