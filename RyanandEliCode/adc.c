/*
 * adc.c
 *
 *  Created on: Mar 25, 2026
 *      Author: rhoefer
 */

#include <inc/tm4c123gh6pm.h>
#include <stdint.h>
#include "adc.h"
#include "Timer.h"
#include "open_interface.h"
#include "lcd.h"
#include <math.h>

/*
void main(){
    init_ADC();
    lcd_init();
    timer_init();
    int result;
    float distanceCM;
    char message[10];
    while (1){
        result = ADC_get_output();
        distanceCM = convert_from_raw_to_cm(result);
        sprintf(message, "%.1f", distanceCM);
        lcd_printf(message);
        timer_waitMicros(500000);
    }





}

*/
float convert_from_raw_to_cm(int rawVal){
     return (6*pow(10,6))*pow(rawVal,-1.692);

}

void init_ADC(){
        SYSCTL_RCGCADC_R |= 0x1; // activate ADC0
        SYSCTL_RCGCGPIO_R |= 0x02; //activate clock for Port B
        while((SYSCTL_PRGPIO_R&0x02) != 0x02){}; //for stabilization
        GPIO_PORTB_DIR_R &= ~0x10; //make PB4 input
        GPIO_PORTB_AFSEL_R |= 0x10; // enable alternate function on PB4
        GPIO_PORTB_DEN_R &= ~0x10; //  disable digital I/O on PB4
        GPIO_PORTB_AMSEL_R |= 0x10; // enable analog functionality on PB4
        ADC0_SSPRI_R = 0x0123; //Sequencer 3 is highest priority
        ADC0_ACTSS_R &= ~0x0008; //disable sample sequencer 3
        ADC0_EMUX_R &= ~0xF000; //seq3 is software trigger
        ADC0_SSMUX3_R &= ~0x000F;
        ADC0_SSMUX3_R += 10; // set channel
        ADC0_SSCTL3_R = 0x0006; // no TS0 D0, yes IE0 END0
        ADC0_IM_R &= ~0x0008; // disable SS3 interrupts
        ADC0_ACTSS_R |= 0x0008; //  enable sample sequencer 3

}

int ADC_get_output(){
    ADC0_PSSI_R = 0x8;// initiate SS3
    while((ADC0_RIS_R & 0x08) == 0){};// make sure an interrupt hasnt occured
    int result = ADC0_SSFIFO3_R & 0xFFF; // Read result from SS3
    ADC0_ISC_R = 0x8; //clear bit in this register and also ADCRIS register too
    return result;

}
