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

    char uart_command;
    while (true) {
        //check for command from PuTTY over UART
        uart_command = uart_receive_nonblocking();

        //move forward at various distances and speeds
        if (uart_command == 'q') {
            move(sensor_data, 100, 10);
        }
        else if (uart_command == 'w') {
            move(sensor_data, 250, 50);
        }
        else if (uart_command == 'e') {
            move(sensor_data, 500, 100);
        }

        //move backward at various distances and speeds
        else if (uart_command == 'z') {
            move(sensor_data, -100, 10);
        }
        else if (uart_command == 'x') {
            move(sensor_data, -250, 50);
        }
        else if (uart_command == 'c') {
            move(sensor_data, -500, 100);
        }

        //turn left at various angles
        else if (uart_command == 'a') {
            turn_left(sensor_data, 15);
        }
        else if (uart_command == 's') {
            turn_left(sensor_data, 45);
        }
        else if (uart_command == 'd') {
            turn_left(sensor_data, 90);
        }

        //turn right at various angles
        else if (uart_command == 'f') {
            turn_right(sensor_data, 15);
        }
        else if (uart_command == 'g') {
            turn_right(sensor_data, 45);
        }
        else if (uart_command == 'h') {
            turn_right(sensor_data, 90);
        }
        else if (uart_command == 'x') {
            oi_setWheels(0,0);
        }

        //scan and send data over uart
        else if (uart_command == 'p') {
            scan(sensor_data);
        }

        //exit program
        else if (uart_command == 'o') {
            break;
        }

        //display message on LCD
        else if (uart_command == 'i') {
            lcd_printf("Delivered pizza!");
        }
    }
}
