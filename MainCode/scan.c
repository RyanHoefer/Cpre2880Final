#include "scan.h"

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

void scanField(int start_angle, int end_angle) {



}