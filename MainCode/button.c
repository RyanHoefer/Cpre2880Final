/*
 * @author Ben Nowell
 * this file contains the functions needed for reading the push buttons for lab 4
 */



//The buttons are on PORTE 3:0
// GPIO_PORTE_DATA_R -- Name of the memory mapped register for GPIO Port E,
// which is connected to the push buttons
#include "button.h"

/**
 * Initialize PORTE and configure bits 0-3 to be used as inputs for the buttons.
 */
void button_init() {
	static uint8_t initialized = 0;

	//Check if already initialized
	if(initialized){
		return;
	}

	//set bit 4 to 1 to enable the clock for port E (0001 0000) using a bitwise OR
	SYSCTL_RCGCGPIO_R |= 0b00010000;

	//wait for port E to be initialized
	while ((SYSCTL_PRGPIO_R & 0b00010000) == 0) {};

	//set the direction of the first 4 bits for GPIO port E to be 0 for input using a bitwise AND
	GPIO_PORTE_DIR_R &= 0b11110000;

	//enable digital functionality for the first 4 bits for GPIO port E using a bitwise OR
	GPIO_PORTE_DEN_R |= 0b00001111;

	initialized = 1;
}



/**
 * Returns the position of the rightmost button being pushed.
 * @return the position of the rightmost button being pushed. 1 is the leftmost button, 4 is the rightmost button.  0 indicates no button being pressed
 */
uint8_t button_getButton() {

	//use bitwise AND to compare the value in the data register and 0b00001000 and check if the result is 0 (push buttons are 0 when pressed)
	if ((GPIO_PORTE_DATA_R & 0b00001000) == 0b00000000) {
		return 4;
	}

	else if ((GPIO_PORTE_DATA_R & 0b00000100) == 0b00000000) {
		return 3;
	}

	else if ((GPIO_PORTE_DATA_R & 0b00000010) == 0b00000000) {
		return 2;
	}

	else if ((GPIO_PORTE_DATA_R & 0b00000001) == 0b00000000) {
		return 1;
	}

	else {
		return 0;
	}

}
