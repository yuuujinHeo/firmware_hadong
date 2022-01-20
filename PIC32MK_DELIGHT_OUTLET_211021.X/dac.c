//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>


void InitDAC(){
    ANSELAbits.ANSA4 = 0;
    TRISAbits.TRISA4 = 0;
    ANSELCbits.ANSC10 = 0;
    TRISCbits.TRISC10 = 0;
    ANSELAbits.ANSA8 = 0;
    TRISAbits.TRISA8 = 0;

    
    DAC1CONbits.REFSEL = 0b11;
    DAC1CONbits.DACOE = 1;
    DAC1CONbits.ON = 1;
    
    DAC2CONbits.REFSEL = 0b11;
    DAC2CONbits.DACOE = 1;
    DAC2CONbits.ON = 1;
    
    DAC3CONbits.REFSEL = 0b11;
    DAC3CONbits.DACOE = 1;
    DAC3CONbits.ON = 1;
}    
    
