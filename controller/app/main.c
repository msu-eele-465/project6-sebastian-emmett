#include <msp430.h>
#include <stdbool.h>
#include <string.h>
// Include this to use init_keypad() and poll_keypad()
#include "../src/keyboard.h"
// For init_heartbeat()
#include "../src/heartbeat.h"
// For adjusting the pwm signal meant for rgb_led
#include "../src/rgb_led.h"
// For creating the pwm signal on P6.0 - P6.2
#include "../src/pwm.h"
// Include I2C header
#include "../../common/i2c.h"
#include "msp430fr2355.h"
#include "../src/lm19.h"
#include "src/temperature_timer.h"

// ----------------------------------------------------------------------------
// Globals! (yes they deserve their own lil space)
// ----------------------------------------------------------------------------
// For general key tracking
bool key_down = false;

// For temperature averaging and targeting
uint8_t window_size = 0;
uint16_t target_temperature = 0;

// Variables for LM19
bool fahrenheit_mode = false;
bool setting_window = false;
bool setting_temperature = false;

// ----------------------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------------------
int main(void)
{
    // Stop watchdog
    WDTCTL = WDTPW | WDTHOLD;

    // Set up Timer_B0 for blinking P1.0
    init_heartbeat();
    // Init keyboard with P4s as input and P5s as output
    init_keypad();
    // LED on P6.6 to show when a key gets pressed
    init_responseLED();
    // Timer_B1 => ~50 ms interrupt
    init_keyscan_timer();
    // Initialize temperature sensors
    init_temperature_sensors();
    // Initialize I2C module as master
    i2c_master_init();

    // Turn on I/O
    PM5CTL0 &= ~LOCKLPM5;

    // Enable global interrupts for the heartbeat timer
    __bis_SR_register(GIE);

    while (1)
    {
        // ----------------------------------------------------------------------------
        // New main
        // ----------------------------------------------------------------------------
    }
}
