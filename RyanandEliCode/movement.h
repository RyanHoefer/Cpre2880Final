/*
 * movement.h
 *
 *  Created on: Feb 4, 2026
 *      Author: rhoefer
 */

#ifndef MOVEMENT_H_
#define MOVEMENT_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inc/tm4c123gh6pm.h>
#include <open_interface.h>
#include "open_interface.h"


double move_backwards(oi_t *sensor_data, double distance_mm);
double move_forward(oi_t *sensor_data, double distance_mm);
//double move_forward_until_pressed(oi_t *sensor_data, double distance_mm);
void turn_right(oi_t *sensor_data, double degrees);
void turn_left(oi_t *sensor_data, double degrees);



#endif /* MOVEMENT_H_ */
