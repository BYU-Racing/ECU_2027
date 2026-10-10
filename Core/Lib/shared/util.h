#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool     adc_inputs_start(void);

static uint16_t adc_get(int i);

uint16_t read_throttle_1(void);
uint16_t read_throttle_2(void);
uint16_t read_brake(void);
uint16_t adc_convert_to_percent(uint16_t raw_value);
uint16_t adc_convert_to_voltage(uint16_t raw_value);

#ifdef __cplusplus
}
#endif