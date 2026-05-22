#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "stdio-task/stdio-task.h"
#include "led-task/led-task.h"
#include "protocol-task.h"
#include "ili9341-driver.h"
#include "ili9341-display.h"
#include "ili9341-font.h"


#define DEVICE_NAME  "RP2040 Control Device"
#define DEVICE_VRSN  "v1.0.0"

#define ILI9341_PIN_MISO 4
#define ILI9341_PIN_CS 10
#define ILI9341_PIN_SCK 6
#define ILI9341_PIN_MOSI 7
#define ILI9341_PIN_DC 8
#define ILI9341_PIN_RESET 9

static ili9341_display_t ili9341_display = {0};

void rp2040_spi_write(const uint8_t *data, uint32_t size)
{
    spi_write_blocking(spi0, data, size);
}

void rp2040_spi_read(uint8_t *buffer, uint32_t length)
{
    spi_read_blocking(spi0, 0, buffer, length);
}

void rp2040_gpio_cs_write(bool level)
{
    gpio_put(ILI9341_PIN_CS, level);
}

void rp2040_gpio_dc_write(bool level)
{
    gpio_put(ILI9341_PIN_DC, level);
}

void rp2040_gpio_reset_write(bool level)
{
    gpio_put(ILI9341_PIN_RESET, level);
}

void rp2040_delay_ms(uint32_t ms)
{
    sleep_ms(ms);
}

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


void disp_screen_callback(const char* args)
{
    uint32_t c = 0;
    int result = sscanf(args, "%x", &c);
    uint16_t color = COLOR_BLACK;
    if (result == 1)
    {
        color = RGB888_2_RGB565(c);
    }
    ili9341_fill_screen(&ili9341_display, color);
}

void disp_px_callback(const char* args)
{
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t c = 0;
    int result = sscanf(args, "%u %u %x", &x, &y, &c);
    if (result == 3)
    {
        uint16_t color = RGB888_2_RGB565(c);
        ili9341_draw_pixel(&ili9341_display, (uint16_t)x, (uint16_t)y, color);
    }
}

void disp_line_callback(const char* args)
{
    uint32_t x1 = 0, y1 = 0, x2 = 0, y2 = 0, c = 0;
    int result = sscanf(args, "%u %u %u %u %x", &x1, &y1, &x2, &y2, &c);
    if (result == 5)
    {
        ili9341_draw_line(&ili9341_display, x1, y1, x2, y2, RGB888_2_RGB565(c));
    }
}

void disp_rect_callback(const char* args)
{
    uint32_t x = 0, y = 0, w = 0, h = 0, c = 0;
    int result = sscanf(args, "%u %u %u %u %x", &x, &y, &w, &h, &c);
    if (result == 5)
    {
        ili9341_draw_rect(&ili9341_display, x, y, w, h, RGB888_2_RGB565(c));
    }
}

void disp_frect_callback(const char* args)
{
    uint32_t x = 0, y = 0, w = 0, h = 0, c = 0;
    int result = sscanf(args, "%u %u %u %u %x", &x, &y, &w, &h, &c);
    if (result == 5)
    {
        ili9341_draw_filled_rect(&ili9341_display, x, y, w, h, RGB888_2_RGB565(c));
    }
}

void disp_text_callback(const char* args)
{
    uint32_t x = 0, y = 0, tc = 0, bg = 0;
    char text_buf[64] = {0};
    int result = sscanf(args, "%u %u %x %x %[^\r\n]", &x, &y, &tc, &bg, text_buf);
    if (result == 5)
    {
        uint16_t text_color = RGB888_2_RGB565(tc);
        uint16_t bg_color = RGB888_2_RGB565(bg);
        ili9341_draw_text(&ili9341_display, (uint16_t)x, (uint16_t)y, text_buf, &jetbrains_font, text_color, bg_color);
    }
}

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
    {"disp_screen", disp_screen_callback, "fill entire screen with HEX color: disp_screen <RRGGBB>"},
    {"disp_px", disp_px_callback, "draw single pixel: disp_px <x> <y> <RRGGBB>"},
    {"disp_line", disp_line_callback, "draw line: disp_line <x1> <y1> <x2> <y2> <RRGGBB>"},
    {"disp_rect", disp_rect_callback, "draw hollow rectangle: disp_rect <x> <y> <w> <h> <RRGGBB>"},
    {"disp_frect", disp_frect_callback, "draw filled rectangle: disp_frect <x> <y> <w> <h> <RRGGBB>"},
    {"disp_text", disp_text_callback, "draw text string: disp_text <x> <y> <text_color_hex> <bg_color_hex> <text>"},
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

    spi_init(spi0, 62500000);
    gpio_set_function(ILI9341_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341_PIN_SCK, GPIO_FUNC_SPI);

    gpio_init(ILI9341_PIN_CS);
    gpio_set_dir(ILI9341_PIN_CS, GPIO_OUT);
    gpio_init(ILI9341_PIN_DC);
    gpio_set_dir(ILI9341_PIN_DC, GPIO_OUT);
    gpio_init(ILI9341_PIN_RESET);
    gpio_set_dir(ILI9341_PIN_RESET, GPIO_OUT);

    gpio_put(ILI9341_PIN_CS, true);
    gpio_put(ILI9341_PIN_DC, false);
    gpio_put(ILI9341_PIN_RESET, false);

    ili9341_hal_t ili9341_hal = {0};
    ili9341_hal.spi_write = rp2040_spi_write;
    ili9341_hal.spi_read = rp2040_spi_read;
    ili9341_hal.gpio_cs_write = rp2040_gpio_cs_write;
    ili9341_hal.gpio_dc_write = rp2040_gpio_dc_write;
    ili9341_hal.gpio_reset_write = rp2040_gpio_reset_write;
    ili9341_hal.delay_ms = rp2040_delay_ms;

    ili9341_init(&ili9341_display, &ili9341_hal);
    ili9341_set_rotation(&ili9341_display, ILI9341_ROTATION_90);

    ili9341_fill_screen(&ili9341_display, COLOR_BLACK);
    sleep_ms(300);

    ili9341_draw_filled_rect(&ili9341_display, 10, 10, 100, 60, COLOR_RED);
    ili9341_draw_filled_rect(&ili9341_display, 120, 10, 100, 60, COLOR_GREEN);
    ili9341_draw_filled_rect(&ili9341_display, 230, 10, 80, 60, COLOR_BLUE);

    ili9341_draw_rect(&ili9341_display, 10, 90, 300, 80, COLOR_WHITE);

    ili9341_draw_line(&ili9341_display, 0, 0, 319, 239, COLOR_YELLOW);
    ili9341_draw_line(&ili9341_display, 319, 0, 0, 239, COLOR_CYAN);

    ili9341_draw_text(&ili9341_display, 20, 100, "Hello, ILI9341!", &jetbrains_font, COLOR_WHITE, COLOR_BLACK);
    ili9341_draw_text(&ili9341_display, 20, 116, "RP2040 / Pico SDK", &jetbrains_font, COLOR_YELLOW, COLOR_BLACK);

    while (1)
    {
        char* command_string = stdio_task_handle();
        protocol_task_handle(command_string);
        led_task_handle();
    }

    return 0;
}
