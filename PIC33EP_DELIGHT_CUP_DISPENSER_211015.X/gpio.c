#include "gpio.h"


void InitGPIO(){
    TRISAbits.TRISA1 = 1;   // SW0
    TRISAbits.TRISA3 = 1;   // SW1
    TRISAbits.TRISA4 = 1;   // SW2
    
    TRISBbits.TRISB14 = 1;  // LIMIT1
    TRISBbits.TRISB15 = 1;  // LIMIT2
    
    TRISBbits.TRISB10 = 0;  // Sonar Trigger
    TRISBbits.TRISB12 = 1;  // Sonar Data
    
    TRISBbits.TRISB4 = 0;   // Motor Enable
    TRISBbits.TRISB11 = 0;  // Motor High1
    TRISBbits.TRISB13 = 0;  // Motor High2
//    LATBbits.LATB11 = 0;
//    LATBbits.LATB13 = 0;
    
    TRISBbits.TRISB1 = 0;   // LED
    LATBbits.LATB1 = 1;
}


void InitInputCapture(){
    RPINR7bits.IC1R = 44;   // input capture RPI44 (RB12)
    
    IC1CON1bits.ICM = 1;
    IC1CON2bits.IC32 = 0;
    IC1CON1bits.ICI = 0;
    IC1CON1bits.ICTSEL = 0b001; // timer2
    
    IC1CON2bits.ICTRIG = 0; //  sync with timer
    IC1CON2bits.SYNCSEL = 0b01100; // timer 2
    
    IFS0bits.IC1IF = 0;
    IEC0bits.IC1IE = 1;
    IPC0bits.IC1IP = 5;
}