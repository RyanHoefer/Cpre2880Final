/**
 * lab5_template.c
 *
 * Template file for CprE 288 Lab 5
 *
 * @author Zhao Zhang, Chad Nelson, Zachary Glanz
 * @date 08/14/2016
 *
 * @author Phillip Jones, updated 6/4/2019
 * @author Diane Rover, updated 2/25/2021, 2/17/2022
 */

#include "button.h"
#include "timer.h"
#include "lcd.h"

#include "cyBot_uart.h"  // Functions for communicating between CyBot and Putty (via UART1)
                         // PuTTy: Baud=115200, 8 data bits, No Flow Control, No Parity, COM1

#include "cyBot_Scan.h"  // Scan using CyBot servo and sensors
#include "uart.h"


#define REPLACEME 0


/*
int main(void) {


  // initialize the cyBot UART1 before trying to use it

  // (Uncomment ME for UART init part of lab)
	// cyBot_uart_init_clean();  // Clean UART1 initialization, before running your UART1 GPIO init code

	// Complete this code for configuring the GPIO PORTB part of UART1 initialization (your UART1 GPIO init code)
     // SYSCTL_RCGCGPIO_R |= FIXME;
	   // while ((SYSCTL_PRGPIO_R & FIXME) == 0) {};
		 // GPIO_PORTB_DEN_R |= FIXME;
		 // GPIO_PORTB_AFSEL_R |= FIXME;
     // GPIO_PORTB_PCTL_R &= FIXME;     // Force 0's in the desired locations
     // GPIO_PORTB_PCTL_R |= FIXME;     // Force 1's in the desired locations
		 // Or see the notes for a coding alternative to assign a value to the PCTL field

    // (Uncomment ME for UART init part of lab)
		// cyBot_uart_init_last_half();  // Complete the UART device configuration

		// Initialize the scan
	  // cyBOT_init_Scan();
		// Remember servo calibration function and variables from Lab 3

	// YOUR CODE HERE
	button_init();
    timer_init(); // Must be called before lcd_init(), which uses timer functions
    lcd_init();
    uart_init();
    int numChars = 0;
    char message[100] = "";
    char recievedChar;
	while(1)
	{

	    recievedChar = uart_receive();

	    if (numChars >= 20 || recievedChar == '\r'){
	        lcd_printf("");

	        lcd_printf("%s", message);
	        memset(message, 0, sizeof(message));

	        lcd_printf(" ");

	        numChars = 0;
	        if(recievedChar == '\r'){
	            uart_sendChar(recievedChar);
	            uart_sendChar('\n');
	        }


	    }else {
	        message[numChars] = recievedChar;
	        lcd_printf("%s\n%d ", message, ++numChars);
	        uart_sendChar(recievedChar);



	    }



	}

}

*/
