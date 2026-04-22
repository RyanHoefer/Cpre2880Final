/*
 * adc.h
 *
 *  Created on: Mar 19, 2026
 *      @author Ben Nowell
 */

#ifndef ADC_H_
#define ADC_H_

#include <inc/tm4c123gh6pm.h>
#include <stdint.h>

void adc_init(void);

uint16_t adc_read(void);

#endif
