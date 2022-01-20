#include "adc.h"


void InitADC(){
    TRISAbits.TRISA0 = 1;
    ANSELAbits.ANSA0 = 1; 
    
    /* Initialize and enable ADC module */
    AD1CON1 = 0x0000;
    AD1CON2 = 0x0000;
    AD1CON3 = 0x000F;
    AD1CON4 = 0x0000;
    AD1CHS0 = 0x0000;
    AD1CHS123 = 0x0000;
    AD1CSSH = 0x0000;
    AD1CSSL = 0x0000;
    AD1CON1bits.ADON = 1;
    int k;
    for(k=0; k<3000; k++)
        ;
}