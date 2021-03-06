/* uartTXCVRG2231.c: Full-duplex UART transceiver for the MSP430G2231.
 *
 *
 *  ::  Copyright 2012, MSPSCI
 *  ::  http://mspsci.blogspot.com
 *
 */

#include <msp430g2231.h>
#include "calibrations.h"

#define bit_time    768      			// 9600 baud
#define halfbit_time    384
#define SET        (OUTMOD_1 + CCIE)	// Timer_A sets on next interrupt
#define RST        (OUTMOD_5 + CCIE)	// Timer_A resets on next interrupt
#define IDLE        OUTMOD_1			// Timer_A stops interrupting
#define TXbit       BIT0				// Define bit in UART_FG to show Tx
#define RXbit		BIT1				//        bit in UART_FG to show Rx

#define LED1        BIT0				// Red LED on P1.0
#define LED2        BIT6				// Green LED on P1.6
#define BTN1		BIT3				// Button on P1.3
#define P1TX        BIT1        		// Tx on P1.1
#define P1RX		BIT2				// Rx on P1.2
#define P1I_MASK	BIT2 + BIT3			// Mask P1 interrupts to just P1.2 and P1.3

int TXBuffer;       		// Buffer for storing current transmission byte
int RXBuffer;		// Buffer for storing current receive byte
char UART_FG=0;     		// flags for UART: BIT0 is TX in process flag
char Tx_bit_count=10;  		// bit counter for transmission, 8N1 is 10 bits
char Rx_bit_count=10;		// bit counter for receive
char command='z';			// default command is sleep

/*  Function definitions  */
void DCO_init(void);
void P1_init(void);
void TA_init(void);
void tx_byte(char byte);
void tx_string(char *message);

void UARTtransceiver(void) {
    char *message;		// pointer to array to store messages to send.

    WDTCTL = WDTPW + WDTHOLD;
    DCO_init();
    P1_init();
    TA_init();
    __delay_cycles(1474560);	// Delay 0.2 s to let clocks settle
    _enable_interrupt();

    while(1) {
        switch(command) {
            case 'r':   P1OUT ^= LED1;	// toggle red
            			command='z';
            			message="Toggled Red LED.";
            			tx_string(message);
                        break;
            case 'g':   P1OUT ^= LED2;	// toggle green
            			command='z';
            			message="Toggled Grn LED.";
            			tx_string(message);
                        break;
            case 0xaf:	command='z';
            			message="Button Pressed!!";
            			tx_string(message);
            			break;
            case 'c':	message="Repeated Message";
            			tx_string(message);
            			break;
            case 'z':   message="Going to Sleep..";
            			tx_string(message);
            			_BIS_SR(LPM0_bits + GIE);
            			break;			// sleep
            default:    command='z';	// signal bad command, go to sleep
            			message="Invalid Command!";
            			tx_string(message);
                        break;
        }
    }

}	// main

void DCO_init(void) {
    char j;
    if ((CALBC1_UART == 0xFF) || (CALDCO_UART == 0xFF)) {
        /* DCO Calibrations not present! Trap processor to prevent damage.
         * Flash LED on P1.0 in patterns of three to indicate DCO error.
         */
        P1OUT=0;
        P1DIR=LED1;
        for(;;) {
            for (j=0; j<6; j++) {
                P1OUT ^= LED1;
                __delay_cycles(50000);
            }
            for (j=0; j<3; j++) {
                __delay_cycles(50000);
            }
        }
    } // end if for DCO trap.
    else {
        BCSCTL1 = CALBC1_UART;
        DCOCTL = CALDCO_UART;
    }
} // DCO_init

void P1_init(void) {
    P1OUT = P1TX;	// default to mark (idle)
    P1DIR = LED1 + LED2 + P1TX;
    P1SEL = P1TX;	// Set P1.1 to TA0.0 Output
    P1IES = P1RX + BTN1;	// interrupt on falling edge (start bit is high -> low)
    P1IFG = 0;		// clear interrupt flags before enabling
    P1IE = P1RX + BTN1;	// enable P1 interrupt on P1RX and BTN1
} // P1_init

void TA_init(void) {
    TACTL = TASSEL_2 + ID_0 + MC_2;     // SMCLK, DIV 1, Continuous
    TACCTL0 = IDLE;                     // Set TA0.0 at TACCR0, no interrupt
} // TA_init

void tx_byte(char byte) {
    while (UART_FG & TXbit);    // trap until free to transmit
    TXBuffer = (byte | 0xff00);	// prepare byte for transmission: add stop bit
    UART_FG |= TXbit;           // mark Tx flag
    Tx_bit_count--;				// count start bit
    TACCTL0 &= ~CCIFG;			// clear interupt flag
    TACCR0 = TAR + bit_time;	// set trigger for first bit
    TACCTL0 = RST;            	// reset for start bit
    __delay_cycles(bit_time);	// let byte start transmission before continuing
}

void tx_string(char *message) {
	int i;
	for (i=0; i<16; i++)
		tx_byte(message[i]);
	 tx_byte('\r');
	 tx_byte('\n');
} // tx_string


/*  Interrupt Service Routines  */
#pragma vector = PORT1_VECTOR
__interrupt void P1_ISR(void) {
	switch(P1IFG & P1I_MASK) {
	case P1RX:	TACCR1 = TAR + halfbit_time;
				P1IFG = 0;  // clear the interrupt flag
				P1IE &= ~P1RX;	// disable further interrupts until Rx done
				TACCTL1 = CCIE;	// enable CCR1 interrupts
				RXBuffer = 0;	// clear the Rx buffer
				Rx_bit_count = 10;	// reset bit counter
				UART_FG |= RXbit;	// mark Rx flag
				return;
	case BTN1:	P1IFG = 0;	// clear the interrupt flag
				command=0xaf;
				__low_power_mode_off_on_exit();
				return;
	default:	P1IFG = 0;	// clear any other interrupts
				return;
	}
} // P1_ISR

#pragma vector = TIMERA0_VECTOR
__interrupt void CCR0_ISR(void) {
    TACCR0 += bit_time;         // set next interrupt

    if (Tx_bit_count == 0) {	// Tx Complete
        TACCTL0 = IDLE;         // Idle TA0.0
        Tx_bit_count=10;        // reset bit counter
        UART_FG &= ~TXbit;      // clear Tx flag
    }
    else {
        if (TXBuffer & BIT0)
            TACCTL0 = SET;
        else
            TACCTL0 = RST;
        TXBuffer >>= 1;         // shift TXBuffer down one bit
        Tx_bit_count--;
    }
} // CCR0_ISR

#pragma vector = TIMERA1_VECTOR
__interrupt void CCR1_ISR(void) {
	switch(TAIV) {
		case 2:	TACCR1 += bit_time;						// set next bit time
				if (P1IN & P1RX)
					RXBuffer = (RXBuffer | 0x400) >> 1;	// if mark, set bit and shift
				else
					RXBuffer = RXBuffer >> 1;			// otherwise just shift.

				if (--Rx_bit_count==0) {
					TACCTL1 = 0;		// turn off timer interrupts
					command = (RXBuffer >> 1) & 0xff;	// extract received byte
					UART_FG &= ~RXbit;	// Rx done, clear flag
					P1IFG = 0;			// clear P1 interrupt flags
					P1IE |= P1RX;		// restart P1 interrupts to receive next byte
					__low_power_mode_off_on_exit();
				}
				break;
		default:	break;
	} // switch
} // TA1_ISR
