#include <msp430.h>
#include "temperature_timer.h"
#include "msp430fr2355.h"
#include "../src/lm19.h"
#include "../src/lm92.h"

void init_temperature_sensors(void)
{
    lm19_sensor_init();
    lm92_sensor_init();
}

#pragma vector=TIMER2_B0_VECTOR
__interrupt void TIMER2_B0_ISR(void)
{
    // LM19

    // Eanble and start conversion
    ADCCTL0 |= ADCENC | ADCSC;
    // Wait for conversion to complete
    while ((ADCIFG & ADCIFG0) == 0);
    long adc_value = ADCMEM0;

    // Compute voltage in mV: Vin = (ADC_value * 3300) / 4095
    long V_in_mV = (adc_value * 3300) / 4095;

    // Compute temperature in tenths of a degree Celsius
    // T(tenths) (°C) = ((Vin(mV) * 100) - 186630) / -117
    long numerator = (V_in_mV * 100) - 186630;
    int16_t temp_tenths = (int16_t)(numerator / -117);

    // Add to lm19 averaging queue
    lm19_add_temperature(temp_tenths);

    // Only send average when window is full
    if (lm19_is_window_full())
    {
        int16_t average = lm19_get_average();

        char whole = average / 10;
        char decimal = average % 10;
        
        char data_to_send[3] = {'I', whole, decimal};
        i2c_send(SLAVE1_ADDR, data_to_send);
    }

    // LM92

    //#TODO
}
