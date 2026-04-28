#include "scan.h"
#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include "Timer.h"

int ir_sample_count = 16;

void singleScan(scan_data* data, int angle, bool ir, bool ping) {

    //record the angle
    data->angle = angle;
    servo_move(angle);

    //get distance from ir sensor if enabled
    if (ir) {
        int avg_ir_val = 0;
        int i;
        for (i=0;i<ir_sample_count;i++) {
            avg_ir_val += adc_read();
        }
        avg_ir_val /= ir_sample_count;
        data->raw_ir_val = avg_ir_val;
    }

    //get distance from ping sensor if enabled
    if (ping) {
        timer_waitMillis(10);
        data->ping_distance = ping_getDistance();
    }

}

void scanField(int start_angle, int end_angle, int angle_step) {

    scan_data data;

    int angle;
    for (angle = start_angle; angle <= end_angle; angle += angle_step) {

        singleScan(&data, angle, 1, 1);
        //singleScan(angle, &scan_results);
        int current_ir_val = data.raw_ir_val;

        char message[70];
        snprintf(message, 70, "Angle: %3d      Distance: %3.2f      Raw IR Value: %4d\r\n", angle, data.ping_distance, current_ir_val);
        uart_sendStr(message);
    }

}
