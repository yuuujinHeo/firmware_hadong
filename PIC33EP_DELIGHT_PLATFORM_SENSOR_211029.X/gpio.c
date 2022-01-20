#include "gpio.h"


void InitGPIO(){
	TRISAbits.TRISA4 = 1;   // Flow Sensor A
    TRISAbits.TRISA3 = 0;   // Flow Sensor B
    
    TRISAbits.TRISA0 = 0;   // DEBUG A
    TRISAbits.TRISA1 = 0;   // DEBUG B
    
    TRISBbits.TRISB10 = 1;  // Platform Sensor A
    TRISBbits.TRISB11 = 1;  // Platform Sensor B
    
    TRISBbits.TRISB12 = 0;  // Pump A
    TRISBbits.TRISB13 = 0;  // Pump B
    
}
