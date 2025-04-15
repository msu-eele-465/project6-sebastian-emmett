#include <msp430.h>
#include <stdbool.h>
#include <stdint.h>
#include "i2c.h"
#include "intrinsics.h"

// Helpful Brock video :D https://www.youtube.com/watch?v=BvITEarUMkc

// Volatile variables for I2C slave receive handling
// Buffer to store up to 3 received bytes
volatile char rx_buffer[3];
// Number of bytes received in current transaction
volatile uint8_t rx_count = 0;
// Flag for completed transaction
volatile bool transaction_complete = false;
// Number of bytes in last completed transaction
volatile uint8_t rx_bytes = 0;

// Volatile variables for i2c_read
// Buffer for received bytes
volatile char rx_data[2];
// Index for received bytes
volatile uint8_t rx_index = 0;
// Flag for read completion
volatile bool i2c_rx_complete = false;

// Transmission complete flag
volatile bool i2c_tx_complete = true;
// Buffer and index for sending three bytes
volatile char tx_buffer[3];
volatile uint8_t tx_index = 0;

void i2c_master_init(void)
{
    // Configure USCI_B0 for I2C master mode
    // Put module in reset
    UCB0CTLW0 |= UCSWRST;
    // Use SMCLK (ensure SMCLK is ~1MHz for UCB0BRW=10)
    UCB0CTLW0 |= UCSSEL_3;
    // Prescaler for ~100kHz I2C clock
    UCB0BRW = 10;
    // I2C mode
    UCB0CTLW0 |= UCMODE_3;
    // Master mode
    UCB0CTLW0 |= UCMST;
    // Transmitter mode
    UCB0CTLW0 |= UCTR;

    // Auto STOP after UCB0TBCNT bytes
    UCB0CTLW1 |= UCASTP_2;
    // Send 3 bytes per transaction
    UCB0TBCNT = 3;
    
    // Configure pins P1.2 (SDA) and P1.3 (SCL)
    // P1.3 SCL
    P1SEL1 &= ~BIT3;
    P1SEL0 |= BIT3;

    // P1.2 SDA
    P1SEL1 &= ~BIT2;
    P1SEL0 |= BIT2;

    // Clear reset to enable module
    UCB0CTLW0 &= ~UCSWRST;

    // Enable interrupts for TX
    // Local enable for TX0
    UCB0IE |= UCTXIE0;
    // Enable maskables
    __enable_interrupt();
}

void i2c_slave_init(uint8_t address)
{
    // Configure pins P1.2 (SDA) and P1.3 (SCL)
    // P1.3 SCL
    P1SEL1 &= ~BIT3;
    P1SEL0 |= BIT3;
    // P1.2 SDA
    P1SEL1 &= ~BIT2;
    P1SEL0 |= BIT2;

    // Configure USCI_B0 for I2C slave mode
    // Put module in reset
    UCB0CTLW0 |= UCSWRST;
    // I2C mode, slave, synchronous
    UCB0CTLW0 |= UCMODE_3 | UCSYNC;
    // Set own address and enable
    UCB0I2COA0 = address | UCOAEN;
    // Clear reset to enable module
    UCB0CTLW0 &= ~UCSWRST;

    // Enable receive interrupt
    // Enable RX interrupt for slave
    UCB0IE |= UCRXIE;
}

void i2c_send(uint8_t slave_address, char data[3])
{
    // Store data for ISR
    tx_buffer[0] = data[0];
    tx_buffer[1] = data[1];
    tx_buffer[2] = data[2];
    tx_index = 0;
    // Toggle tx complete flag false
    i2c_tx_complete = false;
    // Set slave address
    UCB0I2CSA = slave_address;
    // Generate START
    UCB0CTLW0 |= UCTXSTT;

    // Delay
    int i;
    for (i = 0; i < 100; i = i + 1){}
}

uint8_t i2c_get_received_data(char* data) {
    if (transaction_complete) {
        uint8_t i = 0;
        for (i = 0; i < rx_bytes; i++) {
            data[i] = rx_buffer[i];
        }
        transaction_complete = false;
        return rx_bytes;
    } else {
        return 0;
    }
}

void i2c_read(uint8_t slave_address, uint8_t reg_address, uint8_t num_bytes)
{
    // Ensure transmitter mode to write register address
    UCB0CTLW0 |= UCTR;
    // Set slave address
    UCB0I2CSA = slave_address;
    // Send register address
    UCB0TXBUF = reg_address;
    // Wait for TX to complete
    int i;
    for (i = 0; i < 100; i = i + 1){}
    // Switch to receiver mode
    UCB0CTLW0 &= ~UCTR;
    // Set number of bytes to receive
    UCB0TBCNT = num_bytes;
    // Reset rx_index
    rx_index = 0;
    i2c_rx_complete = false;
    // Generate START
    UCB0CTLW0 |= UCTXSTT;
    // Wait for START to be sent
    for (i = 0; i < 100; i = i + 1){}
    // Enable RX interrupt
    UCB0IE |= UCRXIE;
}

#pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void)
{
    // Handle start condition
    if (UCB0IFG & UCSTTIFG)
    {
        rx_count = 0;
        UCB0IFG &= ~UCSTTIFG;
    }

    // Handle receive interrupt
    if (UCB0IFG & UCRXIFG)
    {
        if (UCB0CTLW0 & UCMST) // Master mode
        {
            if (rx_index < 2) // Assuming a buffer size of 2 for i2c_read (it'll always be for this)
            {
                rx_data[rx_index] = UCB0RXBUF;
                rx_index++;
            }
            else
            {
                UCB0RXBUF; // Discard extra bytes
            }
            if (rx_index == 2)
            {
                i2c_rx_complete = true;
                rx_index = 0; // Reset for next read
            }
        }
        else // Slave mode
        {
            if (rx_count < 3) // Buffer size of 3
            {
                rx_buffer[rx_count] = UCB0RXBUF;
                rx_count++;
            }
            else
            {
                UCB0RXBUF; // Discard extra bytes
            }
        }
        UCB0IFG &= ~UCRXIFG; // Clear receive interrupt flag
    }

    // If transmitting
    if (UCB0IFG & UCTXIFG0)
    {
        if (tx_index == 2)
        {
            UCB0TXBUF = tx_buffer[tx_index];
            tx_index = 0;
        }
        else
        {
            UCB0TXBUF = tx_buffer[tx_index];
            tx_index++;
        }
    }
    if (UCB0IFG & UCSTPIFG)
    {
        if (UCB0CTLW0 & UCMST)
        {
            i2c_tx_complete = true;
        }
        else
        {
            transaction_complete = true;
            rx_bytes = rx_count;
        }
        UCB0IFG &= ~UCSTPIFG;
    }
}
