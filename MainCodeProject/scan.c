#include "scan.h"
#include <string.h>
#include <stdio.h>

void singleScan(scan_data* data, int angle, bool ir, bool ping) {

    //record the angle
    data->angle = angle;
    servo_move(angle);

    //get distance from ir sensor if enabled
    if (ir) {
        data->raw_ir_val = adc_read();
    }

    //get distance from ping sensor if enabled
    if (ping) {
        data->ping_distance = ping_getDistance();
    }

}

void scanField(int start_angle, int end_angle, int angle_step) {

    scan_data data;
    int angle;
        for (angle = start_angle; angle <= end_angle; angle += angle_step) {

            singleScan(&data, angle, true, true);

            char message[70];
            snprintf(message, 70, "Angle: %3d      Distance: %3.2f      Raw IR Value: %4d\r\n", angle, data.ping_distance, data.raw_ir_val);
            uart_sendStr(message);
        }

}
