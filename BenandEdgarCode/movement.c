#include "movement.h"
#include "open_interface.h"
#include "lcd.h"

double move(oi_t *sensor_data, double distance_mm, int speed) {

    if (distance_mm > 0) {
        double distance_traveled = 0;
        oi_setWheels(speed,speed);

        while((sensor_data->bumpRight == 0) && (sensor_data->bumpLeft == 0) && (distance_traveled < distance_mm)) {
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
    double angle_turned = 0;
    oi_setWheels(-50,50);
       while(angle_turned<degrees) {
           oi_update(sensor_data);
           angle_turned -= sensor_data->angle;
           lcd_printf("Turned %f degrees", angle_turned);
       }
       oi_setWheels(0,0);


}

void turn_left(oi_t *sensor_data, double degrees) {
    double angle_turned = 0;
    oi_setWheels(50,-50);
       while(angle_turned<degrees) {
           oi_update(sensor_data);
           angle_turned += sensor_data->angle;
           lcd_printf("Turned %f degrees", angle_turned);
       }
       oi_setWheels(0,0);


}

