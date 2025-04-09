#include <msp430fr2310.h>
#include <stdbool.h>

#include "../src/lcd.h"
#include "../../common/i2c.h"


/* --- defines --- */


// this is used in the slave init for i2c
#define SLAVE_ADDRESS SLAVE1_ADDR


/* --- program --- */


// copy the provided string into the start of a line buffer
void _copy_mode(char *line, const char *string){
	*line++ = *string++;
	*line++ = *string++;
	*line++ = *string++;
	*line++ = *string++;
	*line++ = *string++;
}

// update the temperature on a line with the provided temperature info
void _update_temperature(char *line, char temperature[2]){
	// for reference:
	//  temperature[0] -> whole number part of temp
	//  temperature[1] -> float part of temp, as tenths

	// extract whole number part
	line[10] = '0';
	while (temperature[0] > 9)
	{
		line[10]++;
		temperature[0] -= 10;
	}
	line[11] = '0' + temperature[0];

	// extract float part
	line[13] = '0' + temperature[1];
}

int main(void)
{
	// Stop watchdog timer
	WDTCTL = WDTPW | WDTHOLD;

	const char *heat_mode = "heat ";
	const char *cool_mode = "cool ";
	const char *off_mode = "off  ";
	const char *match_mode = "match";
	const char *set_mode = "set  ";

	char line_0[] = "off    \x00""A:xx.x\xDF""C";
	char line_1[] = "N xxxs \x01""P:xx.x\xDF""C";

	char received_buffer[3];

	// This double while(1) loop looks redudant but is quite important. Without this
	// 	extra while(1) loop, when powering up a programmed MSP the lcd_init will
	// 	always fail. I do not know why this solves it, but it does.
	// Unfortunately, this extra while(1) loop can cause issues when debugging with CCS.
	while (1)
	{
		lcd_init();

		// Initialize I2C as slave
		i2c_slave_init(SLAVE_ADDRESS);
		__enable_interrupt();

		// Disable low-power mode / GPIO high-impedance
		PM5CTL0 &= ~LOCKLPM5;

		// create custom characters for both temperatures
		const char *ambient_character = "";
		const char *plant_character = "";

		lcd_create_character(ambient_character, 0);
		lcd_create_character(plant_character, 1);

		while (1)
		{
			// poll until new data is received
			while(!i2c_get_received_data(received_buffer))
				;

			switch (received_buffer[0])
			{
				case 'A':
					_copy_mode(line_0, heat_mode);
					break;

				case 'B':
					_copy_mode(line_0, cool_mode);
					break;

				case 'C':
					_copy_mode(line_0, match_mode);
					break;

				case 'D':
					_copy_mode(line_0, off_mode);
					break;

				case 'P':
					_update_temperature(line_0, received_buffer + 1);
					break;

				case 'I':
					_update_temperature(line_1, received_buffer + 1);
					break;

				case 'T':
					// extract hundreds place of seconds
					line_1[2] = '0' + received_buffer[1];

					// extract tenths and ones place of seconds
					line_1[3] = '0';
					while (received_buffer[2] > 9)
					{
						line_1[3]++;
						received_buffer[2] -= 10;
					}
					line_1[4] = '0' + received_buffer[2];

					break;

				case '*':
					_copy_mode(line_0, set_mode);
					break;

				case '#':
					// set window size
					// right now the tens place is not printed to the lcd
					line_1[0] = '0' + received_buffer[2];
					break;

				default:
					break;
			}

			// update both lines rather than doing it in the switch
			// 	this doesn't take too long and saves some bytes
			lcd_print_line(line_0, 0);
			lcd_print_line(line_1, 1);
		}
	}
}
