/*
 * scanning.h
 *
 *  Created on: Feb 11, 2026
 *      Author: rhoefer
 */

#ifndef SCANNING_H_
#define SCANNING_H_
#include "cyBot_scan.h"
#include <open_interface.h>
#include "open_interface.h"

void sendAStringToPuTTY(char string[]);
void scanIntermitently(int intermitentAngle, int angleDesired, cyBOT_Scan_t* scanner);
void moveToSmallestWidth(int angle, float distance);
void go_around_object(oi_t *sensor_data);



#endif /* SCANNING_H_ */
