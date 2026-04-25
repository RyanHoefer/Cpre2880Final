#include "movement.h"
#include "open_interface.h"
#include "lcd.h"
#include "uart.h"
#include <string.h>
#include "ping.h"
#include "servo.h"

double move(oi_t *sensor_data, double distance_mm, int speed) {

    char uart_message[18];

    //if distance is positive move forward and check for bumps, cliffs, and forward distance using the ping sensor
    if (distance_mm > 0) {
        double distance_traveled = 0;
        double last_update_dist = 0;
        int loop_iterations = 0;

        //get initial readings from ping sensor for forward object detection
        servo_move(90);
        double forward_distance = ping_getDistance();

        //start driving
        oi_setWheels(speed,speed);

        //check all sensors and continue to move forward
        while((sensor_data->bumpRight == 0) && 
                (sensor_data->bumpLeft == 0) && 
                (distance_traveled < distance_mm) && 
                (sensor_data->cliffLeft == 0) && 
                (sensor_data->cliffFrontLeft == 0) && 
                (sensor_data->cliffFrontRight == 0) && 
                (sensor_data->cliffRight == 0) &&
                (forward_distance > 15.0)) {

            //update sensors from roomba
            oi_update(sensor_data);

            //record distance travelled
            distance_traveled += sensor_data->distance;

            //use the ping sensor to avoid running into tall objects
            if (loop_iterations == 5) {
                forward_distance = ping_getDistance();
                loop_iterations = 0;
            }
            else {
                loop_iterations++;
            }

            //update distance over uart if the bot has travelled more than 20mm
            if (fabs(distance_traveled - last_update_dist) > 20.0) {
                snprintf(uart_message, 18, "Moved %.0f mm\r\n", distance_traveled - last_update_dist);
                uart_sendStr(uart_message);
                last_update_dist = distance_traveled;
            }
        }

        //stop driving
        oi_setWheels(0,0);

        //send final uart message
        snprintf(uart_message, 18, "Moved %.0f mm\r\n", distance_traveled - last_update_dist);
        uart_sendStr(uart_message);

        return distance_traveled;
    }

    //if distance is negative move backward and check for cliffs
    else {
        double distance_traveled = distance_mm;
        double last_update_dist = distance_mm;

        //start reversing
        oi_setWheels(speed*-1,speed*-1);

        //check sensors and continue reversing
        while((distance_traveled > 0) && 
                (sensor_data->cliffLeft == 0) &&  
                (sensor_data->cliffRight == 0)) {

            //update sensors from rooba
            oi_update(sensor_data);

            //record distance travelled
            distance_traveled += sensor_data->distance;

            //update distance over uart if the bot has travelled more than 20mm
            if (fabs(distance_traveled - last_update_dist) > 20.0) {
                snprintf(uart_message, 18, "Moved %.0f mm\r\n", distance_traveled - last_update_dist);
                uart_sendStr(uart_message);
                last_update_dist = distance_traveled;
            }
        }

        //stop reversing
        oi_setWheels(0,0);

        //send final uart message
        snprintf(uart_message, 18, "Moved %.0f mm\r\n", distance_traveled - last_update_dist);
        uart_sendStr(uart_message);

        return distance_traveled;
    }

}



void turn_right(oi_t *sensor_data, double degrees) {

    char uart_message[14];
    double angle_turned = 0;
    double last_update_angle = 0;

    //start turning
    oi_setWheels(-50,50);
    while(angle_turned<degrees) {

        //update sensors from roomba
        oi_update(sensor_data);

        //record angle turned
        angle_turned -= sensor_data->angle;

        //send angle turned over uart
        if (fabs(angle_turned - last_update_angle) > 5.0) {
            snprintf(uart_message, 14, "right %.0f\r\n", angle_turned - last_update_angle);
            uart_sendStr(uart_message);
            last_update_angle = angle_turned;
        }
    }

    //stop turning
    oi_setWheels(0,0);

    //send final uart message
    snprintf(uart_message, 14, "right %.0f\r\n", angle_turned - last_update_angle);
    uart_sendStr(uart_message);
}



void turn_left(oi_t *sensor_data, double degrees) {

    char uart_message[13];
    double angle_turned = 0;
    double last_update_angle = 0;

    //start turning
    oi_setWheels(50,-50);
    while(angle_turned<degrees) {

        //update sensors from roomba
        oi_update(sensor_data);

        //record angle turned
        angle_turned += sensor_data->angle;

        //send angle turned over uart
        if (fabs(angle_turned - last_update_angle) > 5.0) {
            snprintf(uart_message, 13, "left %.0f\r\n", angle_turned - last_update_angle);
            uart_sendStr(uart_message);
            last_update_angle = angle_turned;
        }
    }

    //stop turning
    oi_setWheels(0,0);

    //send final uart message
    snprintf(uart_message, 13, "left %.0f\r\n", angle_turned - last_update_angle);
    uart_sendStr(uart_message);
}



//move without checking for bumps or cliffs
double move_dumb(oi_t *sensor_data, double distance_mm, int speed) {

    if (distance_mm > 0) {
        double distance_traveled = 0;
        oi_setWheels(speed,speed);

        while(distance_traveled < distance_mm) {
            oi_update(sensor_data);
            distance_traveled += sensor_data->distance;
            lcd_printf("Traveled %f mm", distance_traveled);
        }
        oi_setWheels(0,0);

        return distance_traveled;
    }
    else {
        double distance_traveled = distance_mm;
        oi_setWheels(speed*-1,speed*-1);

        while(distance_traveled > 0) {
            oi_update(sensor_data);
            distance_traveled += sensor_data->distance;
            lcd_printf("Traveled %f mm", distance_traveled);
        }
        oi_setWheels(0,0);

        return distance_traveled;
    }
}