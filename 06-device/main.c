#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"
#include "adc-task/adc-task.h"
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

#define BME280_I2C_SDA_PIN 18
#define BME280_I2C_SCL_PIN 19
#define BME280_ADDR 0x76

static ili9341_display_t ili9341_display = {0};

static uint32_t measurement_period_ms = 1000;
static uint32_t last_measurement_time = 0;
static float current_temperature = 0.0f;
static float last_temperature = 0.0f;
static float temp_history[60] = {0};
static int history_index = 0;
static bool history_ready = false;

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

void bme280_init_hw(void)
{
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(BME280_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BME280_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(BME280_I2C_SDA_PIN);
    gpio_pull_up(BME280_I2C_SCL_PIN);
}

float bme280_read_temperature(void)
{
    uint8_t reg = 0xFA;
    uint8_t data[3] = {0};
    if (i2c_write_blocking(i2c1, BME280_ADDR, &reg, 1, true) >= 0) {
        i2c_read_blocking(i2c1, BME280_ADDR, data, 3, false);
    }
    static float mock_t = 24.5f;
    mock_t += ((float)(to_ms_since_boot(get_absolute_time()) % 7) - 3.0f) * 0.05f;
    if (mock_t > 40.0f) mock_t = 20.0f;
    if (mock_t < 10.0f) mock_t = 25.0f;
    return mock_t;
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

void get_adc_callback(const char* args)
{
    float voltage_V = adc_task_read_voltage();
    printf("%f\n", voltage_V);
}

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
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t tc = 0;
    uint32_t bg = 0;
    char text_buf[128] = {0};
    int result = sscanf(args, "%u %u %x %x %[^\r\n]", &x, &y, &tc, &bg, text_buf);
    if (result == 5)
    {
        uint16_t text_color = RGB888_2_RGB565(tc);
        uint16_t bg_color = RGB888_2_RGB565(bg);
        ili9341_draw_text(&ili9341_display, (uint16_t)x, (uint16_t)y, text_buf, &jetbrains_font, text_color, bg_color);
    }
}

void set_meas_period_callback(const char* args)
{
    uint32_t p = 0;
    if (sscanf(args, "%u", &p) == 1 && p >= 100) {
        measurement_period_ms = p;
        printf("Measurement period set to %u ms\n", measurement_period_ms);
    } else {
        printf("Usage: set_meas_period <ms>\n");
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
    {"get_adc", get_adc_callback, "measure voltage on GPIO 26"},
    {"disp_screen", disp_screen_callback, ""},
    {"disp_px", disp_px_callback, ""},
    {"disp_line", disp_line_callback, ""},
    {"disp_rect", disp_rect_callback, ""},
    {"disp_frect", disp_frect_callback, ""},
    {"disp_text", disp_text_callback, ""},
    {"set_meas_period", set_meas_period_callback, "set sensor poll rate in ms"},
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

void draw_gui_base(void)
{
    ili9341_fill_screen(&ili9341_display, COLOR_BLACK);
    ili9341_draw_rect(&ili9341_display, 5, 5, 310, 230, COLOR_WHITE);
    ili9341_draw_text(&ili9341_display, 15, 15, "WEATHER STATION", &jetbrains_font, COLOR_CYAN, COLOR_BLACK);
    ili9341_draw_line(&ili9341_display, 5, 32, 314, 32, COLOR_CYAN);
    ili9341_draw_text(&ili9341_display, 15, 45, "Temp:", &jetbrains_font, COLOR_WHITE, COLOR_BLACK);
    ili9341_draw_rect(&ili9341_display, 15, 150, 290, 75, COLOR_YELLOW);
}

void update_gui_data(float temp)
{
    char buf[32];
    sprintf(buf, "%2.1f C  ", temp);
    ili9341_draw_text(&ili9341_display, 70, 45, buf, &jetbrains_font, COLOR_GREEN, COLOR_BLACK);

    uint16_t bar_width = (uint16_t)((temp - 10.0f) * (200.0f / 30.0f));
    if (bar_width > 200) bar_width = 200;
    if (temp < 10.0f) bar_width = 0;

    ili9341_draw_filled_rect(&ili9341_display, 70, 75, bar_width, 15, COLOR_RED);
    ili9341_draw_filled_rect(&ili9341_display, (uint16_t)(70 + bar_width), 75, (uint16_t)(200 - bar_width), 15, COLOR_BLACK);
    ili9341_draw_rect(&ili9341_display, 70, 75, 200, 15, COLOR_WHITE);

    if (temp > last_temperature) {
        ili9341_draw_text(&ili9341_display, 180, 45, "TREND: UP   ^", &jetbrains_font, COLOR_RED, COLOR_BLACK);
    } else if (temp < last_temperature) {
        ili9341_draw_text(&ili9341_display, 180, 45, "TREND: DOWN v", &jetbrains_font, COLOR_BLUE, COLOR_BLACK);
    } else {
        ili9341_draw_text(&ili9341_display, 180, 45, "TREND: STBL =", &jetbrains_font, COLOR_WHITE, COLOR_BLACK);
    }

    ili9341_draw_filled_rect(&ili9341_display, 16, 151, 288, 73, COLOR_BLACK);
    int count = history_ready ? 60 : history_index;
    if (count > 1) {
        for (int i = 0; i < count - 1; i++) {
            int idx1 = history_ready ? (history_index + i) % 60 : i;
            int idx2 = history_ready ? (history_index + i + 1) % 60 : i + 1;
            uint16_t x1 = (uint16_t)(16 + (i * 288 / 60));
            uint16_t x2 = (uint16_t)(16 + ((i + 1) * 288 / 60));
            uint16_t y1 = (uint16_t)(220 - ((temp_history[idx1] - 10.0f) * (70.0f / 30.0f)));
            uint16_t y2 = (uint16_t)(220 - ((temp_history[idx2] - 10.0f) * (70.0f / 30.0f)));
            if (y1 < 152) y1 = 152; if (y1 > 223) y1 = 223;
            if (y2 < 152) y2 = 152; if (y2 > 223) y2 = 223;
            ili9341_draw_line(&ili9341_display, x1, y1, x2, y2, COLOR_GREEN);
        }
    }
}

int main()
{
    stdio_task_init();
    protocol_task_init(device_api);
    led_task_init();
    adc_task_init();

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

    bme280_init_hw();
    draw_gui_base();
    last_measurement_time = to_ms_since_boot(get_absolute_time());

    while (1)
    {
        char* command_string = stdio_task_handle();
        protocol_task_handle(command_string);
        led_task_handle();
        adc_task_handle();

        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_measurement_time >= measurement_period_ms) {
            last_measurement_time = now;
            last_temperature = current_temperature;
            current_temperature = bme280_read_temperature();

            temp_history[history_index] = current_temperature;
            history_index++;
            if (history_index >= 60) {
                history_index = 0;
                history_ready = true;
            }
            update_gui_data(current_temperature);
        }
    }

    return 0;
}
