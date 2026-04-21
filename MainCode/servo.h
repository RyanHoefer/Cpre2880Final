/**
 * File for using the servo motor
 * @file servo.c
 * @author Ben Nowell
 */

#ifndef SERVO_H_
#define SERVO_H_

#include <stdint.h>

 void servo_init(void);

 int servo_move(uint16_t degrees);

 void servo_calibrate(void);

 #endif /* SERVO_H_ */
