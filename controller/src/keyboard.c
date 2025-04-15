#include <msp430.h>
#include <stdbool.h>
#include "keyboard.h"
#include "../src/rgb_led.h"
#include "../../common/i2c.h"
#include "../src/lm19.h"

extern bool key_down;

extern bool setting_window;
extern uint8_t window_size;
int window_digits_entered = 0;

extern bool setting_temperature;
extern uint16_t target_temperature;
int temp_digits_entered = 0;

// 4x4 Keypad Layout
static const char KEYPAD_MAP[4][4] =
{
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// ----------------------------------------------------------------------------
// init_keypad: 
//   Rows (P5.0..3) as outputs
//   Columns now on P4.4..7 - active-high requires pull-downs
// ----------------------------------------------------------------------------
void init_keypad(void)
{
    // Rows = outputs, start low
    P5DIR |= (BIT0 | BIT1 | BIT2 | BIT3);
    P5OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3);

    // Columns = inputs on P4.4..7
    P4DIR &= ~(BIT4 | BIT5 | BIT6 | BIT7);

    // Enable internal pull-downs for columns on P4.4..7
    P4REN |=  (BIT4 | BIT5 | BIT6 | BIT7);
    P4OUT &= ~(BIT4 | BIT5 | BIT6 | BIT7);
}

// ----------------------------------------------------------------------------
// poll_keypad:
//   1) For each row, set that row high, all others low
//   2) Read columns (P4.4..7). If any column bit is 1 => pressed key
//   3) Return the char from KEYPAD_MAP[row][col], or 0 if none
// ----------------------------------------------------------------------------
char poll_keypad(void)
{
    #define ROW_MASK (BIT0 | BIT1 | BIT2 | BIT3)

    int row;
    for (row = 0; row < 4; row++)
    {
        // Clear all rows
        P5OUT &= ~ROW_MASK;
        // Drive *only* this row high
        P5OUT |= (BIT0 << row);

        // Small settle delay - DO NOT REMOVE THIS LMAO
        __delay_cycles(50);

        // Read columns from P4.4..7 => shift right by 4, mask 0x0F
        unsigned char col_state = (P4IN >> 4) & 0x0F;

        int col;
        for (col = 0; col < 4; col++)
        {
            // If column bit is high => pressed key
            if (col_state & (1 << col))
            {
                // Reset rows
                P5OUT &= ~ROW_MASK;
                // Return the key from KEYPAD_MAP[row][col]
                return KEYPAD_MAP[row][col];
            }
        }
    }

    // No key found
    P5OUT &= ~ROW_MASK;
    return 0;
}

// ----------------------------------------------------------------------------
// init_responseLED: LED on P6.6 (off initially)
// ----------------------------------------------------------------------------
void init_responseLED(void)
{
    P6DIR |= BIT6;
    P6OUT &= ~BIT6;
}

// ----------------------------------------------------------------------------
// init_keyscan_timer: Timer_B1 => fires ~every 50 ms
// ----------------------------------------------------------------------------
void init_keyscan_timer(void)
{
    // For 50 ms @ ~1 MHz SMCLK, /64 => 1 MHz/64 = 15625 Hz
    // 50 ms => 0.05 * 15625 = 781 counts
    TB1CTL   = TBSSEL__SMCLK | ID__8 | MC__UP | TBCLR;
    // total /64
    TB1EX0   = TBIDEX__8;
    // ~50 ms 
    TB1CCR0  = 781;
    // Enable CCR0 interrupt
    TB1CCTL0 = CCIE;
}

// ----------------------------------------------------------------------------
// Timer_B1 ISR => polls keypad every ~50ms and runs keyboard_interrupt logic
// ----------------------------------------------------------------------------
#pragma vector=TIMER1_B0_VECTOR
__interrupt void TIMER1_B0_ISR(void)
{
    char key = poll_keypad();
    // A key was detected
    if (key != 0 && !key_down)
    {
        key_down = true;
        // Set P6.6 LED on
        P6OUT |= BIT6;

        // ---------------------------
        // Additional Logic
        // ---------------------------
        
        // 1) If 'A' => Heat Mode
        if (key == 'A')
        {
            char data_to_send[3] = {'A', '0', '0'};
            i2c_send(SLAVE1_ADDR, data_to_send);
        }
        // 2) If 'B' => Cool Mode
        else if (key == 'B')
        {
            char data_to_send[3] = {'B', '0', '0'};
            i2c_send(SLAVE1_ADDR, data_to_send);
        }
        // 3) If 'C' => Match Mode
        else if (key == 'C')
        {
            char data_to_send[3] = {'C', '0', '0'};
            i2c_send(SLAVE1_ADDR, data_to_send);
        }
        // 4) If 'D' => Off Mode
        else if (key == 'D')
        {
            char data_to_send[3] = {'D', '0', '0'};
            i2c_send(SLAVE1_ADDR, data_to_send);
        }
        // 5) If key is numeric => update window/temp
        else if (key >= '0' && key <= '9')
        {
            if (setting_window)
            {
                // Update window_size with new incoming value
                window_size = window_size * 10 + (key - '0');
                // Increment digit counter
                window_digits_entered++;
                if (window_digits_entered == 2)
                {
                    // If all digits aquired, calculate tens and ones
                    char tens = window_size / 10;
                    char ones = window_size % 10;
                    // Send it out on i2c
                    char data_to_send[3] = {'#', tens, ones};
                    i2c_send(SLAVE1_ADDR, data_to_send);
                    setting_window = false;
                    // Then, set lm19 window size
                    set_lm19_window_size(window_size);
                }
            }

            if (setting_temperature)
            {
                // Update target_temperature with new incoming value
                target_temperature = target_temperature * 10 + (key - '0');
                // Increment digit counter
                temp_digits_entered++;
                if (temp_digits_entered == 3)
                {
                    // If all digits aquired, calculate whole and decimal
                    char whole = target_temperature / 10;
                    char decimal = target_temperature % 10;
                    // Send it out on i2c
                    char data_to_send[3] = {'*', whole, decimal};
                    i2c_send(SLAVE1_ADDR, data_to_send);
                    setting_temperature = false;
                }
            }
        }
        // 6) If '#' => Set Window Mode
        else if (key == '#')
        {
            setting_window = true;
            window_size = 0;
            window_digits_entered = 0;
        }
        // 7) If '*' => Set Temperature Mode
        else if (key == '*')
        {
            setting_temperature = true;
            target_temperature = 0;
            temp_digits_entered = 0;
        }
    } 
    else if (key == 0 && key_down == true) 
    {
        key_down = false;

        // Set P6.6 to off
        P6OUT &= ~BIT6;
    }
}
