/*
 * Transistor_test.c
 *
 *	Create test voltage from 1.0 toggled from pin 1.3 button
 *		to test transistors
 *
 *  Created on: Oct 1, 2012
 *      Author: Meow
 */
#include <msp430g2231.h>

#define VIN_DIR P1DIR
#define VIN_DATA P1OUT
#define VIN_0 BIT0
#define BUTTON BIT3

void TransistorTest(void);

void TransistorTest(void) {
	unsigned int counter=0;
	VIN_DIR |= VIN_0;		// Pin 1.0 will be the input voltage to the transistor
	VIN_DIR &= 0xF7;		// Pin 1.3 (button) is input

	for(;;) {
		if((VIN_DATA & 0x08) == 0x08) {
			counter++;
		}
		if((counter%2) != 0) {		// if counter is odd, button was pressed, toggle 1.0 on
			VIN_DATA |= 0x01;
		} else {
			VIN_DATA &= 0xFE;
		}
	}
}



