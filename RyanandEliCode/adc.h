

/*
 * adc.h
 *
 *  Created on: Mar 25, 2026
 *      Author: rhoefer
 */

#ifndef ADC_H_
#define ADC_H_


#include <inc/tm4c123gh6pm.h>

void init_ADC();
int ADC_get_output();
float convert_from_raw_to_cm(int rawVal);

#endif /* ADC_H_ */
