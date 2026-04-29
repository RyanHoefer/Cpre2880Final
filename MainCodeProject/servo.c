/**
 * File for using the servo motor
 * @file servo.c
 * @author Ben Nowell
 */

#include "servo.h"
#include "Timer.h"
#include "lcd.h"
#include "button.h"
#include <stdint.h>

//standard values are 16000 (1ms match) for 0 degrees (right), 32000 (2ms) for 180 degrees

//these are the values for cybot 17
int right_calibration_value = 7490;
int left_calibration_value = 35230;
int current_angle = 90;

 void servo_init(void) {

    //enable the clock to GPIO Port B
    SYSCTL_RCGCGPIO_R |= 0x2;

    //wait for the GPIO port to be initialized
    while((SYSCTL_PRGPIO_R & 0x02) == 0){};

    //enable the clock for timer 1
    SYSCTL_RCGCTIMER_R |= 0x2;

    //wait for the timer to be initialized
    while ((SYSCTL_RCGCTIMER_R & 0x2) == 0){};

    //enable digital functionality on pin PB5
    GPIO_PORTB_DEN_R |= 0x20;

    //set pin PB5 as an output
    GPIO_PORTB_DIR_R |= 0x20;

    //enable alternate functionality on pin PB5
    GPIO_PORTB_AFSEL_R |= 0x20;

    //set timer 1 to control pin PB5
    GPIO_PORTB_PCTL_R |=0x700000;

    //disable timer 1B while we configure it
    TIMER1_CTL_R &= ~0x100;

    //configure timer 1B for 16 bit mode
    TIMER1_CFG_R &= ~0x7;
    TIMER1_CFG_R |= 0x4;

    //set timer 1B to PWM mode
    TIMER1_TBMR_R |= 0x8;
    TIMER1_TBMR_R &= ~0x7;
    TIMER1_TBMR_R |= 0x2;

    //set the period of the pwm cycle
    TIMER1_TBPR_R = 0x4;
    TIMER1_TBILR_R = 0xE200;

    //set intitial position of 90 degrees
    TIMER1_TBPMR_R = 0x4;
    TIMER1_TBMATCHR_R = 0x8420;

    //reenable timer
    TIMER1_CTL_R |= 0x100;
 }

 int servo_move(uint16_t degrees) {

    if (degrees > 180) degrees = 180;
    if (degrees < 0) degrees = 0;

    //find the width of the high pulse
    int pulse_width = right_calibration_value + (degrees * ((left_calibration_value - right_calibration_value) / 180));

    //calculate what the match value should be
    int match_value = 320000 - pulse_width;

    //set prescaler and match value
    TIMER1_TBPMR_R = match_value / 65536; 
    TIMER1_TBMATCHR_R = match_value % 65536;

    int wait_time = abs(current_angle - degrees) * 10;
    if (wait_time < 20) {
        wait_time = 40;
    }
    timer_waitMillis(wait_time);

    current_angle = degrees;
     
     return match_value;
 }

 void servo_calibrate(void) {

    int current_match = 296000; //90 degrees
    uint8_t button = 0;

    lcd_init();
    timer_init();
     button_init();
    lcd_printf("Move servo all the way to the right");

    while (button != 2) {
        button = button_getButton();
        
        //move to the right
        if (button == 4) {
            current_match += 10;
        }

        //move to the left
        else if (button == 3) {
            current_match -= 10;
        }

        TIMER1_TBPMR_R = current_match / 65536; 
        TIMER1_TBMATCHR_R = current_match % 65536;

        timer_waitMillis(50);
    }
    int right_match = 320000 - current_match;
    button = 0;
    timer_waitMillis(1000);

    lcd_printf("Move servo all the way to the left");

    while(button != 2) {
        button = button_getButton();

        //move to the right
        if (button == 4) {
            current_match += 10;
        }
        
        //move to the left
        else if (button == 3) {
            current_match -= 10;
        }

        TIMER1_TBPMR_R = current_match / 65536; 
        TIMER1_TBMATCHR_R = current_match % 65536;

        timer_waitMillis(50);
    }
    int left_match = 320000 - current_match;

    lcd_printf("Right Value: %d\nLeft Value: %d", right_match, left_match);
 }
