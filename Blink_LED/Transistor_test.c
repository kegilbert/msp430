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
#define VIN_6 BIT6
#define BUTTON BIT3

void TransistorTest(void);

void TransistorTest(void) {
	unsigned int bSwitch = 0;

	VIN_DIR &= 0xF7;		// Pin 1.3 (button) is input
	VIN_DIR |= (VIN_0 + VIN_6);	// Pin 1.0 will be the input voltage to the transistor, 1.6 will be output LED corresponding w/ button
	VIN_DATA |= (VIN_0 + VIN_6);

	for(;;) {
		if ( !((P1IN)&(BIT3))  )
			bSwitch++;
		if(bSwitch%2 != 0)
			VIN_DATA |= (VIN_0 + VIN_6);
		else {
			VIN_DATA &= ~(VIN_0 + VIN_6);
		}
		__delay_cycles(150000);		// Works moderately well, but still allows errors when rapidly pressed or held too long, consider interrupts (External_LED.c)
	}
}

