#include <msp430.h>
#include "lm92.h"
#include "msp430fr2355.h"

#define LM92_MAX_WINDOW 20

static int16_t lm92_queue[LM92_MAX_WINDOW];
static uint8_t lm92_window_size = 3;
static uint8_t lm92_num_samples = 0;
static uint8_t lm92_index = 0;
static int32_t lm92_sum = 0;

void lm92_sensor_init(void)
{
    
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
