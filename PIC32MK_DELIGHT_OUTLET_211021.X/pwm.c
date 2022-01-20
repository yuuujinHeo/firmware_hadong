//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>

#include "board_definition.h"
#include "pwm.h"


void InitPWM(){
    PMD4bits.PWM5MD = 1;
    PMD4bits.PWM6MD = 1;
    PMD4bits.PWM11MD = 1;
    PMD4bits.PWM12MD = 1;
        
    // PTPER = (FSYSCLK / (FPWM *(2^PTCON<PCLKDIV>)))
    // FPWM = 40kHz
//    PTCONbits.PCLKDIV = 0;
//    PTPER = 3000;
    
    // FPWM = 80kHz
    PTCONbits.PCLKDIV = 0;
    PTPER = PWM_RES;
    
    PWMCON1bits.ECAM = 0b00;    // edge-aligned mode
    PWMCON2bits.ECAM = 0b00;    // edge-aligned mode
    PWMCON3bits.ECAM = 0b00;    // edge-aligned mode
    PWMCON4bits.ECAM = 0b00;    // edge-aligned mode

    IOCON1bits.PMOD = 0b00;     // complementary
    IOCON1bits.PENH = 1;        // PWM (not GPIO)
    IOCON1bits.PENL = 1;        // PWM (not GPIO)
    IOCON1bits.FLTMOD = 0b11;   // fault disable
    IOCON1bits.POLH = 0;        // Active High
    IOCON1bits.POLL = 0;        // Active High
    
    IOCON2bits.PMOD = 0b00;     // complementary
    IOCON2bits.PENH = 1;        // PWM (not GPIO)
    IOCON2bits.PENL = 1;        // PWM (not GPIO)
    IOCON2bits.FLTMOD = 0b11;   // fault disable
    IOCON2bits.POLH = 0;        // Active High
    IOCON2bits.POLL = 0;        // Active High
    
    IOCON3bits.PMOD = 0b00;     // complementary
    IOCON3bits.PENH = 1;        // PWM (not GPIO)
    IOCON3bits.PENL = 1;        // PWM (not GPIO)
    IOCON3bits.FLTMOD = 0b11;   // fault disable
    IOCON3bits.POLH = 0;        // Active High
    IOCON3bits.POLL = 0;        // Active High
    
    IOCON4bits.PMOD = 0b00;     // complementary
    IOCON4bits.PENH = 1;        // PWM (not GPIO)
    IOCON4bits.PENL = 1;        // PWM (not GPIO)
    IOCON4bits.FLTMOD = 0b11;   // fault disable
    IOCON4bits.POLH = 0;        // Active High
    IOCON4bits.POLL = 0;        // Active High
    
    
    IOCON1bits.OVRENH = 0;  // pwm          - pulse
    IOCON1bits.OVRENL = 1;  // override     - pulse
    IOCON2bits.OVRENH = 0;  // pwm          - pulse
    IOCON2bits.OVRENL = 1;  // override     - pulse
    IOCON3bits.OVRENH = 0;  // pwm          - pulse
    IOCON3bits.OVRENL = 1;  // override     - pulse
    IOCON4bits.OVRENH = 0;  // pwm          - pulse
    IOCON4bits.OVRENL = 1;  // override     - pulse
    
    IOCON1bits.OVRDAT = 0b00;
    IOCON2bits.OVRDAT = 0b00;
    IOCON3bits.OVRDAT = 0b00;
    IOCON4bits.OVRDAT = 0b00;
    
    DTR1 = 20;          // 8*20 = 160ns
    DTR2 = 20;          // 8*20 = 160ns
    DTR3 = 20;          // 8*20 = 160ns
    DTR4 = 20;          // 8*20 = 160ns
    ALTDTR1 = 20;       // 8*20 = 160ns
    ALTDTR2 = 20;       // 8*20 = 160ns
    ALTDTR3 = 20;       // 8*20 = 160ns
    ALTDTR4 = 20;       // 8*20 = 160ns
    
    
    PTCONbits.PTEN = 1;
    TMR3 = 750;
    
    PDC1 = PWM_STOP;
    PDC2 = PWM_STOP;
    PDC3 = PWM_STOP;
    PDC4 = PWM_STOP;
}
