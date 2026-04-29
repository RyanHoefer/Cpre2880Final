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


int mainCode(){
    timer_init();
    lcd_init();
    uart_init();
    servo_init();
    adc_init();
    ping_init();

    oi_t *sensor_data = oi_alloc();
    oi_init(sensor_data);

    servo_move(90);

    char uart_command;
    while (true) {
        //check for command from PuTTY over UART
        uart_command = uart_receive_nonblocking();

        //move forward at various distances and speeds
        if (uart_command == 'q') {
            move(sensor_data, 100, 100);
        }
        else if (uart_command == 'w') {
            move(sensor_data, 250, 100);
        }
        else if (uart_command == 'e') {
            move(sensor_data, 500, 100);
        }

        //move backward at various distances and speeds
        else if (uart_command == 'z') {
            move(sensor_data, -100, 100);
        }
        else if (uart_command == 'x') {
            move(sensor_data, -250, 100);
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
            scanField(0, 180, 1);
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
    return 0;
}

void Calibrate(void) {
    timer_init();
    lcd_init();
    uart_init();
    servo_init();
    adc_init();
    ping_init();

    servo_calibrate();
}



void playSong(){
    /*
     * g_3 = 57
     * a_4 = 58
     * b_4 = 59
     * c_4 = 60
     * d_4 = 61
     * e_4 = 62
     * f_4 = 63
     * g_4 = 64
     * a_5 = 65
     */
    unsigned char notes[5] = {60,63,64,61,58};
    unsigned char duration[5] = {60,120,60,30,30};

    oi_loadSong(1,5, notes, duration);
    oi_play_song(1);



}

int main(void) {
    //playSong();
    mainCode();
}
