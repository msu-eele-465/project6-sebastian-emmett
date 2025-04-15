#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>
#include <stdbool.h>

#define RTC_ADDR    0x68
#define LM92_ADDR   0x48
#define SLAVE1_ADDR 0x49
#define SLAVE2_ADDR 0x50

void i2c_master_init(void);
void i2c_slave_init(uint8_t address);
void i2c_send(uint8_t slave_address, char data[3]);
uint8_t i2c_get_received_data(char* data);
void i2c_read(uint8_t slave_address, uint8_t reg_address, uint8_t num_bytes);

#endif
