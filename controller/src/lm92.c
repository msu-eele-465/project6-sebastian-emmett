#include <msp430.h>
#include "lm92.h"
#include "msp430fr2355.h"
#include "../common/i2c.h"

#define LM92_MAX_WINDOW 20

static int16_t lm92_queue[LM92_MAX_WINDOW];
static uint8_t lm92_window_size = 3;
static uint8_t lm92_num_samples = 0;
static uint8_t lm92_index = 0;
static int32_t lm92_sum = 0;

extern volatile char rx_data[2];
extern bool i2c_rx_complete;

void lm92_sensor_init(void)
{
    // Dont actually need to do anything here lol
}

void set_lm92_window_size(uint8_t size)
{
    if (size > LM92_MAX_WINDOW) size = LM92_MAX_WINDOW;
    if (size < 1) size = 1;
    lm92_window_size = size;
    lm92_num_samples = 0;
    lm92_index = 0;
    lm92_sum = 0;
}

static void add_to_lm92_queue(int16_t value)
{
    if (lm92_num_samples < lm92_window_size)
    {
        lm92_queue[lm92_index] = value;
        lm92_sum += value;
        lm92_index = (lm92_index + 1) % lm92_window_size;
        lm92_num_samples++;
    }
    else
    {
        lm92_sum -= lm92_queue[lm92_index];
        lm92_queue[lm92_index] = value;
        lm92_sum += value;
        lm92_index = (lm92_index + 1) % lm92_window_size;
    }
}

static int16_t get_lm92_average(void)
{
    if (lm92_num_samples == 0) return 0;
    return (int16_t)(lm92_sum / lm92_num_samples);
}

void lm92_add_temperature(int16_t value)
{
    add_to_lm92_queue(value);
}

bool lm92_is_window_full(void)
{
    return lm92_num_samples == lm92_window_size;
}

int16_t lm92_get_average(void)
{
    return get_lm92_average();
}

int16_t lm92_read_temperature(void)
{
    // Read temperature from LM92 (register 0x00, 2 bytes)
    i2c_read(LM92_ADDR, 0x00, 2);
    // Wait for read to complete
    int i;
    for (i = 0; i < 100; i = i + 1){}
    // Combine bytes into 16-bit value
    int16_t temp_raw = (rx_data[0] << 8) | rx_data[1];
    // Shift right by 3 to remove status bits (D0-D2)
    temp_raw >>= 3;
    // Convert to tenths of a degree C (LSB = 0.0625 C)
    // temp_tenths = temp_raw * 0.0625 * 10 = temp_raw * 625 / 100
    int16_t temp_tenths = (temp_raw * 6.25);
    return temp_tenths;
}