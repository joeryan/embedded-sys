// TuningFork.c Lab 12
// Runs on LM4F120/TM4C123
// Use SysTick interrupts to create a squarewave at 440Hz.  
// There is a positive logic switch connected to PA3, PB3, or PE3.
// There is an output on PA2, PB2, or PE2. The output is 
//   connected to headphones through a 1k resistor.
// The volume-limiting resistor can be any value from 680 to 2000 ohms
// The tone is initially off, when the switch goes from
// not touched to touched, the tone toggles on/off.
//                   |---------|               |---------|     
// Switch   ---------|         |---------------|         |------
//
//                    |-| |-| |-| |-| |-| |-| |-|
// Tone     ----------| |-| |-| |-| |-| |-| |-| |---------------
//
// Daniel Valvano, Jonathan Valvano
// March 8, 2014

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */


#include "TExaS.h"
#include "..//tm4c123gh6pm.h"

#define GPIO_PORTA_DATA_R       (*((volatile unsigned long *)0x400043FC))
#define GPIO_PORTA_DIR_R        (*((volatile unsigned long *)0x40004400))
#define GPIO_PORTA_AFSEL_R      (*((volatile unsigned long *)0x40004420))
#define GPIO_PORTA_DEN_R        (*((volatile unsigned long *)0x4000451C))
#define GPIO_PORTA_AMSEL_R      (*((volatile unsigned long *)0x40004528))
#define GPIO_PORTA_PCTL_R       (*((volatile unsigned long *)0x4000452C))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define NVIC_SYS_PRI3_R         (*((volatile unsigned long *)0xE000ED20))  // Sys. Handlers 12 to 15 Priority
#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
#define PA2 (*((volatile unsigned long *)0x40004010))
#define PA3 (*((volatile unsigned long *)0x40004020))

// basic functions defined at end of startup.s
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void WaitForInterrupt(void);  // low power mode

// global variable to show if sound on/off, and combat switch bounce
unsigned short WaveON;
unsigned long SwitchPrevious;

// input from PA3, output from PA2, SysTick interrupts
void Sound_Init(void){ 
	SYSCTL_RCGC2_R |= 0x00000001; // activate port A
	WaveON = 0;
	
  GPIO_PORTA_DIR_R |= 0x04;   // make PA2 output for headphones
  GPIO_PORTA_AFSEL_R &= ~0x0C;// disable alt funct on PA2/3
  GPIO_PORTA_DEN_R |= 0x0C;   // enable digital I/O on PA2/3
                              // configure PA3/2 as GPIO
  GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0xFFFF00FF)+0x00000000;
  GPIO_PORTA_AMSEL_R = 0;     // disable analog functionality on PA
	
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_RELOAD_R = 90908;// reload value to get an interrupt every 1.13636ms, 880Hz
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
                              // enable SysTick with core clock and interrupts
  NVIC_ST_CTRL_R = 0x07;
	
  EnableInterrupts();
}

// called at 400 Hz
void SysTick_Handler(void){ unsigned long SW1;

	SW1 = PA3; // get current switch state
	if (SW1 == 0x00000008) { // switch pressed
		if ( SwitchPrevious == 0) { // only toggle sound if this is a new switch press
		WaveON ^= 1; // Toggle sound
		} 
		SwitchPrevious = 1;
	} else {
		SwitchPrevious = 0;
	}
	if (WaveON == 1) {
		PA2 ^= 0x04;       // toggle PA2
	} else {
	  PA2 = 0x00;       // PA2 set to low
	}
}

int main(void){// activate grader and set system clock to 80 MHz
  TExaS_Init(SW_PIN_PA3, HEADPHONE_PIN_PA2,ScopeOn); 
  Sound_Init();         
  EnableInterrupts();   // enable after all initialization are done
  while(1){
    // main program is free to perform other tasks
    // do not use WaitForInterrupt() here, it may cause the TExaS to crash
  }
}
