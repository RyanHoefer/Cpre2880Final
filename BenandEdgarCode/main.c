/*

@author Ben Nowell
This file contains most of the needed code for lab 3

*/

#include "Timer.h"
#include "open_interface.h"
#include "movement.h"
#include "lcd.h"
#include "cyBot_Scan.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>

#define MINIMUM_IR_VAL 615
#define ANGLE_STEP 1
#define MAX_OBJECTS 5
#define DETECTION_START_THRESHOLD 70
#define DETECTION_END_THRESHOLD 70

typedef struct {
    int object_id;
    int start_angle;
    int end_angle;
    int mid_angle;
    float distance;
    int radial_width;
    float linear_width;
} Object;

//get calibration values for servo
void calibrateServo(void) {
    timer_init();
    lcd_init();
    cyBOT_init_Scan(0b0001);
    cyBOT_SERVO_cal();
}

//function to test the servo calibration values
void testCalibration(void) {
    timer_init();
    lcd_init();
    cyBOT_init_Scan(0b0011);
    right_calibration_value = 248500;
    left_calibration_value = 1251250; //cybot 22

    int i;
    cyBOT_Scan_t scan;

    for (i=0; i<=180;i+=2) {
        cyBOT_Scan(i, &scan);
    }
}

int scanField(Object *objects) {
    int object_count = 0;
    bool in_object = false;
    cyBOT_Scan_t scan_results;

    //wipe all old object data
    int i;
    for (i=0; i<MAX_OBJECTS; i++) {
        objects[i].object_id = -1;
        objects[i].start_angle = -1;
        objects[i].end_angle = -1;
        objects[i].mid_angle = -1;
        objects[i].distance = -1;
        objects[i].radial_width = -1;
        objects[i].linear_width = -1;
    }

    int previous_ir_val = -1;
    int angle;
    for (angle = 0; angle <= 180; angle += ANGLE_STEP) {

        cyBOT_Scan(angle, &scan_results);
        int current_ir_val = scan_results.IR_raw_val;

        //if the IR raw value is less than 1000 we can be relatively confident there is nothing here so we dont need to waste time taking multiple samples
        if ((current_ir_val < MINIMUM_IR_VAL) && !in_object) {
            previous_ir_val = current_ir_val;
        }
        else {
            for (i=0; i<2; i++) {
            cyBOT_Scan(angle, &scan_results);
            current_ir_val += scan_results.IR_raw_val;
            }
            current_ir_val /= 3;
        }

        char message[70];
        snprintf(message, 70, "Angle: %3d      Distance: %3.2f      IR Raw Value: %4d\r\n", angle, scan_results.sound_dist, current_ir_val);
        uart_sendStr(message);

        //get initial reading for previous distance at the start of program
        if (previous_ir_val == -1.0) {
            previous_ir_val = current_ir_val;
            continue;
        }

        //DETECT START OF OBJECT
        //start scanning object if the difference between previous distance and current distance is greater than the detection threshold
        if ((current_ir_val - previous_ir_val) > DETECTION_START_THRESHOLD) {

            //add new object if not already in an object
            if (!in_object && (object_count < MAX_OBJECTS)) {
                in_object = true;
                char foundObj[30];
                snprintf(foundObj, 30, "Found an object\r\n");
                uart_sendStr(foundObj);
                objects[object_count].start_angle = angle;
                objects[object_count].object_id = object_count;
            }
        }

        //DETECT END OF OBJECT
        else if ((previous_ir_val - current_ir_val) > (DETECTION_END_THRESHOLD)) {
            //finish scanning object if the distance drops off
            if (in_object) {
                in_object = false;
                char lostObj[45];
                snprintf(lostObj, 45, "Stopped detecting object\r\n");
                uart_sendStr(lostObj);
                objects[object_count].end_angle = angle - ANGLE_STEP;
                objects[object_count].radial_width = objects[object_count].end_angle - objects[object_count].start_angle;
                objects[object_count].mid_angle = (objects[object_count].start_angle + objects[object_count].end_angle) / 2;

                object_count++;
            }
        }

        //update previous_dist for next loop iteration
        previous_ir_val = current_ir_val;
    }

    //if the scan ends on an object, perform the necessary steps to finish building the object
    if (in_object) {
        in_object = false;
        objects[object_count].end_angle = 180;
        objects[object_count].radial_width = objects[object_count].end_angle - objects[object_count].start_angle;
        objects[object_count].mid_angle = (objects[object_count].start_angle + objects[object_count].end_angle) / 2;
        object_count++;
    }

    //get the distance from the midpoint of each object
    for (i = 0; i < object_count; i++) {
        float distance = 0;
        float mindistance = 99999;
        int j, k;
        for (k=-2; k<2; k++) {
            for (j=0;j<10;j++) {
                cyBOT_Scan(objects[i].mid_angle, &scan_results);
                distance += scan_results.sound_dist;
            }
            distance /= 10;
            if (distance < mindistance) {
                mindistance = distance;
            }
        }
        objects[i].distance = mindistance;
    }

    //calculate the linear width of each object
    for (i=0; i<object_count; i++) {
        objects[i].linear_width = 2.0 * 3.14 * objects[i].distance * (objects[i].radial_width / 360.0);
    }


    for (i=0; i<object_count; i++) {
        //send string with information about objects to PuTTY
        char message[70];
        snprintf(message, 70, "Object #: %d, Angle: %d, Distance: %4.2f, Width: %f\r\n", objects[i].object_id, objects[i].mid_angle, objects[i].distance, objects[i].linear_width);
        uart_sendStr(message);
    }

    return object_count;
}

void mainPart1(void) {
    timer_init();
    lcd_init();

    Object objects[MAX_OBJECTS];

    //enable the servo and ping sensor
    cyBOT_init_Scan(0b0111);

    right_calibration_value = 248500;
    left_calibration_value = 1251250; //cybot 22

    //wait until an m is received over UART
    char receivedByte = 0;
    uart_init();
    lcd_printf("Waiting for UART Byte");
    while (receivedByte != 'm') {
        receivedByte = uart_receive();
    }

    int object_count = scanField(objects);
}

void mainPart2(void) {
    timer_init();
    lcd_init();

    Object objects[MAX_OBJECTS];

    //enable the servo and ping sensor
    cyBOT_init_Scan(0b0111);

    right_calibration_value = 248500;
    left_calibration_value = 1251250; //cybot 22

    //wait until an m is received over UART
    char receivedByte = 0;
    uart_init();
    lcd_printf("Waiting for UART Byte");
    while (receivedByte != 'm') {
        receivedByte = uart_receive();
    }

    int object_count = scanField(objects);

    int i;
    for (i=0; i<object_count; i++) {
        //send string with information about objects to PuTTY
        char message[70];
        snprintf(message, 70, "Object #: %d, Angle: %d, Distance: %4.2f, Width: %f\r\n", objects[i].object_id, objects[i].mid_angle, objects[i].distance, objects[i].linear_width);
        uart_sendStr(message);
    }
}

void mainPart3(void) {
    timer_init();
    lcd_init();

    Object objects[MAX_OBJECTS];

    //enable the servo and ping sensor
    cyBOT_init_Scan(0b0111);

    //right_calibration_value = 248500;
    //left_calibration_value = 1251250; cybot 22

    //right_calibration_value = 295750;
    //left_calibration_value = 1251250; //cybot 08

    right_calibration_value = 290500;
    left_calibration_value = 1293250; //cybot 14

    //wait until an m is received over UART
    char receivedByte = 0;
    uart_init();
    lcd_printf("Waiting for UART Byte");
    while (receivedByte != 'm') {
        receivedByte = uart_receive();
    }

    bool complete = false;
    int scan_count = 0;
    double previous_distance;
    while (1) {

        //If we have reached the target object or exceeded the max number of scans, exit the program
        if (complete || (scan_count >= 3)) {
            break;
        }

        //Perform the initial scan
        int object_count = scanField(objects);

        //find narrowest object
        int i, smallestWidthIndex = 0;
        if (objects[0].linear_width < 0.01) {
            smallestWidthIndex = 1;
        }
        for (i=0;i<object_count;i++) {
            if ((objects[i].linear_width < objects[smallestWidthIndex].linear_width) && (objects[i].linear_width > 0.01) && (objects[i].distance < 90) && scan_count == 0) {
                    smallestWidthIndex = i;
            }
            else if ((objects[i].distance < objects[smallestWidthIndex].distance) && (scan_count > 0)) {
                smallestWidthIndex = i;
            }
        }

        //Initialize connection with roomba
        lcd_printf("Initializing roomba connection");
        oi_t *sensor_data = oi_alloc();
        oi_init(sensor_data);

        //If the distance to the smallest width object is less than 10cm, we have already reached the target object and can exit the program
        if (objects[smallestWidthIndex].distance < 10) {
            complete = true;
            continue;
        }

        //point towards object with the smallest width
        char message[70];
        snprintf(message, 70, "Turning towards object %d\r\n", smallestWidthIndex);
        uart_sendStr(message);
        if (objects[smallestWidthIndex].mid_angle < 90) {
            turn_right(sensor_data, 90 - objects[smallestWidthIndex].mid_angle);
        }
        else {
            turn_left(sensor_data, objects[smallestWidthIndex].mid_angle - 90);
        }

        //Drive towards object with the smallest width
        snprintf(message, 70, "Moving %d mm\r\n", (int)(objects[smallestWidthIndex].distance * 10));
        uart_sendStr(message);
        int distance_travelled = move(sensor_data, (int)(objects[smallestWidthIndex].distance * 10) - 150,75);
        snprintf(message, 70, "Moved %d mm\r\n", (int)(objects[smallestWidthIndex].distance * 10));
        uart_sendStr(message);

        //If the bump sensors are triggered, drive around the object
        if ((sensor_data->bumpLeft == 1) || (sensor_data->bumpRight == 1)) {
            if ((sensor_data->bumpLeft == 1) && (sensor_data->bumpRight == 0)) {
                snprintf(message, 70, "Moved %d mm\r\n", move(sensor_data, -50, 100));
                //move(sensor_data, -50, 100);
                turn_right(sensor_data, 90);
                move(sensor_data, 150, 100);
                turn_left(sensor_data, 90);
                move(sensor_data, 400, 100);
                turn_left(sensor_data, 90);
            }
            else if ((sensor_data->bumpLeft == 0) && (sensor_data->bumpRight == 1)) {
                snprintf(message, 70, "Moved %d mm\r\n", move(sensor_data, -50, 100));
                move(sensor_data, -50, 100);
                turn_left(sensor_data, 90);
                move(sensor_data, 150, 100);
                turn_right(sensor_data, 90);
                move(sensor_data, 400, 100);
                turn_right(sensor_data, 90);
            }
            else {
                snprintf(message, 70, "Moved %d mm\r\n", move(sensor_data, -50, 100));
                move(sensor_data, -50, 100);
                turn_right(sensor_data, 90);
                move(sensor_data, 150, 100);
                turn_left(sensor_data, 90);
                move(sensor_data, 400, 100);
                turn_left(sensor_data, 90);
            }
        }

        //If no bump sensors were triggered, we can assume that we have reached the target object and can exit the program
        else {
            complete = true;
        }

        oi_free(sensor_data);
        scan_count++;
    }

}

main(void) {
    mainPart3();

    return 0;
}
