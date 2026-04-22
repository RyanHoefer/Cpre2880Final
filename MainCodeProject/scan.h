#ifndef SCAN_H_
#define SCAN_H_

#include "adc.h"
#include "ping.h"
#include "servo.h"
#include "uart.h"

typedef struct {
    uint16_t raw_ir_val;
    float ping_distance;
    int angle;
} scan_data;

void singleScan(scan_data* data, int angle, bool ir, bool ping);

void scanField(int start_angle, int end_angle, int angle_step);

#endif
