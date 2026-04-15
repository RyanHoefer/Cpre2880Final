/*
 * scanning.c
 *
 *  Created on: Feb 11, 2026
 *      Author: rhoefer
 */

#include "movement.h"
#include "open_interface.h"
#include <stdlib.h>
#include <stdbool.h>
#include "Timer.h"
#include "lcd.h"

#include <stdio.h>
#include "scanning.h"
#include "cyBot_Scan.h"
#include "uart.h"

/*
int main(void){

    //timer_init(); // Initialize Timer, needed before any LCD screen functions can be called
                      // and enables time functions (e.g. timer_waitMillis)

    //lcd_init();   // Initialize the LCD screen.  This also clears the screen.

    //For Calibration
    // right_calibration_value = 269500;
    //left_calibration_value = 1209250;

    cyBOT_Scan_t* scanner = calloc(1, sizeof(cyBOT_Scan_t));

    //int intermittentAngle = 2;
    //int angleDesired = 90;
    //double distArr[(angleDesired/intermittentAngle) + 1];


    timer_init();
    lcd_init();
    uart_init();
    //cyBot_uart_init();
    cyBOT_init_Scan(0b0111);
    //cyBOT_SERVO_cal();

    //lcd_printf("%c",(char)cyBot_getByte());


    char message[25];
    int totalAngle = 0;
    float scannerVal = scanner -> sound_dist;
    sprintf(message, "%d\t %f\n\r", totalAngle, scannerVal);
    sendAStringToPuTTY(message);


    //cyBOT_Scan(90, scanner);

    //sendACharToPuTTY((char)cyBot_getByte());
    //cyBOT_Scan(180, scanner);
    scanIntermitently(2, 180, scanner);


  // return 0;


}
*/

void pointToSmallestObject(double *distances, int intermittentAngle){
    int i;
    double current = distances[0];
    double previous;
    int tracking = 0;
    int totalAngle = 0;
    int objNum = 0;


    typedef struct{
        int angleToCenter;
        double width;
    } object;

    object objArr[10];

    for(i = 1; (i < sizeof(distances) / sizeof(distances[0])); i ++){
        previous = distances[i-1];
        current = distances[i];
        totalAngle += intermittentAngle;
        if(((abs(previous - current)) > 10.0) && tracking == 0){
            tracking = 1;
        }
        else if(((abs(previous - current)) > 10.0) && tracking == 1){
            tracking = 0;
            objNum++;
        }

        if(tracking){
            objArr[i].width += intermittentAngle;
        }

    }
}


void sendAStringToPuTTY(char string[]){
    char message[100];
    sprintf(message, "%s", string);
    int i;
    for(i = 0; i < strlen(message); i++){
        uart_sendChar(message[i]);
    }

}



//mostly good, a little error with edge detection but mostly good, figure oit bug with the width calulation and good to go
void scanIntermitently(int intermitentAngle, int angleDesired, cyBOT_Scan_t* scanner){
    sendAStringToPuTTY("Degrees  PING distance (cm)  IR distance\n\r");
    char message[25];
    char message2[150];
    //char message3[25];
    int totalAngle = 0;
    float pingVal;
    double current = 0;
    double previous = 0;
    int currentInf = 0;
    int previousInf = 0;
    //int doublePreviousInf = 0;
    int objNum = 0;
    int tracking = 0;
    int infVal = 0;
    int j;
    int totalInfValue = 0;
    float  averageInfValue;
    const int differenceEpsilon = 150;
    int firstIteration = 1;
    int trackedLastTime = 0;
    int stoppedTrackingLastTime = 0;
    const float TURNING_ANGLE = 0.75;


    typedef struct{
            int angleToCenter;
            int width;
            int startAngle;
            int endAngle;
            float distance;
            float linearWidth;
        } object;

    object objArr[10];


    while(totalAngle < angleDesired){



        totalInfValue = 0; //reset the total value
        cyBOT_Scan(totalAngle, scanner);
        totalAngle += intermitentAngle;
        pingVal = scanner -> sound_dist;
        infVal = scanner-> IR_raw_val;
        //sprintf(message3,"%f", pingVal);

        sendAStringToPuTTY(message);
        totalInfValue += infVal;

        for(j = 0; j < 2; j++){
            cyBOT_Scan(totalAngle, scanner);
            infVal = scanner-> IR_raw_val;

            totalInfValue += infVal;
        }

        averageInfValue = totalInfValue / 3.0;

        //doublePreviousInf = previousInf;

        previous = current;
        previousInf = currentInf;

        current = pingVal;
        currentInf = averageInfValue;


        if((firstIteration<3)){
               firstIteration++;
               continue;
       }




            //                        epsilon here
        if (((abs(previousInf - currentInf)) > differenceEpsilon) && tracking == 0 && stoppedTrackingLastTime == 0 /*&& (infVal < 100.0)*/){
            tracking = 1;
            trackedLastTime = 1;

            objArr[objNum].startAngle = totalAngle;
            objArr[objNum].distance = pingVal;

        } else if(((abs(previousInf - currentInf)) > differenceEpsilon) && tracking == 1 && trackedLastTime == 0){
            tracking = 0;
            stoppedTrackingLastTime = 1;
            objArr[objNum].endAngle = totalAngle;
            objArr[objNum].width = objArr[objNum].endAngle - objArr[objNum].startAngle;
            //objArr[objNum].linearWidth = (tan(objArr[objNum].width)) * (objArr[objNum].distance);//issue here, not calculating the linear width correctly
            objArr[objNum].linearWidth = (objArr[objNum].width*(M_PI/180)) * objArr[objNum].distance;
            objArr[objNum].angleToCenter = (objArr[objNum].width / 2) + objArr[objNum].startAngle;


            //sprintf(message2, "Number: %d, Angle: %d, Distance %f,radial width %d, linear Width %f, tan function: %f\n\r", objNum, objArr[objNum].angleToCenter, objArr[objNum].distance,objArr[objNum].width, objArr[objNum].linearWidth, tan(objArr[objNum].width)  );
            sprintf(message2," Distance %f,radial width %d, linear Width %f, tan function: %f\n\r", objArr[objNum].distance,objArr[objNum].width, objArr[objNum].linearWidth, tan(objArr[objNum].width)  );
            objNum++;
            sendAStringToPuTTY(message2);
        }else{
            stoppedTrackingLastTime = 0;

            trackedLastTime = 0;
        }

        sprintf(message, "%d\t %f\t %f %d\t %d\t %d\n\r", totalAngle, pingVal, averageInfValue, previousInf, currentInf, tracking );

    }
    int i;
    float smallestWidth = objArr[0].linearWidth;
    float currentWidth = 0;
    int smallestObj = 0;
    for(i = 0; i < objNum; i++){
        currentWidth = objArr[i].linearWidth;
        if(currentWidth < smallestWidth) {
            smallestWidth = currentWidth;
            smallestObj = i;
        }
    }

    cyBOT_Scan(objArr[smallestObj].angleToCenter, scanner);
    char message3[25];
    sprintf(message3, "moving: %f mm\n\r", (objArr[smallestObj].distance) * 10);
    sendAStringToPuTTY(message3);
    //moveToSmallestWidth(objArr[smallestObj].angleToCenter, objArr[smallestObj].distance);
    moveToSmallestWidth((int)(objArr[smallestObj].angleToCenter * TURNING_ANGLE), (objArr[smallestObj].distance) * 10);


}
double move_forward_until_pressed(oi_t *sensor_data, double distance_mm){
    double totalDistanceTraveled = 0.0;

    while((totalDistanceTraveled <= distance_mm) && (sensor_data -> bumpLeft == 0 && sensor_data -> bumpRight == 0)){
        oi_setWheels(150,150);
        oi_update(sensor_data);
        totalDistanceTraveled += sensor_data -> distance;
        if(sensor_data -> bumpLeft != 0 || sensor_data -> bumpRight != 0){
            go_around_object(sensor_data);
            totalDistanceTraveled += 35.0;
        }
    }
    char message4[25];
        sprintf(message4, "stopped moving %lf mm\n\r", distance_mm);
        sendAStringToPuTTY(message4);
    oi_setWheels(0,0);
    return totalDistanceTraveled;
}


void moveToSmallestWidth(int angle, float distance){
    oi_t *sensor_data = oi_alloc();
    oi_init(sensor_data);
    float totalDistanceTraveled = 0;

    int turningAngle = angle - 90;
    if(turningAngle < 0){
        //turn right
        turn_right(sensor_data, abs(turningAngle));
    }else{
        //turn left
        turn_left(sensor_data, abs(turningAngle));

    }


    while(totalDistanceTraveled < distance){
        totalDistanceTraveled += move_forward_until_pressed(sensor_data, distance);
    }
    oi_free(sensor_data);


}



void go_around_object(oi_t *sensor_data){
    const int TURNING_ANGLE = 65;
            oi_update(sensor_data);
            if (sensor_data -> bumpLeft != 0) {
                move_backwards(sensor_data, 50);
                turn_right(sensor_data, TURNING_ANGLE);
                move_forward(sensor_data, 150);
                turn_left(sensor_data, TURNING_ANGLE);
                move_forward(sensor_data, 450);
                turn_left(sensor_data, TURNING_ANGLE);
                move_forward(sensor_data, 150);
                turn_right(sensor_data, TURNING_ANGLE);
            }
            else if (sensor_data -> bumpRight != 0) {
                move_backwards(sensor_data, 50);
                turn_left(sensor_data, TURNING_ANGLE);
                move_forward(sensor_data, 150);
                turn_right(sensor_data, TURNING_ANGLE);
                move_forward(sensor_data, 450);
                turn_right(sensor_data, TURNING_ANGLE);
                move_forward(sensor_data, 150);
                turn_left(sensor_data, TURNING_ANGLE);
            }
            oi_update(sensor_data);

}

 /*funciton below works for ping scanner
void scanIntermitently(int intermitentAngle, int angleDesired, cyBOT_Scan_t* scanner){
    sendAStringToPuTTY("Degrees  PING distance (cm)\n\r");
    char message[25];
    char message2[100];
    int totalAngle = 0;
    float pingVal;
    double current = 0;
    double previous = 0;
    int objNum = 0;
    int tracking = 0;
    typedef struct{
            int angleToCenter;
            int width;
            int startAngle;
            int endAngle;
            float distance;
        } object;

    object objArr[10];

    while(totalAngle < angleDesired){



        cyBOT_Scan(totalAngle, scanner);
        totalAngle += intermitentAngle;
        pingVal = scanner -> sound_dist;
        //infVal = scanner-> IR_raw_val;
        sprintf(message, "%d\t %f\n\r", totalAngle, pingVal);
        sendAStringToPuTTY(message);

        previous = current;
        current = pingVal;

        if (((abs(previous - current)) > 30.0) && tracking == 0 && (pingVal < 100.0)){
            tracking = 1;

            objArr[objNum].startAngle = totalAngle;
            objArr[objNum].distance = pingVal;

        }
        else if(((abs(previous - current)) > 30.0) && tracking == 1){
            tracking = 0;
            objArr[objNum].endAngle = totalAngle;
            objArr[objNum].width = objArr[objNum].endAngle - objArr[objNum].startAngle;
            objArr[objNum].angleToCenter = (objArr[objNum].width / 2) + objArr[objNum].startAngle;


            sprintf(message2, "Number: %d, Angle: %d, Distance %f, Width %d \n\r", objNum, objArr[objNum].angleToCenter, objArr[objNum].distance, objArr[objNum].width  );
            objNum++;
            sendAStringToPuTTY(message2);
        }

    }
    int i;
    int smallestWidth = objArr[0].width;
    int currentWidth = 0;
    int smallestObj = 0;
    for(i = 0; i < objNum; i++){
        currentWidth = objArr[i].width;
        if(currentWidth < smallestWidth) {
            smallestWidth = currentWidth;
            smallestObj = i;
        }
    }
    cyBOT_Scan(objArr[smallestObj].angleToCenter, scanner);
}
*/

