#include <msp430.h>
#include "temperature_timer.h"
#include "msp430fr2355.h"
#include "../src/lm19.h"
#include "../src/lm92.h"
#include "../src/heartbeat.h"
#include "src/keyboard.h"

int toggle_sensor = 0;
extern unsigned int heartbeat_count;
extern unsigned int last_mode_switch_time;

void init_temperature_sensors(void)
{
    lm19_sensor_init();
    lm92_sensor_init();

    // Configure Timer 2 B0 for ~2Hz if SMCLK ~1MHz
    // SMCLK/8, up mode
    TB2CTL = TBSSEL__SMCLK | ID__8 | MC__UP | TBCLR;
    // further /8 => total /64
    TB2EX0 = TBIDEX__8;
    // 1/8 second at ~1 MHz/3906
    TB2CCR0 = 1953;
    // enable interrupt :D
    TB2CCTL0 = CCIE;
}

#pragma vector=TIMER2_B0_VECTOR
__interrupt void TIMER2_B0_ISR(void)
{
    switch (toggle_sensor)
    {
        case 0:
        toggle_sensor++;

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
            break;
        
        case 1:
            toggle_sensor++;

            // LM92

            char data_to_also_send[3] = {'P', 0x45, 0x00};
            i2c_send(SLAVE1_ADDR, data_to_also_send);

            /*

            // Get lm92 temperature
            int16_t lm92_temp = lm92_read_temperature();

            // Add to lm92 averaging queue
            lm92_add_temperature(lm92_temp);

            // Only send average when window is full
            if (lm92_is_window_full())
            {
                int16_t average = lm92_get_average();

                char whole = average / 10;
                char decimal = average % 10;

                char data_to_send[3] = {'P', whole, decimal};
                i2c_send(SLAVE1_ADDR, data_to_send);
            }

            */
            break;
        
        case 2:
            toggle_sensor++;
            char data_to_also_ALSO_send[3] = {'T', (heartbeat_count - last_mode_switch_time) / 100, (heartbeat_count - last_mode_switch_time) % 100};
            i2c_send(SLAVE1_ADDR, data_to_also_ALSO_send);
            break;
        
        case 3:
            toggle_sensor = 0;
            break;
    }
}
