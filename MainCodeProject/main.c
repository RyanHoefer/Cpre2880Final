
#include "adc.h"
#include "button.h"
#include "movement.h"
#include "open_interface.h"
#include "ping.h"
#include "scan.h"
#include "servo.h"
#include "Timer.h"
#include "uart.h"
#include "lcd.h"
#include <inc/tm4c123gh6pm.h>
#include <string.h>
#include <stdio.h>


int main(){
    timer_init();
    lcd_init();
    uart_init();
    servo_init();
    adc_init();
    ping_init();

    oi_t *sensor_data = oi_alloc();
    oi_init(sensor_data);

    move(sensor_data, 1000, 100);

    //scanField(0, 180, 1);
}
