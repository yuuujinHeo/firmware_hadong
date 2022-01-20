#include "timer.h"


void InitTimer(){
	T1CONbits.TCS = 0; 		// Timer1 clock source is the internal clock Fcy = 60MHz
	T1CONbits.TGATE = 0;
	T1CONbits.TCKPS = 0b01; 	// Timer1 input clock  prescaler 1:8
	
	TMR1 = 0;			// clear Timer1 register 
//	PR1 = 7499;			// 60,000,000/8/7500=1K  for 1KHz interrupt
    PR1 = 4999;			// 40,000,000/8/5000=1K  for 1KHz interrupt
	
	IPC0bits.T1IP = 4; 	// interrupt priority 4
	
	IFS0bits.T1IF = 0;
	IEC0bits.T1IE = 1;
	T1CONbits.TON = 1;		// turn on timer 1 
}

void InitTimer2(){
    T2CONbits.TCS = 0;
    T2CONbits.TGATE = 0;
    T2CONbits.TCKPS = 0b10; // 64:1
    
    PR2 = 30000;
    T2CONbits.TON = 1;
}