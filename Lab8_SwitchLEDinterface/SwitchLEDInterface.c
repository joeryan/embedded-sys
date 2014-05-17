// ***** 0. Documentation Section *****
// SwitchLEDInterface.c for Lab 8
// Runs on LM4F120/TM4C123
// Use simple programming structures in C to toggle an LED
// while a button is pressed and turn the LED on when the
// button is released.  This lab requires external hardware
// to be wired to the LaunchPad using the prototyping board.
// January 11, 2014

// Lab 8
//      Jon Valvano and Ramesh Yerraballi
//      November 21, 2013

// ***** 1. Pre-processor Directives Section *****
#include "TExaS.h"
#include "tm4c123gh6pm.h"
#define PE0 (*((volatile unsigned long *)0x40024004))
#define PE1 (*((volatile unsigned long *)0x40024008))
// ***** 2. Global Declarations Section *****

// FUNCTION PROTOTYPES: Each subroutine defined
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void PortE_init(void); // initialize inputs/outputs
void toggle(void); // toggle light to oposite state
void led_on(void); // turn led on
void delay(unsigned long time); // provide a time*5ms delay function 
// ***** 3. Subroutines Section *****

// PE0, PB0, or PA2 connected to positive logic momentary switch using 10 k ohm pull down resistor
// PE1, PB1, or PA3 connected to positive logic LED through 470 ohm current limiting resistor
// To avoid damaging your hardware, ensure that your circuits match the schematic
// shown in Lab8_artist.sch (PCB Artist schematic file) or 
// Lab8_artist.pdf (compatible with many various readers like Adobe Acrobat).
unsigned long in, out;
int main(void){ 
//**********************************************************************
// The following version tests input on PE0 and output on PE1
//**********************************************************************
  TExaS_Init(SW_PIN_PE0, LED_PIN_PE1);  // activate grader and set system clock to 80 MHz
  
	PortE_init();
  EnableInterrupts();           // enable interrupts for the grader
	led_on(); //start with LED illuminated
  while(1){
		delay(20); // delay 100ms for each read of switch
		in = PE0;
    if(in == 0x01) {
			toggle();
		}
  }
}

void PortE_init(void) { volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x00000010;     // 1) E clock
  delay = SYSCTL_RCGC2_R;           // delay   
  // GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock PortF PF0 PortE unlock not required
  GPIO_PORTE_CR_R |= 0x2F;           // allow changes to PE5-0       
  GPIO_PORTE_AMSEL_R &= 0x00;        // 3) disable analog function
  GPIO_PORTE_PCTL_R &= 0x00000000;   // 4) GPIO clear bit PCTL  
  GPIO_PORTE_DIR_R &= ~0x01;          // 5.1) PE0 input, 
  GPIO_PORTE_DIR_R |= 0x02;          // 5.2) PE1 output  
  GPIO_PORTE_AFSEL_R &= 0x00;        // 6) no alternate function
  // GPIO_PORTF_PUR_R |= 0x11;        // pullup resistors nnot required
  GPIO_PORTE_DEN_R |= 0x03;          // 7) enable digital pins PE1-PE0 
}

void led_on(void) {
	PE1 = 0x02; // output PE1 high (led on)
}

void delay(unsigned long time){ // provides time*5ms delay
  unsigned long i;
  while(time > 0){
    i = 66665;
    while(i > 0){
      i = i - 1;
    }
    time = time - 1;
  }
}
void toggle(void) {
	PE1 = PE1^0x08;
}
