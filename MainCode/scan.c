#include "scan.h"
#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include "Timer.h"

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

    int angle;
    for (angle = start_angle; angle <= end_angle; angle += ANGLE_STEP) {

        cyBOT_Scan(angle, &scan_results);
        int current_ir_val = scan_results.IR_raw_val;

        char message[70];
        snprintf(message, 70, "Angle: %3d      Distance: %3.2f      IR Raw Value: %4d\r\n", angle, scan_results.sound_dist, current_ir_val);
        uart_sendStr(message);
    }

}