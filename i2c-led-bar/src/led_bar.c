#include <msp430.h>
#include <stdbool.h>
#include <stdint.h>
#include "../src/led_bar.h"


/* --- globals --- */


// this global will be referenced from main.c
extern uint8_t pattern_type;


/* --- pattern variable --- */


// count used for keeping track of how much of the
// 	led bar has been filled up
static uint8_t fill_count = 0;


/* --- program --- */


void led_bar_init(void)
{
    // configure LED bar pins as outputs: P1.0, P1.4, P1.5, P1.6, P1.7, P1.1, P2.6, P2.7

	// set as digital I/O
    P1SEL0 &= ~(BIT0 | BIT1 | BIT4 | BIT5 | BIT6 | BIT7);
    P1SEL1 &= ~(BIT0 | BIT1 | BIT4 | BIT5 | BIT6 | BIT7);
	// clear output
    P1OUT &= ~(BIT0 | BIT1 | BIT4 | BIT5 | BIT6 | BIT7);
	// set as output
    P1DIR |= BIT0 | BIT1 | BIT4 | BIT5 | BIT6 | BIT7;

	// set as digital I/O
    P2SEL0 &= ~(BIT6 | BIT7);
    P2SEL1 &= ~(BIT6 | BIT7);
	// clear output
    P2OUT &= ~(BIT6 | BIT7);
	// set as output
    P2DIR |= BIT6 | BIT7;
}

// update the output displayed on the LED bar
void _led_bar_update(void){
    // update LED bar following pin mapping of: P1.0, P1.4 - P1.7, P1.1, P2.6 - P2.7
    P1OUT = (P1OUT & ~(BIT0 | BIT1 | BIT4 | BIT5 | BIT6 | BIT7)) | \
            (((fill_count) & BIT0) << 0) | \
            (((fill_count) & (BIT1 | BIT2 | BIT3 | BIT4)) << 3) | \
            (((fill_count) & BIT5) >> 4); \
    P2OUT = (P2OUT & ~(BIT6 | BIT7)) | \
            (((fill_count) & (BIT6 | BIT7)) << 0);
}

void led_bar_update_pattern(void)
{
    switch (pattern_type)
    {
		// off
		case 0:
			fill_count = 0;
			break;

		// heating up, fill right
		case 1:
			// this will reset the count if fill left was previously selected
			if (fill_count & 0x01)
			{
				fill_count = 0;
			}
			// advance count right
			else{
				fill_count = (fill_count >> 1) + 128;
			}

			break;

		// cooling down, fill left
		case 2:
			// this will reset the count if fill right was previously selected
			if (fill_count & 0x80)
			{
				fill_count = 0;
			}
			// advance count left
			else{
				fill_count = (fill_count << 1) + 1;
			}

			break;

		default:
			break;
	}

	_led_bar_update();
}
