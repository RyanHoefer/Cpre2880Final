/*
 * adc.c
 *
 *  Created on: Mar 19, 2026
 *      @author Ben Nowell
 */

#include "adc.h"
#include <inc/tm4c123gh6pm.h>
#include <stdint.h>

void adc_init(void) {
    //enable the clock to ADC Module 0
    SYSCTL_RCGCADC_R |= 0x1;
    
    //wait for ADC Module 0 to be initialized
    while((SYSCTL_PRADC_R & 0x01) == 0){};
    
    //enable the clock to GPIO Port B
    SYSCTL_RCGCGPIO_R |= 0x2;
    
    //wait for the GPIO port to be initialized
    while((SYSCTL_PRGPIO_R & 0x02) == 0){};
    
    //set PB4 as an input by clearing bit 4
    GPIO_PORTB_DIR_R &= ~0x10;
    
    //enable the alternate function on PB4
    GPIO_PORTB_AFSEL_R |= 0x10;
    
    //disable digital functionality on PB4
    GPIO_PORTB_DEN_R &= ~0x10;
    
    //enable analog functionality on PB4
    GPIO_PORTB_AMSEL_R |= 0x10;
    
    //disable sequencer 3 while we are configuring it
    ADC0_ACTSS_R &= ~0x8;
    
    //set trigger for sequencer 3 to 0 for software controller sampling
    ADC0_EMUX_R &= ~0xF000;
    
    //enable 16x hardware averaging for ADC0
    ADC0_SAC_R &= ~0x7;
    ADC0_SAC_R |= 0x4;
    
    //set AIN10 to be the input for sequencer 3
    ADC0_SSMUX3_R &= ~0xF;
    ADC0_SSMUX3_R |= 0xA;
    
    //set the end of sequence and interrupt enable
    ADC0_SSCTL3_R |= 0x6;
    
    //reenable sequencer 3
    ADC0_ACTSS_R |= 0x8;
}

uint16_t adc_read(void) {
    uint16_t result;
    
    //tell sequencer 3 to sample
    ADC0_PSSI_R = 0x0008;
    
    //wait for the sample and conversion to be complete
    while((ADC0_RIS_R & 0x08)==0);
    
    //read the result
    result = ADC0_SSFIFO3_R & 0xFFF;
    
    //clear the flag
    ADC0_ISC_R = 0x0008;
        
    return result;
}
