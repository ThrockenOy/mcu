#include "bme280-driver.h"
#include "bme280-regs.h"
#include <stdio.h>

static bme280_ctx_t bme280_ctx = {0};

static struct {
    uint16_t dig_T1; int16_t dig_T2; int16_t dig_T3;
    uint16_t dig_P1; int16_t dig_P2; int16_t dig_P3; int16_t dig_P4;
    int16_t  dig_P5; int16_t dig_P6; int16_t dig_P7; int16_t dig_P8; int16_t dig_P9;
    uint8_t  dig_H1; int16_t dig_H2; uint8_t  dig_H3; int16_t dig_H4; int16_t dig_H5; int8_t dig_H6;
} calib;

static int32_t t_fine;

static void read_calibration_data(void) {
    uint8_t b1[26] = {0};
    uint8_t b2[8] = {0};
    
    bme280_read_regs(BME280_REG_CALIB00, b1, 26);
    
    calib.dig_T1 = (uint16_t)((b1[1] << 8) | b1[0]);
    calib.dig_T2 = (int16_t)((b1[3] << 8) | b1[2]);
    calib.dig_T3 = (int16_t)((b1[5] << 8) | b1[4]);
    
    calib.dig_P1 = (uint16_t)((b1[7] << 8) | b1[6]);
    calib.dig_P2 = (int16_t)((b1[9] << 8) | b1[8]);
    calib.dig_P3 = (int16_t)((b1[11] << 8) | b1[10]);
    calib.dig_P4 = (int16_t)((b1[13] << 8) | b1[12]);
    calib.dig_P5 = (int16_t)((b1[15] << 8) | b1[14]);
    calib.dig_P6 = (int16_t)((b1[17] << 8) | b1[16]);
    calib.dig_P7 = (int16_t)((b1[19] << 8) | b1[18]);
    calib.dig_P8 = (int16_t)((b1[21] << 8) | b1[20]);
    calib.dig_P9 = (int16_t)((b1[23] << 8) | b1[22]);
    
    calib.dig_H1 = b1[25];
    
    bme280_read_regs(BME280_REG_CALIB26, b2, 7);
    calib.dig_H2 = (int16_t)((b2[1] << 8) | b2[0]);
    calib.dig_H3 = b2[2];
    calib.dig_H4 = (int16_t)((b2[3] << 4) | (b2[4] & 0x0F));
    calib.dig_H5 = (int16_t)((b2[5] << 4) | (b2[4] >> 4));
    calib.dig_H6 = (int8_t)b2[6];
}

void bme280_init(bme280_i2c_read i2c_read, bme280_i2c_write i2c_write) {
    bme280_ctx.i2c_read = i2c_read;
    bme280_ctx.i2c_write = i2c_write;

    uint8_t id_reg_buf[1] = {0};
    bme280_read_regs(BME280_REG_ID, id_reg_buf, sizeof(id_reg_buf));
    
    if (id_reg_buf[0] != 0x60) {
        return;
    }

    read_calibration_data();

    bme280_write_reg(BME280_REG_RESET, 0xB6);
    for (volatile int i = 0; i < 100000; i++);

    uint8_t ctrl_hum_reg_value = 0b001;
    bme280_write_reg(BME280_REG_CTRL_HUM, ctrl_hum_reg_value);
    
    for (volatile int i = 0; i < 10000; i++);

    uint8_t config_reg_value = 0b00000000;
    bme280_write_reg(BME280_REG_CONFIG, config_reg_value);

    uint8_t ctrl_meas_reg_value = 0b00100111;
    bme280_write_reg(BME280_REG_CTRL_MEAS, ctrl_meas_reg_value);
}

void bme280_read_regs(uint8_t start_reg_address, uint8_t* buffer, uint8_t length) {
    uint8_t data[1] = {start_reg_address};
    bme280_ctx.i2c_write(data, sizeof(data));
    bme280_ctx.i2c_read(buffer, length);
}

void bme280_write_reg(uint8_t reg_address, uint8_t value) {
    uint8_t data[2] = {reg_address, value};
    bme280_ctx.i2c_write(data, sizeof(data));
}

int32_t bme280_read_temp_raw(void) {
    uint8_t read[3] = {0};
    bme280_read_regs(BME280_REG_TEMP_MSB, read, 3);
    return ((int32_t)read[0] << 12) | ((int32_t)read[1] << 4) | ((int32_t)read[2] >> 4);
}

int32_t bme280_read_pres_raw(void) {
    uint8_t read[3] = {0};
    bme280_read_regs(BME280_REG_PRESS_MSB, read, 3);
    return ((int32_t)read[0] << 12) | ((int32_t)read[1] << 4) | ((int32_t)read[2] >> 4);
}

int32_t bme280_read_hum_raw(void) {
    uint8_t read[2] = {0};
    bme280_read_regs(BME280_REG_HUM_MSB, read, 2);
    return ((int32_t)read[0] << 8) | (int32_t)read[1];
}

float bme280_read_temperature(void) {
    int32_t adc_T = bme280_read_temp_raw(); 
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) * ((int32_t)calib.dig_T3)) >> 14;
    t_fine = var1 + var2;
    return (float)((t_fine * 5 + 128) >> 8) / 100.0f;
}

float bme280_read_pressure(void) {
    bme280_read_temperature();
    int32_t adc_P = bme280_read_pres_raw();
    
    int64_t var1, var2, p;
    
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;
    
    if (var1 == 0) return 0.0f;
    
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    
    return (float)p / 256.0f;
}

float bme280_read_humidity(void) {
    bme280_read_temperature();
    int32_t adc_H = bme280_read_hum_raw();
    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib.dig_H4) << 20) - (((int32_t)calib.dig_H5) * v_x1_u32r)) +
                   ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)calib.dig_H6)) >> 10) *
                                                   (((v_x1_u32r * ((int32_t)calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                                                 ((int32_t)2097152)) * ((int32_t)calib.dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calib.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (float)(v_x1_u32r >> 12) / 1024.0f;
}