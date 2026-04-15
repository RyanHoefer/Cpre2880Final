/*
 * ping.c
 *
 *  Created on: Apr 1, 2026
 *      Author: rhoefer
 */


/**
 * Driver for ping sensor
 * @file ping.c
 * @author
 */

#include "ping.h"
#include "Timer.h"
#include "lcd.h"


// Global shared variables
// Use extern declarations in the header file

volatile uint32_t g_start_time = 0;
volatile uint32_t g_end_time = 0;
volatile int triggered = 0;
volatile enum{LOW, HIGH, DONE} g_state = LOW; // State of ping echo pulse
int overflow = 0;


void ping_init (void){
    SYSCTL_RCGCGPIO_R |= 0x02;
    SYSCTL_RCGCTIMER_R |= 0x8;

    while ((SYSCTL_PRGPIO_R & 0x2) == 0) {};
    while ((SYSCTL_PRTIMER_R & 0x8) == 0) {};

    GPIO_PORTB_DEN_R |= 0x8; //  enable PB3 as digital pin
    GPIO_PORTB_AFSEL_R |= 0x8; // make PB3 alternate function
    GPIO_PORTB_PCTL_R |= 0x7000;// set 7 to the mux value for the timer

    IntRegister(INT_TIMER3B, TIMER3B_Handler);

    IntMasterEnable();
    NVIC_EN1_R |= 0x10;


    TIMER3_CTL_R  &= ~0x100;// disable the timer
    TIMER3_CFG_R  |= 0x4; // set 16 bit mode
    TIMER3_TBMR_R |= 0x7;// capture mode, edge time mode
    TIMER3_TBILR_R |= 0xFFFF;// set max value
    TIMER3_TBPR_R = 0xFF;//
    TIMER3_ICR_R |= 0x400; // clear capture mode interrupt
    TIMER3_IMR_R |= 0x400;// set apture mode interrupt thing here
    NVIC_PRI9_R |= 0x10;
    NVIC_PRI9_R &= ~0x60;


    // Configure and enable the timer

    TIMER3_CTL_R |= 0xD;//
}

void ping_trigger (void){
    g_state = LOW;
    // Disable timer and disable timer interrupt
    TIMER3_CTL_R &= ~0xD00;
    TIMER3_IMR_R &= ~0x400 ;
    // Disable alternate function (disconnect timer from port pin)
    GPIO_PORTB_AFSEL_R &= ~0x8;// disable alternate function
    GPIO_PORTB_DIR_R |= 0x8;//set as output

    // YOUR CODE HERE FOR PING TRIGGER/START PULSE
    GPIO_PORTB_DATA_R &= ~(0x8);
    timer_waitMicros(4);
    GPIO_PORTB_DATA_R |= 0x8;
    g_state = HIGH;
    timer_waitMicros(6);
    GPIO_PORTB_DATA_R &= ~(0x8);
    g_state = LOW;



    // Clear an interrupt that may have been erroneously triggered
    TIMER3_ICR_R |= 0xF00;
    // Re-enable alternate function, timer interrupt, and timer

    GPIO_PORTB_AFSEL_R |= 0x8;//re enable alternate function
    TIMER3_IMR_R |= 0x400;

    TIMER3_CTL_R |= 0xD00;
}

void TIMER3B_Handler(void){



  // YOUR CODE HERE
  // As needed, go back to review your interrupt handler code for the UART lab.
  // What are the first lines of code in the ISR? Regardless of the device, interrupt handling
  // includes checking the source of the interrupt and clearing the interrupt status bit.
  // Checking the source: test the MIS bit in the MIS register (is the ISR executing
  // because the input capture event happened and interrupts were enabled for that event?
  // Clearing the interrupt: set the ICR bit (so that same event doesn't trigger another interrupt)
  // The rest of the code in the ISR depends on actions needed when the event happens.


        //check if handler called due to RX event
        if (TIMER3_MIS_R & 0x400)
        {
            //byte was received in the UART data register
            //clear the RX trigger flag (clear by writing 1 to ICR)
            TIMER3_ICR_R |= 0x400;
            if (triggered == 0){
                g_start_time = TIMER3_TBR_R;
                triggered = 1;
            }else{
                g_end_time = TIMER3_TBR_R;
                triggered = 0;
                //int time = timer_getMicros();
                if ((g_start_time - g_end_time) > 288000){
                    overflow++;
                }
                lcd_printf("Clk Cyc %d\nS-Time %d\nE-Time %d\nOF%d", (g_start_time - g_end_time), g_end_time,g_start_time,overflow);
            }





        }

}

float ping_getDistance (void){




    return 0.0;

}

