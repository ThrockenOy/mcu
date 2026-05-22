#pragma once

typedef enum
{
    ADC_TASK_STATE_IDLE = 0,
    ADC_TASK_STATE_RUN = 1,
} adc_task_state_t;



void adc_task_init(void);
float adc_task_read_voltage(void);
float adc_task_read_temperature(void);

void adc_task_handle(void);
void adc_task_set_state(adc_task_state_t new_state);

void get_adc_callback(const char* args);
void get_temp_callback(const char* args);

void tm_start_callback(const char* args);
void tm_stop_callback(const char* args);