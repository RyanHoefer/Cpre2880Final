/*
 * servo.h
 *
 *  Created on: Apr 8, 2026
 *      Author: rhoefer
 */

#ifndef SERVO_H_
#define SERVO_H_



#include <stdint.h>
#include <stdbool.h>
#include <inc/tm4c123gh6pm.h>
#include "driverlib/interrupt.h"



/**
 * Initialize ping sensor. Uses PB3 and Timer 3B
 */
void servo_init(void);
void servo_move(uint16_t degrees);





#endif /* SERVO_H_ */
