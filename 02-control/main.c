#include <stdio.h>
#include "pico/stdlib.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"

#define DEVICE_NAME  "RP2040 Control Device"
#define DEVICE_VRSN  "v1.0.0"

void version_callback(const char* args)
{
    printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
}

void led_on_callback(const char* args)
{
    led_task_state_set(LED_STATE_ON);
    printf("LED turned ON\n");
}

void led_off_callback(const char* args)
{
    led_task_state_set(LED_STATE_OFF);
    printf("LED turned OFF\n");
}

void led_blink_callback(const char* args)
{
    led_task_state_set(LED_STATE_BLINK);
    printf("LED blinking started\n");
}

void led_blink_set_period_ms_callback(const char* args)
{
    uint period_ms = 0;
    sscanf(args, "%u", &period_ms);

    if (period_ms == 0)
    {
        printf("Error: invalid period value. Usage: set_period <milliseconds>\n");
        return;
    }

    led_task_set_blink_period_ms(period_ms);
    printf("LED blink period set to %u ms\n", period_ms);
}


void help_callback(const char* args);

void mem_callback(const char* args);
void wmem_callback(const char* args);

api_t device_api[] =
{
    {"version", version_callback, "get device name and firmware version"},
    {"on", led_on_callback, "turn LED on"},
    {"off", led_off_callback, "turn LED off"},
    {"blink", led_blink_callback, "make LED blink"},
    {"set_period", led_blink_set_period_ms_callback, "set blink period in milliseconds"},
    {"help", help_callback, "print commands description"},
    {"mem", mem_callback, "read memory: mem <hex_addr>"},
    {"wmem", wmem_callback, "write memory: wmem <hex_addr> <hex_val>"},
    {NULL, NULL, NULL},  // Маркер конца массива
};

void help_callback(const char* args) {
    int i = 0;

    while (device_api[i].command_name != NULL) {
        printf("Команда '%s': '%s'\n",
                device_api[i].command_name,
                device_api[i].command_help);
        i++;
    }
}

void mem_callback(const char* args)
{
    uint32_t addr = 0;
    if (sscanf(args, "%x", &addr) != 1) {
        printf("Usage: mem <hex_address>\n");
        return;
    }

    uint32_t value = *(volatile uint32_t*)addr;

    printf("Memory at 0x%08X: 0x%08X\n", addr, value);
}


void wmem_callback(const char* args)
{
    uint32_t addr = 0;
    uint32_t value = 0;


    if (sscanf(args, "%x %x", &addr, &value) != 2) {
        printf("Usage: wmem <hex_address> <hex_value>\n");
        return;
    }

    *(volatile uint32_t*)addr = value;

    printf("Wrote 0x%08X to 0x%08X\n", value, addr);
}

int main()
{
    stdio_task_init();
    protocol_task_init(device_api);
    led_task_init();

    while (1)
    {
        char* command_string = stdio_task_handle();
        protocol_task_handle(command_string);
        led_task_handle();
    }

    return 0;
}


