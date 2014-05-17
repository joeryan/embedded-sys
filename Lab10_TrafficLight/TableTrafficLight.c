// ***** 0. Documentation Section *****
// TableTrafficLight.c for Lab 10
// Runs on LM4F120/TM4C123
// Index implementation of a Moore finite state machine to operate a traffic light.  
// Daniel Valvano, Jonathan Valvano
// November 7, 2013

// east/west red light connected to PB5
// east/west yellow light connected to PB4
// east/west green light connected to PB3
// north/south facing red light connected to PB2
// north/south facing yellow light connected to PB1
// north/south facing green light connected to PB0
// pedestrian detector connected to PE2 (1=pedestrian present)
// north/south car detector connected to PE1 (1=car present)
// east/west car detector connected to PE0 (1=car present)
// "walk" light connected to PF3 (built-in green LED)
// "don't walk" light connected to PF1 (built-in red LED)

// ***** 1. Pre-processor Directives Section *****
#include "TExaS.h"
#include "tm4c123gh6pm.h"
#include "PLL.h"
#include "SysTick.h"

#define GPIO_PORTB_OUT          (*((volatile unsigned long *)0x400050FC)) // bits 5-0
#define GPIO_PORTB_DIR_R        (*((volatile unsigned long *)0x40005400))
#define GPIO_PORTB_AFSEL_R      (*((volatile unsigned long *)0x40005420))
#define GPIO_PORTB_DEN_R        (*((volatile unsigned long *)0x4000551C))
#define GPIO_PORTB_AMSEL_R      (*((volatile unsigned long *)0x40005528))
#define GPIO_PORTB_PCTL_R       (*((volatile unsigned long *)0x4000552C))

#define GPIO_PORTE_IN           (*((volatile unsigned long *)0x4002401C)) // bits 2-0
#define GPIO_PORTE_DIR_R        (*((volatile unsigned long *)0x40024400))
#define GPIO_PORTE_AFSEL_R      (*((volatile unsigned long *)0x40024420))
#define GPIO_PORTE_DEN_R        (*((volatile unsigned long *)0x4002451C))
#define GPIO_PORTE_AMSEL_R      (*((volatile unsigned long *)0x40024528))
#define GPIO_PORTE_PCTL_R       (*((volatile unsigned long *)0x4002452C))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC2_GPIOE      0x00000010  // port E Clock Gating Control
#define SYSCTL_RCGC2_GPIOB      0x00000002  // port B Clock Gating Control

#define DONT_WALK								(*((volatile unsigned long *)0x40025008))
#define GPIO_PORTF_OUT					(*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_DIR_R				(*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R			(*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_DEN_R				(*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_AMSEL_R 			(*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R				(*((volatile unsigned long *)0x4002552C))
#define GPIO_PORTF_LOCK_R				(*((volatile unsigned long *)0x40025520))
	
// SysTick definititions
#define NVIC_ST_CTRL_R      (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R    (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R   (*((volatile unsigned long *)0xE000E018))
	
// ***** 2. Global Declarations Section *****
struct State {
  unsigned long OutPB;
	unsigned long OutPF;
  unsigned long Time;  
  unsigned long Next[8];}; 
typedef const struct State STyp;
#define goW   0
#define waitW 1
#define goN   2
#define waitN 3
#define goP		4
#define waitP 5

	
STyp FSM[6]={
 {0x0C,0x02,300,{waitW,goW,waitW,waitW,waitW,waitW,waitW,waitW}},//State 0 goW
 {0x14,0x02, 50,{goN,goW,goN,goN,goP,goP,goN,goP}},							//State 1 waW
 {0x21,0x02,300,{waitN,waitN,goN,waitN,waitN,waitN,waitN,waitN}},//State 2 goN
 {0x22,0x02, 50,{goP,goW,goN,goW,goP,goP,goP,goW}},							//State 3 waN
 {0x24,0x08,300,{waitP,waitP,waitP,waitP,goP,waitP,waitP,waitP}},//State 4 goP
 {0x24,0x02, 5,{goW,goW,goN,goW,goP,goW,goN,goN}}};							//State 5 waP
 
unsigned long CurrState;  // index to the current state 
unsigned long Input; 
 
// FUNCTION PROTOTYPES: Each subroutine defined
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

void flash_waitP(void); // flash wait light for pedestrians
// ***** 3. Subroutines Section *****

int main(void){ unsigned long delayme;
  TExaS_Init(SW_PIN_PE210, LED_PIN_PB543210); // activate grader and set system clock to 80 MHz
  SYSCTL_RCGC2_R |= 0x32;      // 1) B E F
	GPIO_PORTF_LOCK_R = 0X4C4F434B; // unlock port F  ??
  delayme = SYSCTL_RCGC2_R;      // 2) no need to unlock
  GPIO_PORTE_AMSEL_R &= ~0x03; // 3) disable analog function on PE1-0
  GPIO_PORTE_PCTL_R &= ~0x000000FF; // 4) enable regular GPIO
  GPIO_PORTE_DIR_R &= ~0x07;   // 5) inputs on PE2-0
  GPIO_PORTE_AFSEL_R &= ~0x07; // 6) regular function on PE1-0
  GPIO_PORTE_DEN_R |= 0x07;    // 7) enable digital on PE1-0
  
	GPIO_PORTB_AMSEL_R &= ~0x3F; // 3) disable analog function on PB5-0
  GPIO_PORTB_PCTL_R &= ~0x00FFFFFF; // 4) enable regular GPIO
  GPIO_PORTB_DIR_R |= 0x3F;    // 5) outputs on PB5-0
  GPIO_PORTB_AFSEL_R &= ~0x3F; // 6) regular function on PB5-0
  GPIO_PORTB_DEN_R |= 0x3F;    // 7) enable digital on PB5-0

	GPIO_PORTF_AMSEL_R &= ~0x0A; // 3) disable analog on PF1 & PF3
	GPIO_PORTF_PCTL_R &= 0x00FFFFFF; // 4) enable regular GPIO
	GPIO_PORTF_DIR_R |= 0x0A; // 5) outputs on PF1 & PF3
	GPIO_PORTF_AFSEL_R &= ~0x0A; // 6)  regular function PF1 & PF3
	GPIO_PORTF_DEN_R |= 0x0A; // 7) enable digital on PF1 & PF3
	
  CurrState = goW;  
	PLL_Init2();
	SysTick_Init();
	
  EnableInterrupts();
  while(1){
    GPIO_PORTB_OUT = FSM[CurrState].OutPB; // set current street lights
		GPIO_PORTF_OUT = FSM[CurrState].OutPF; // set pedestrian light
		if (CurrState == waitP) { flash_waitP(); }; // toggle if waitP
		SysTick_Wait10ms(FSM[CurrState].Time); // delay in current state
		Input = GPIO_PORTE_IN & 0x07;
		CurrState = FSM[CurrState].Next[Input]; // get input from switches
		
  }
}


void flash_waitP(void) { // flashes wait signal 4 times for Pedestrians
	unsigned long time; 
	unsigned char i;
	time = 50; // set up flash to occur in .5 sec increments
	
	for (i = 0;i<4;i++) {
		DONT_WALK = DONT_WALK^0x02;// toggle ^0x02
		SysTick_Wait10ms(time);
	}
}
 