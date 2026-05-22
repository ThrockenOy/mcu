#include "adc-task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

const uint adc_pin = 26;
const uint adc_channel = 0;
const uint adc_temperature = 4;

// Переменные для отсчета времени и хранения состояния
static adc_task_state_t adc_state = ADC_TASK_STATE_IDLE;
static uint64_t last_meas_time_us = 0;

// Период измерений 100 мс
const uint64_t ADC_TASK_MEAS_PERIOD_US = 100000;

void adc_task_init(void) {
    adc_init();
    adc_gpio_init(adc_pin);
    adc_set_temp_sensor_enabled(true);
}

float adc_task_read_voltage(void) {
    adc_select_input(adc_channel);

    uint16_t voltage_counts = adc_read();

    float voltage_V = (voltage_counts * 3.3f) / 4096.0f;

    return voltage_V;
}

float adc_task_read_temperature(void) {
    adc_select_input(adc_temperature);

    uint16_t temp_counts = adc_read();

    float temp_V = (temp_counts * 3.3f) / 4096.0f;  
    
    float temp_C = 27.0f - (temp_V - 0.706f) / 0.001721f;

    return temp_C;

}

void adc_task_set_state(adc_task_state_t new_state) {
    adc_state = new_state;
    if (new_state == ADC_TASK_STATE_RUN) {
        last_meas_time_us = time_us_64();
    }
}

// Фоновый обработчик 
void adc_task_handle(void) {
    if (adc_state != ADC_TASK_STATE_RUN) {
        return; // Если выключено, ничего
    }

    uint64_t current_time_us = time_us_64();

    // Проверяем, прошло ли 100 мс
    if (current_time_us - last_meas_time_us >= ADC_TASK_MEAS_PERIOD_US) {
        last_meas_time_us = current_time_us;

        float voltage_V = adc_task_read_voltage();
        float temp_C = adc_task_read_temperature();

        // Выводим U и t
        printf("%f %f\n", voltage_V, temp_C);
    }
}