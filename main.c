//******************************************************************************
//  MSP430G2955 - DCO Calibracion para el G2955 utilizando una señal externa de
//  reloj de 32 kHz, no un cristal.
//  ACLK = LFXT1/8 = 32768/8, MCLK = SMCLK = target DCO
//           MSP430G2x55
//         ---------------
//     /|\|            XIN|-32kHz
//      | |               |
//      --|RST            |
//        |               |
//        |           P4.3|--> LED
//        |           	  |
//
//  Xiaodong Li
//  Texas Instruments Inc.
//  March 2013
//  Built with CCS Version: 5.3.0 and IAR Embedded Workbench Version: 5.51
//******************************************************************************
#include <msp430.h>
#include <stdio.h>

#define DELTA_1MHZ    244                   // 244 x 4096Hz = 999.4Hz
#define DELTA_8MHZ    1953                  // 1953 x 4096Hz = 7.99MHz
#define DELTA_12MHZ   2930                  // 2930 x 4096Hz = 12.00MHz
#define DELTA_16MHZ   3906                  // 3906 x 4096Hz = 15.99MHz

unsigned char CAL_DATA[8];                  // Temp. storage for constants
volatile unsigned int i;
int j;
char *Flash_ptrA;                           // Segment A pointer
void Set_DCO(unsigned int Delta);

int main(void)
{

  WDTCTL = WDTPW | WDTHOLD;                 // Stop WDT
  __delay_cycles(721000);                   // Delay for XTAL stabilization


  BCSCTL3 |= LFXT1S_3 | XCAP_0;				//Seleccion de reloj externo, NO cristal.


  P4OUT = 0x00;                             // Clear P1 output latches
  P4DIR = BIT3 | BIT4;						// P4.3 P4.4 output

  j = 0;                                    // Reset pointer

  Set_DCO(DELTA_16MHZ);                     // Set DCO and obtain constants
  CAL_DATA[j++] = DCOCTL;
  CAL_DATA[j++] = BCSCTL1;

  P4OUT &= ~BIT3;
  Set_DCO(DELTA_12MHZ);                     // Set DCO and obtain constants
  CAL_DATA[j++] = DCOCTL;
  CAL_DATA[j++] = BCSCTL1;
  P4OUT |= BIT3;
  Set_DCO(DELTA_8MHZ);                      // Set DCO and obtain constants
  CAL_DATA[j++] = DCOCTL;
  CAL_DATA[j++] = BCSCTL1;
  P4OUT &= ~BIT3;
  Set_DCO(DELTA_1MHZ);                      // Set DCO and obtain constants
  CAL_DATA[j++] = DCOCTL;
  CAL_DATA[j++] = BCSCTL1;
  P4OUT |= BIT3;


  Flash_ptrA = (char *)0x10C0;              // Point to beginning of seg A
  FCTL2 = FWKEY | FSSEL0 | FN1;             // MCLK/3 for Flash Timing Generator
  FCTL1 = FWKEY | ERASE;                    // Set Erase bit
  FCTL3 = FWKEY | LOCKA;                    // Clear LOCK & LOCKA bits
  *Flash_ptrA = 0x00;                       // Dummy write to erase Flash seg A
  FCTL1 = FWKEY | WRT;                      // Set WRT bit for write operation
  Flash_ptrA = (char *)0x10F8;              // Point to beginning of cal consts
  for (j = 0; j < 8; j++)
    *Flash_ptrA++ = CAL_DATA[j];            // re-flash DCO calibration data
  FCTL1 = FWKEY;                            // Clear WRT bit
  FCTL3 = FWKEY | LOCKA | LOCK;             // Set LOCK & LOCKA bit


  P3SEL |= (BIT4 | BIT5) ;                     // P1.1 = RXD, P1.2=TXD
  	P3SEL2 &= ~(BIT4 | BIT5);                    // P1.1 = RXD, P1.2=TXD
  	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
  	UCA0BR0 = 104;                            // 1MHz 9600
  	UCA0BR1 = 0;                              // 1MHz 9600
  	UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
  	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  	IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt

  	while(1){
  		char str[40];
  		sprintf(str, "CALBC1_1MHZ = %x CALDCO_1MHZ = %x\n\r", CALBC1_1MHZ, CALDCO_1MHZ);
  		send_uart(str);
  		sprintf(str, "CALBC1_8MHZ = %x CALDCO_8MHZ = %x\n\r", CALBC1_8MHZ, CALDCO_8MHZ);
  		send_uart(str);
  		sprintf(str, "CALBC1_12MHZ = %x CALDCO_12MHZ = %x\n\r", CALBC1_12MHZ, CALDCO_12MHZ);
  		send_uart(str);
  		sprintf(str, "CALBC1_16MHZ = %x CALDCO_16MHZ = %x\n\r", CALBC1_16MHZ, CALDCO_16MHZ);
  		send_uart(str);
  		P4OUT ^= BIT3;
  		__delay_cycles(4250000);
  	}

}
void send_uart(char *data)
{
	unsigned int i;
	unsigned int size = strlen(data);      //get length of data to be sent
	for (i = 0; i < size; i++) {
		while (!(IFG2 & UCA0TXIFG));      //Wait UART to finish before next send
		UCA0TXBUF = data[i];
	}
}
void Set_DCO(unsigned int Delta)            // Set DCO to selected frequency
{
  unsigned int Compare, Oldcapture = 0;

  BCSCTL1 |= DIVA_3;                        // ACLK = LFXT1CLK/8
  TACCTL2 = CM_1 | CCIS_1 | CAP;            // CAP, ACLK
  TACTL = TASSEL_2 | MC_2 | TACLR;          // SMCLK, cont-mode, clear

  while (1)
  {
    while (!(CCIFG & TACCTL2));             // Wait until capture occured
    P4OUT ^= BIT4;
    TACCTL2 &= ~CCIFG;                      // Capture occured, clear flag
    Compare = TACCR2;                       // Get current captured SMCLK
    Compare = Compare - Oldcapture;         // SMCLK difference
    Oldcapture = TACCR2;                    // Save current captured SMCLK

    if (Delta == Compare)
      break;                                // If equal, leave "while(1)"
    else if (Delta < Compare)
    {
      DCOCTL--;                             // DCO is too fast, slow it down
      if (DCOCTL == 0xFF)                   // Did DCO roll under?
        if (BCSCTL1 & 0x0f)
          BCSCTL1--;                        // Select lower RSEL
    }
    else
    {
      DCOCTL++;                             // DCO is too slow, speed it up
      if (DCOCTL == 0x00)                   // Did DCO roll over?
        if ((BCSCTL1 & 0x0f) != 0x0f)
          BCSCTL1++;                        // Sel higher RSEL
    }
  }
  TACCTL2 = 0;                              // Stop TACCR2
  TACTL = 0;                                // Stop Timer_A
  BCSCTL1 &= ~DIVA_3;                       // ACLK = LFXT1CLK
}
