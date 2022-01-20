#include "gpio.h"


void InitGPIO(){
	TRISBbits.TRISB0 = 0;   // HOT
    TRISAbits.TRISA0 = 0;   // SODA
    TRISAbits.TRISA1 = 0;   // COLD
    
    TRISBbits.TRISB7 = 0;   // DEBUG A
    TRISBbits.TRISB8 = 0;   // DEBUG B
    
    TRISBbits.TRISB1 = 0;   // ROBOT STOP
    TRISBbits.TRISB10 = 0;  // MCU for Hot Dispenser(Delight)
    TRISBbits.TRISB11 = 0;  // RELAY(Solenoid Valve)
}
