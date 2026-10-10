#include "util.h"
#include "main.h"

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
