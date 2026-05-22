#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "protocol-task.h"
#include "bme280-driver.h"
#include "stdio-task/stdio-task.h"
#include "led-task/led-task.h"

#define DEVICE_NAME  "RP2040 BME280 Device"
#define DEVICE_VRSN  "v1.0.0"

void version_callback(const char* args) {
    printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
}

void led_on_callback(const char* args) {
    led_task_state_set(LED_STATE_ON);
    printf("LED turned ON\n");
}

void led_off_callback(const char* args) {
    led_task_state_set(LED_STATE_OFF);
    printf("LED turned OFF\n");
}

void led_blink_callback(const char* args) {
    led_task_state_set(LED_STATE_BLINK);
    printf("LED blinking started\n");
}

void led_blink_set_period_ms_callback(const char* args) {
    uint period_ms = 0;
    sscanf(args, "%u", &period_ms);
    if (period_ms == 0) return;
    led_task_set_blink_period_ms(period_ms);
}

void help_callback(const char* args);
void mem_callback(const char* args);
void wmem_callback(const char* args);

void rp2040_i2c_read(uint8_t* buffer, uint16_t length) {
	i2c_read_timeout_us(i2c1, 0x76, buffer, length, false, 100000);
}

void rp2040_i2c_write(uint8_t* data, uint16_t size) {
	i2c_write_timeout_us(i2c1, 0x76, data, size, false, 100000);
}

void read_reg_callback(const char* args) {
    uint32_t addr = 0, N = 0;
    if (sscanf(args, "%x %x", &addr, &N) != 2) return;

    if (addr > 0xFF || N > 0xFF || (addr + N) > 0x100) return;

    uint8_t buffer[256] = {0};
    bme280_read_regs((uint8_t)addr, buffer, (uint8_t)N);

    for (int i = 0; i < N; i++) {
        printf("bme280 register [0x%X] = 0x%X\n", addr + i, buffer[i]);
    }
}

void write_reg_callback(const char* args) {
    uint32_t addr = 0, val = 0;
    if (sscanf(args, "%x %x", &addr, &val) != 2) return;
    if (addr > 0xFF || val > 0xFF) return;

    bme280_write_reg((uint8_t)addr, (uint8_t)val);
}

void temp_raw_callback(const char* args) { printf("%u\n", bme280_read_temp_raw()); }
void pres_raw_callback(const char* args) { printf("%u\n", bme280_read_pres_raw()); }
void hum_raw_callback(const char* args) { printf("%u\n", bme280_read_hum_raw()); }

void temp_callback(const char* args) { printf("%f\n", bme280_read_temperature()); }
void pres_callback(const char* args) { printf("%f\n", bme280_read_pressure()); }
void hum_callback(const char* args) { printf("%f\n", bme280_read_humidity()); }

void get_all_callback(const char* args) {
    printf("%f %f %f\n", bme280_read_temperature(), bme280_read_pressure(), bme280_read_humidity());
}

api_t device_api[] = {
    {"version", version_callback, "get device name and firmware version"},
    {"on", led_on_callback, "turn LED on"},
    {"off", led_off_callback, "turn LED off"},
    {"blink", led_blink_callback, "make LED blink"},
    {"set_period", led_blink_set_period_ms_callback, "set blink period in milliseconds"},
    {"help", help_callback, "print commands description"},
    {"mem", mem_callback, "read memory: mem <hex_addr>"},
    {"wmem", wmem_callback, "write memory: wmem <hex_addr> <hex_val>"},
    {"read_reg", read_reg_callback, "read N registers of bme280: read_reg <hex_addr> <count>"},
    {"write_reg", write_reg_callback, "write to bme280 register: write_reg <hex_addr> <hex_val>"},
    {"temp_raw", temp_raw_callback, "get raw temperature counts from BME280"},
    {"pres_raw", pres_raw_callback, "get raw pressure counts from BME280"},
    {"hum_raw", hum_raw_callback, "get raw humidity counts from BME280"},
    {"temp", temp_callback, "get temperature in Celsius"},
    {"pres", pres_callback, "get pressure in Pascals"},
    {"hum", hum_callback, "get humidity in %"},
    {"get_all", get_all_callback, "get temperature, pressure and humidity in one line"},
    {NULL, NULL, NULL},  
};

void help_callback(const char* args) {
    int i = 0;

    while (device_api[i].command_name != NULL) {
        printf(" '%s': '%s'\n",
                device_api[i].command_name,
                device_api[i].command_help);
        i++;
    }
}


void mem_callback(const char* args) {
    uint32_t addr = 0;
    if (sscanf(args, "%x", &addr) != 1) return;
    printf("Memory at 0x%08X: 0x%08X\n", addr, *(volatile uint32_t*)addr);
}

void wmem_callback(const char* args) {
    uint32_t addr = 0, value = 0;
    if (sscanf(args, "%x %x", &addr, &value) != 2) return;
    *(volatile uint32_t*)addr = value;
}

int main() {
    i2c_init(i2c1, 100000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);

    bme280_init(rp2040_i2c_read, rp2040_i2c_write);

    stdio_task_init();
    protocol_task_init(device_api);
    led_task_init();

    while (1) {
        char* command_string = stdio_task_handle();
        protocol_task_handle(command_string);
        led_task_handle();
    }
    return 0;
}
