#include "util.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1;

#define ADC_MAX_VALUE 4095
#define ADC_VOLTAGE_REF 3300
#define ADC_PERCENTAGE_REF 100
#define ADC_CHANNEL_COUNT 3

enum { IDX_THROTTLE_1 = 0, IDX_THROTTLE_2 = 1, IDX_BRAKE = 2 };

static uint16_t adc_buf[ADC_CHANNEL_COUNT];

bool adc_inputs_start(void) {
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) return false;
    return HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, ADC_CHANNEL_COUNT) == HAL_OK;
}

/* The DMA updates the buffer in the background, so read through a volatile pointer. */
static uint16_t adc_get(int i) { return *(volatile uint16_t *)&adc_buf[i]; }

uint16_t read_throttle_1(void) { return adc_get(IDX_THROTTLE_1); }
uint16_t read_throttle_2(void) { return adc_get(IDX_THROTTLE_2); }
uint16_t read_brake(void)      { return adc_get(IDX_BRAKE); }

uint16_t adc_convert_to_percent(uint16_t raw_value) {
    if (raw_value > ADC_MAX_VALUE) {
        return ADC_PERCENTAGE_REF;
    } else {
        return (raw_value / ADC_MAX_VALUE) * ADC_PERCENTAGE_REF;
    }
}

uint16_t adc_convert_to_voltage(uint16_t raw_value) {
    if (raw_value > ADC_MAX_VALUE) {
        return ADC_VOLTAGE_REF;
    } else {
        return (raw_value / ADC_MAX_VALUE) * ADC_VOLTAGE_REF;
    }
}
