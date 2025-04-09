#include <msp430fr2310.h>
#include <stdbool.h>
#include "../../common/i2c.h"
#include "../src/led_bar.h"

// Globals for LED bar control

// used within the led bar for update logic
uint8_t pattern_type = 0;

// For holding incoming data from i2c
char received_buffer[3];

// Use SLAVE1_ADDR from i2c.h
#define SLAVE_ADDRESS SLAVE1_ADDR

int main(void)
{
    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    while (true)
    {
        // Initialize LED bar
        led_bar_init();

        // Disable low-power mode / GPIO high-impedance
        PM5CTL0 &= ~LOCKLPM5;

        // Initialize I2C as slave
        i2c_slave_init(SLAVE_ADDRESS);
        __enable_interrupt();

		static uint16_t delay = 0;

        while (true)
        {
            // update pattern_type if new data received
			if (i2c_get_received_data(received_buffer))
			{
				switch (received_buffer[0])
				{
					case '=':
					case 'D':
						pattern_type = 0;
						break;

					case '+':
					case 'A':
						pattern_type = 1;
						break;

					case '-':
					case 'B':
						pattern_type = 2;
						break;

					default:
						break;
				}
			}

			if (!delay)
			{
				// delay for at least 1/16th of a second
				// 	because the above instructions influence timing
				// (~1 MHz clock => 62500 cycles = ~1/16s)
				delay = 62500;

				led_bar_update_pattern();
			}
			else{
				delay--;
			}
        }
    }
}
