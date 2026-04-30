//#include "adc.h"
//#include "button.h"
//#include "movement.h"
//#include "open_interface.h"
//#include "ping.h"
//#include "scan.h"
//#include "servo.h"
//#include "Timer.h"
//#include "uart.h"
//#include "lcd.h"
//#include <inc/tm4c123gh6pm.h>
//#include <string.h>
//#include <stdio.h>
//
////165 is completely arbitrary, but seems right
//int pizza_temp(int elapsed_seconds) {
//    int temp = 165 - (elapsed_seconds / 3);
//    if (temp < 72) temp = 72;
//    return temp;
//}
//
//int mainCode(){
//    timer_init();
//    lcd_init();
//    uart_init();
//    servo_init();
//    adc_init();
//    ping_init();
//
//    unsigned int start_ms = timer_getMillis();
//
//    oi_t *sensor_data = oi_alloc();
//    oi_init(sensor_data);
//    servo_move(90);
//
//    char uart_command;
//    while (true) {
//        //check for command from PuTTY over UART
//        uart_command = uart_receive_nonblocking();
//
//        //move forward at various distances and speeds
//        if (uart_command == 'q') {
//            move(sensor_data, 100, 100);
//        }
//        else if (uart_command == 'w') {
//            move(sensor_data, 250, 100);
//        }
//        else if (uart_command == 'e') {
//            move(sensor_data, 500, 100);
//        }
//
//        //move backward at various distances and speeds
//        else if (uart_command == 'z') {
//            move(sensor_data, -100, 100);
//        }
//        else if (uart_command == 'x') {
//            move(sensor_data, -250, 100);
//        }
//        else if (uart_command == 'c') {
//            move(sensor_data, -500, 100);
//        }
//
//        //turn left at various angles
//        else if (uart_command == 'a') {
//            turn_left(sensor_data, 15);
//        }
//        else if (uart_command == 's') {
//            turn_left(sensor_data, 45);
//        }
//        else if (uart_command == 'd') {
//            turn_left(sensor_data, 90);
//        }
//
//        //turn right at various angles
//        else if (uart_command == 'f') {
//            turn_right(sensor_data, 15);
//        }
//        else if (uart_command == 'g') {
//            turn_right(sensor_data, 45);
//        }
//        else if (uart_command == 'h') {
//            turn_right(sensor_data, 90);
//        }
//        else if (uart_command == 'x') {
//            oi_setWheels(0,0);
//        }
//
//        //scan and send data over uart
//        else if (uart_command == 'p') {
//            scanField(0, 180, 1);
//        }
//
//        //exit program
//        else if (uart_command == 'o') {
//            unsigned int end_ms = timer_getMillis();
//            unsigned int elapsed_ms = end_ms - start_ms;
//            int elapsed_sec = (int)(elapsed_ms / 1000);
//            int minutes = elapsed_sec / 60;
//            int seconds = elapsed_sec % 60;
//
//            int temp = pizza_temp(elapsed_sec);
//
//            const char *quality;
//            if (temp >= 140) quality = "Still hot!";
//            else if (temp >= 110) quality = "Warm";
//            else if (temp >= 90) quality = "Lukewarm...";
//            else quality = "Cold pizza :(";
//            lcd_printf("%dm%ds | %dF\n%s", minutes, seconds, temp, quality);
//
//            char buf[128];
//            sprintf(buf,
//                "\r\n--- Delivery Report ---\r\n"
//                "Raw time:  %u ms (%dm %ds)\r\n"
//                "Pizza temp: %dF  [%s]\r\n"
//                "-----------------------\r\n",
//                elapsed_ms, minutes, seconds, temp, quality);
//            uart_sendStr(buf);
//
//            break;
//        }
//
//        //display message on LCD
//        else if (uart_command == 'i') {
//            lcd_printf("Delivered pizza!");
//        }
//    }
//    return 0;
//}
//
//void Calibrate(void) {
//    timer_init();
//    lcd_init();
//    uart_init();
//    servo_init();
//    adc_init();
//    ping_init();
//
//    servo_calibrate();
//}
//
//
//
//void playSong(){
//    /*
//     * g_3 = 57
//     * a_4 = 58
//     * b_4 = 59
//     * c_4 = 60
//     * d_4 = 61
//     * e_4 = 62
//     * f_4 = 63
//     * g_4 = 64
//     * a_5 = 65
//     */
//    unsigned char notes[5] = {60,63,64,61,58};
//    unsigned char duration[5] = {60,120,60,30,30};
//
//    oi_loadSong(1,5, notes, duration);
//    oi_play_song(1);
//
//
//
//}
//
//int main(void) {
//    //playSong();
//    mainCode();
//}
