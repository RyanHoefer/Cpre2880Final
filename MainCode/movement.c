#include "movement.h"
#include "open_interface.h"
#include "lcd.h"
#include "uart.h"
#include <string.h>

double move(oi_t *sensor_data, double distance_mm, int speed) {

    char uart_message[18];

    //if distance is positive move forward and check for bumps and cliffs
    if (distance_mm > 0) {
        double distance_traveled = 0;
        oi_setWheels(speed,speed);

        while((sensor_data->bumpRight == 0) && 
                (sensor_data->bumpLeft == 0) && 
                (distance_traveled < distance_mm) && 
                (sensor_data->cliffLeft == 0) && 
                (sensor_data->cliffFrontLeft == 0) && 
                (sensor_data->cliffFrontRight == 0) && 
                (sensor_data->cliffRight == 0)) {
            oi_update(sensor_data);

            //send distance traveled over uart
            snprintf(uart_message, 18, "Moved %4f mm\r\n", sensor_data->distance);
            uart_sendStr(uart_message);

            distance_traveled += sensor_data->distance;
            //lcd_printf("Traveled %f mm", distance_traveled);
        }
        oi_setWheels(0,0);

        return distance_traveled;
    }

    //if distance is negative move backward and check for cliffs
    else {
        double distance_traveled = distance_mm;
        oi_setWheels(speed*-1,speed*-1);

        while((distance_traveled > 0) && 
                (sensor_data->cliffLeft == 0) &&  
                (sensor_data->cliffRight == 0)) {
            oi_update(sensor_data);

            //send distance traveled over uart
            snprintf(uart_message, 18, "Moved %4f mm\r\n", sensor_data->distance);
            uart_sendStr(uart_message);

            distance_traveled += sensor_data->distance;
            // lcd_printf("Traveled %f mm", distance_traveled);
        }
        oi_setWheels(0,0);

        return distance_traveled;
    }

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

void turn_right(oi_t *sensor_data, double degrees) {

    char uart_message[12];

    double angle_turned = 0;
    oi_setWheels(-50,50);
       while(angle_turned<degrees) {
           oi_update(sensor_data);

           //send angle turned over uart
           snprintf(uart_message, 12, "right %3f\r\n", sensor_data->angle);
           uart_sendStr(uart_message);

           angle_turned -= sensor_data->angle;
           //lcd_printf("Turned %f degrees", angle_turned);
       }
       oi_setWheels(0,0);


}

void turn_left(oi_t *sensor_data, double degrees) {

    char uart_message[11];

    double angle_turned = 0;
    oi_setWheels(50,-50);
       while(angle_turned<degrees) {
           oi_update(sensor_data);

           //send angle turned over uart
           snprintf(uart_message, 11, "left %3f\r\n", sensor_data->angle);
           uart_sendStr(uart_message);

           angle_turned += sensor_data->angle;
           //lcd_printf("Turned %f degrees", angle_turned);
       }
       oi_setWheels(0,0);


}

