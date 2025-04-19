#ifndef LM92_H
#define LM92_H

#include <stdint.h>
#include <stdbool.h>
#include <../common/i2c.h>

void lm92_sensor_init(void);
void set_lm92_window_size(uint8_t size);
void lm92_add_temperature(int16_t value);      // Public function to add temperature to queue
bool lm92_is_window_full(void);                // Public function to check if averaging window is full
int16_t lm92_get_average(void);                // Public function to get the average
int16_t lm92_read_temperature(void);           // Public function to read the temperature via i2c

#endif
