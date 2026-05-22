#ifndef BME280_DRIVER_H
#define BME280_DRIVER_H

#include <stdint.h>

typedef void (*bme280_i2c_read)(uint8_t* buffer, uint16_t length);
typedef void (*bme280_i2c_write)(uint8_t* data, uint16_t size);

typedef struct {
    bme280_i2c_read i2c_read;
    bme280_i2c_write i2c_write;
} bme280_ctx_t;

void bme280_init(bme280_i2c_read i2c_read, bme280_i2c_write i2c_write);
void bme280_read_regs(uint8_t start_reg_address, uint8_t* buffer, uint8_t length);
void bme280_write_reg(uint8_t reg_address, uint8_t value);


int32_t bme280_read_temp_raw(void);
int32_t bme280_read_pres_raw(void);
int32_t bme280_read_hum_raw(void);

float bme280_read_temperature(void);
float bme280_read_pressure(void);
float bme280_read_humidity(void);

#endif 
