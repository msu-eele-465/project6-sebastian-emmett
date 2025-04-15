#include <msp430.h>
#include "lm19.h"
#include "msp430fr2355.h"

#define LM19_MAX_WINDOW 20

static int16_t lm19_queue[LM19_MAX_WINDOW];
static uint8_t lm19_window_size = 3;
static uint8_t lm19_num_samples = 0;
static uint8_t lm19_index = 0;
static int32_t lm19_sum = 0;

void lm19_sensor_init(void)
{
    // Configure P1.4 as ADC input (A4)
    P1SEL0 |= BIT4;
    P1SEL1 |= BIT4;

    // Configure ADC
    // Set conv clock cycles = 16 (10)
    ADCCTL0 &= ~ADCSHT;
    ADCCTL0 |= ADCSHT_2;
    // Turn on ADC
    ADCCTL0 |= ADCON;

    // Choose SMCLK
    ADCCTL1 |= ADCSSEL_2;
    // Sample singal source = sampling timer
    ADCCTL1 |= ADCSHP;

    // Clear resolution
    ADCCTL2 &= ~ADCRES;
    // Set 12-bit resolution
    ADCCTL2 |= ADCRES_2;

    // ADC input = A4 (P1.4)
    ADCMCTL0 |= ADCINCH_4;

    // Configure Timer 2 B0 for ~2Hz if SMCLK ~1MHz
    // SMCLK/8, up mode
    TB2CTL = TBSSEL__SMCLK | ID__8 | MC__UP | TBCLR;
    // further /8 => total /64
    TB2EX0 = TBIDEX__8;
    // 1 second at ~1 MHz/128
    TB2CCR0 = 7812;
    // enable interrupt :D
    TB2CCTL0 = CCIE;

    // Enable global interrupts
    __enable_interrupt();
}

void set_lm19_window_size(uint8_t size)
{
    if (size > LM19_MAX_WINDOW) size = LM19_MAX_WINDOW;
    if (size < 1) size = 1;
    lm19_window_size = size;
    lm19_num_samples = 0;
    lm19_index = 0;
    lm19_sum = 0;
}

static void add_to_lm19_queue(int16_t value)
{
    if (lm19_num_samples < lm19_window_size)
    {
        lm19_queue[lm19_index] = value;
        lm19_sum += value;
        lm19_index = (lm19_index + 1) % lm19_window_size;
        lm19_num_samples++;
    }
    else
    {
        lm19_sum -= lm19_queue[lm19_index];
        lm19_queue[lm19_index] = value;
        lm19_sum += value;
        lm19_index = (lm19_index + 1) % lm19_window_size;
    }
}

static int16_t get_lm19_average(void)
{
    if (lm19_num_samples == 0) return 0;
    return (int16_t)(lm19_sum / lm19_num_samples);
}

void lm19_add_temperature(int16_t value)
{
    add_to_lm19_queue(value);
}

bool lm19_is_window_full(void)
{
    return lm19_num_samples == lm19_window_size;
}

int16_t lm19_get_average(void)
{
    return get_lm19_average();
}
