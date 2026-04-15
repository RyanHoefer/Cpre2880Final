/**
 * lab4_template.c
 *
 * Template file for CprE 288 lab 4
 *
 * @author Zhao Zhang, Chad Nelson, Zachary Glanz
 * @date 08/14/2016
 */

#include "button.h"
#include "Timer.h"
#include "lcd.h"
#include "cyBot_uart.h"
#include "scanning.h"
                         // Functions for communicating between CyBot and Putty (via UART)
                         // PuTTy: Baud=115200, 8 data bits, No Flow Control, No Parity, COM1


#define REPLACEME 0

/*

int main(void) {
	button_init();
	timer_init(); // Must be called before lcd_init(), which uses timer functions
	lcd_init();
	cyBot_uart_init();


	// YOUR CODE HERE

	
	while(1)
	{

      uint8_t buttonPressed = button_getButton();

      if (buttonPressed == 1) {
          sendAStringToPuTTY("Hey you finally figured out how to press a button\n\r");
      }
      else if (buttonPressed == 2) {
          sendAStringToPuTTY("Now you're getting it. Keep up the good work\n\r");
      }
      else if (buttonPressed == 3) {
          sendAStringToPuTTY("Almost there! You can do it\n\r");
      }
      else if (buttonPressed == 4) {
          sendAStringToPuTTY("Congrats you reached the end! Heres your prize: :)\n\r");
      }
      else {
          sendAStringToPuTTY("Do something bro your not pressing anything\n\r");
      }

      timer_waitMicros(2000);

	}

}
*/
/*
void sendAStringToPuTTY(char string[]){
    char message[100];
    sprintf(message, "%s", string);
    int i;
    for(i = 0; i < strlen(message); i++){
        cyBot_sendByte(message[i]);
    }

}
*/
