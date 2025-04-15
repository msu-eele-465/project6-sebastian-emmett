#ifndef LM19_H
#define LM19_H

#include <stdint.h>
#include <stdbool.h>
#include <../common/i2c.h>

void lm19_sensor_init(void);
void set_lm19_window_size(uint8_t size);
void lm19_add_temperature(int16_t value);      // Public function to add temperature to queue
bool lm19_is_window_full(void);                // Public function to check if averaging window is full
int16_t lm19_get_average(void);                // Public function to get the average

#endif
