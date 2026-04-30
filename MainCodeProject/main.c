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
     * b=66
     * c=67
     * d=68
     * e=69
     * f=70
     */
    unsigned char notes[4] = {64,60,64,60};
    unsigned char duration[4] = {30,15,30,15};

    oi_loadSong(1,4, notes, duration);
    oi_play_song(1);
}

int pizzaTemp(int seconds) {
    int temp = 165 - (seconds / 5);
    if (temp < 72) {
        temp = 72;
    }
    return temp;
}

void debugCliffSensor() {
    timer_init();
    lcd_init();
    uart_init();
    servo_init();
    adc_init();
    ping_init();

    oi_t *sensor_data = oi_alloc();
    oi_init(sensor_data);

    //front left range: 1000 - 2200
    //left range: 1000 - 2500
    //right range: 2500 - 2800
    //front right range: 1000 - 2200


    while (true) {
        oi_update(sensor_data);
        lcd_printf("Cliff Front Right Value: %d", sensor_data->cliffFrontRightSignal);
    }

}


int mainCode(){
    timer_init();
    lcd_init();
    uart_init();
    servo_init();
    adc_init();
    ping_init();

    unsigned int start_ms = timer_getMillis();

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
            unsigned int end_ms = timer_getMillis();
            unsigned int totalTimeMs = end_ms - start_ms;
            int totalTime = (int) (totalTimeMs / 1000);
            int totalTimeMinutes = totalTime / 60;
            int totalTimeSeconds = totalTime % 60;

            int temp = pizzaTemp(totalTime);

            const char *quality;
            if (temp >= 140) {
                quality = "Still hot!";
            }
            else if (temp >= 110) {
                quality = "Warm";
            }
            else if (temp >= 90) {
                quality = "Lukewarm";
            }
            else {
                quality = "Cold";
            }
            //lcd_printf("%dM%dS | %F\n%s", totalTimeMinutes, totalTimeSeconds, temp, quality);

            char buffer[128];
            sprintf(buffer,
                    "-Delivery Report-\n"
                    "Time: %dM %dS\n"
                    "Temp: %dF [%s]\n",
                    totalTimeMinutes, totalTimeSeconds, temp, quality);
            lcd_printf(buffer);
            sprintf(buffer,
                "\r-Delivery Report-\r\n"
                "Time: %dM %dS\r\n"
                "Temp: %dF [%s]\r\n",
                totalTimeMinutes, totalTimeSeconds, temp, quality);
            uart_sendStr(buffer);
            break;
        }

        //display message on LCD
        else if (uart_command == 'i') {
            lcd_printf("Delivered pizza!");
            playSong();
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





int main(void) {
    //playSong();
    mainCode();
    //Calibrate();
    //debugCliffSensor();
}
