/**
 * Driver for ping sensor
 * @file ping.c
 * @author Ben Nowell
 */

#include "ping.h"
#include "Timer.h"

// Global shared variables
// Use extern declarations in the header file

volatile uint32_t g_start_time = 0;
volatile uint32_t g_end_time = 0;
volatile enum{LOW, HIGH, DONE} g_state = LOW; // State of ping echo pulse

void ping_init (void){

    //enable the clock to GPIO Port B
    SYSCTL_RCGCGPIO_R |= 0x2;

    //wait for the GPIO port to be initialized
    while((SYSCTL_PRGPIO_R & 0x02) == 0){};

    //enable the clock for timer 3
    SYSCTL_RCGCTIMER_R |= 0x8;

    //wait for the timer to be initialized
    while ((SYSCTL_RCGCTIMER_R & 0x8) == 0){};

    //enable digital functionality on pin PB3
    GPIO_PORTB_DEN_R |= 0x8;

    //set PB3 as an input by clearing bit 3
    GPIO_PORTB_DIR_R &= ~0x8;

    //enable alternate functionality on pin PB3
    GPIO_PORTB_AFSEL_R |= 0x8;

    //set timer 3 to control pin PB3
    GPIO_PORTB_PCTL_R |=0x7000;
    
    //set the interrupt to priority 2 (so that in future labs the UART interrupt which is priority 1 will have higher priority than the ping interrupt)
    NVIC_PRI9_R = (NVIC_PRI9_R & 0xFFFFFF1F) | 0x60;
    
    //enable the interrupt for timer 3B
    NVIC_EN1_R |= 0x10;

    //bind out interrupt handler to the timer interrupt
    IntRegister(INT_TIMER3B, TIMER3B_Handler);

    IntMasterEnable();

    //disable timer 3B while we configure it
    TIMER3_CTL_R &= ~0x100;
    
    //configure timer 3B for 16 bit mode
    TIMER3_CFG_R &= ~0x7;
    TIMER3_CFG_R |= 0x4;
    
    //set timer 3B to capture mode and edge time mode
    TIMER3_TBMR_R |= 0x7;
    
    //set the load value for timer 3B
    TIMER3_TBILR_R |= 0xFFFF;
    
    //set the prescale to extend the timer to 24 bits
    TIMER3_TBPR_R |= 0xFF;
    
    //enable the interrupt for timer 3B
    TIMER3_IMR_R |= 0x400;
    
    //tell the timer to listen for both rising and falling edges
    TIMER3_CTL_R |= 0xC00;
    
    //reenable the timer after we are done configuring
    TIMER3_CTL_R |= 0x100;
}

void ping_trigger (void){
    g_state = LOW;
    // Disable timer and disable timer interrupt
    TIMER3_CTL_R &= ~0x100;
    TIMER3_IMR_R &= ~0x400;
    // Disable alternate function (disconnect timer from port pin)
    GPIO_PORTB_AFSEL_R &= ~0x8;

    // YOUR CODE HERE FOR PING TRIGGER/START PULSE
    //set pin PB3 to be an output so we can trigger the pulse
    GPIO_PORTB_DIR_R |= 0x8;
    
    //send the pulse
    GPIO_PORTB_DATA_R &= ~0x8;
    GPIO_PORTB_DATA_R |= 0x8;
    timer_waitMicros(5);
    GPIO_PORTB_DATA_R &= ~0x8;

    // Clear an interrupt that may have been erroneously triggered
    TIMER3_ICR_R |= 0x400;
    // Re-enable alternate function, timer interrupt, and timer
    GPIO_PORTB_AFSEL_R |= 0x8;
    TIMER3_IMR_R |= 0x400;
    TIMER3_CTL_R |= 0x100;
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

    //check if the interrupt is from the capture event
    if ((TIMER3_MIS_R & 0x400) == 0x400) {
        
        //clear the interrupt
        TIMER3_ICR_R |= 0x400;

        if (g_state == LOW) {
            g_start_time = TIMER3_TBR_R;
            g_state = HIGH;
        }
        else if (g_state == HIGH) {
            g_end_time = TIMER3_TBR_R;
            g_state = DONE;
        }
    }
    
}

float ping_getDistance (void){
    
    uint32_t cycles;
    float time;
    float distance;
    
    //trigger ping and wait for it to finish
    ping_trigger();
    while (g_state != DONE) {};
    
    //calculate the amount of clock cycles the pulse took to return
    if (g_start_time < g_end_time) {
        //overflow occured
        cycles = g_start_time + (0xFFFFFF - g_end_time) + 1;
    }
    else {
        cycles = g_start_time - g_end_time;
    }
    
    //convert clock cycles to seconds
    time = (float)cycles / 16000000.0;
    
    //calculate the distance by multiplying by the speed of sound (34300 cm/s) and dividing by 2 to account for the round trip
    distance = (time * 34300.0) / 2.0;
    distance -= 3.0;
    
    return distance;
    
}

void ping_getCycles(uint32_t* cycles, bool* overflow) {
    //trigger ping and wait for it to finish
    ping_trigger();
    while (g_state != DONE) {};
    
    //calculate the amount of clock cycles the pulse took to return
    if (g_start_time < g_end_time) {
        //overflow occured
        *overflow = true;
        *cycles = g_start_time + (0xFFFFFF - g_end_time) + 1;
    }
    else {
        *overflow = false;
        *cycles = g_start_time - g_end_time;
    }
}
