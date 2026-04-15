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




void servo_init (void){
    SYSCTL_RCGCGPIO_R |= 0x02;// clock to port gpiob
    SYSCTL_RCGCTIMER_R |= 0x02;// clock to timer 1

    while ((SYSCTL_PRGPIO_R & 0x2) == 0) {};
    while ((SYSCTL_PRTIMER_R & 0x02) == 0) {};

    GPIO_PORTB_DEN_R |= 0x20; //  enable PB5 as digital pin
    GPIO_PORTB_AFSEL_R |= 0x20; // make PB5 alternate function
    //GPIO_PORTB_PCTL_R &= ~0xF00000;// reset mux for the timer
    GPIO_PORTB_PCTL_R |= 0x700000;// set 7 to the mux value for the timer


    TIMER1_CTL_R  &= ~0x100;// disable the timer
    TIMER1_CFG_R  |= 0x4; // set 16 bit mode
    TIMER1_TBMR_R |= 0xA;// tbams and 0x2 for low group
    //TIMER1_TBMR_R &= ~0x4;// reset bit 2
    TIMER1_TBMR_R &= ~0x10;// make sure bit 4 is 0


   // Configure and enable the timer

    TIMER1_CTL_R |= 0x100; //
}

void servo_move(uint16_t degrees){
    TIMER1_CTL_R  &= ~0x100;// disable the timer


      TIMER1_TBILR_R = 0xE200;// set max value
      TIMER1_TBPR_R = 0x04;//



      TIMER1_TBMATCHR_R = (0xDF94 - (degrees * 215));
      //TIMER1_TBMATCHR_R = (0x0100 /*- (degrees * 50)*/);
      TIMER1_TBPMR_R = 0x04;

      TIMER1_CTL_R |= 0x100;

}







