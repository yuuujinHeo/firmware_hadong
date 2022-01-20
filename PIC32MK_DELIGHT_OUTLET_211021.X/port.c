//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>
#include <sys/attribs.h>

#include "port.h"


void InitPort(){
    // All Analog Off
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;
    ANSELE = 0;
    ANSELG = 0;
    
    // ID
    TRISGbits.TRISG9 = 1;   // SW1
    TRISGbits.TRISG7 = 1;   // SW2
    TRISGbits.TRISG6 = 1;   // SW3
    
    // LED
    TRISEbits.TRISE15 = 0;  // LED_STATUS
    TRISAbits.TRISA12 = 0;  // LED_STAT
        
    // UltraSonic
    TRISFbits.TRISF0 = 0;   // UltraSonic Trig
    TRISFbits.TRISF1 = 1;   // UltraSonic Data
    
    // Encoder
    TRISCbits.TRISC8 = 1;   // ENC1 B  // ENC1 A
    TRISCbits.TRISC9 = 1;   // ENC2 A  // ENC1 B
    TRISCbits.TRISC6 = 1;   // ENC2 B  // ENC2 A
    TRISCbits.TRISC7 = 1;   // ENC1 A  // ENC2 B
    
    // Motor Fault
    TRISEbits.TRISE13 = 1;    // Fault1
    TRISEbits.TRISE12 = 1;    // Fault2
    
    // Motor PWM
    TRISBbits.TRISB14 = 0;  // M1 PWM1H
    TRISBbits.TRISB15 = 0;  // M1 PWM1L
    TRISBbits.TRISB12 = 0;  // M1 PWM2H
    TRISBbits.TRISB13 = 0;  // M1 PWM2L
    TRISBbits.TRISB10 = 0;  // M2 PWM1H
    TRISBbits.TRISB11 = 0;  // M2 PWM1L
    TRISAbits.TRISA10 = 0;  // M2 PWM2H
    TRISAbits.TRISA7 = 0;   // M2 PWM2L
    
    // Motor Enable
    TRISAbits.TRISA11 = 0;  // M1 Gate Enable
    TRISGbits.TRISG8 = 0;   // M2 Gate Enable
    
    // Limit Home Switch
    TRISBbits.TRISB4 = 1;   // Limit1
    TRISDbits.TRISD5 = 1;   // Limit2
    TRISDbits.TRISD6 = 1;   // Limit3
    
    // CAN
    TRISCbits.TRISC0 = 0;   // CAN Tx
    TRISCbits.TRISC1 = 1;   // CAN Rx
    
    // SPI
    TRISBbits.TRISB1 = 0;   // MOSI
    TRISBbits.TRISB3 = 1;   // MISO
    TRISBbits.TRISB7 = 0;   // CLK
    TRISEbits.TRISE12 = 0;  // CS (EERAM)
    
        
    // Analog
    TRISAbits.TRISA0 = 1;   // Current M1
    TRISAbits.TRISA1 = 1;   // Current M2
    TRISBbits.TRISB0 = 1;   // Temperature
    TRISBbits.TRISB2 = 1;   // Voltage
    ANSELAbits.ANSA0 = 1;
    ANSELAbits.ANSA1 = 1;
    ANSELBbits.ANSB0 = 1;
    ANSELBbits.ANSB2 = 1;
}
