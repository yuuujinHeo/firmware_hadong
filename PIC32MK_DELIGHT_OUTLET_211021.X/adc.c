//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>
#include <sys/attribs.h>
#include <math.h>

#include "adc.h"
#include "can.h"
#include "board_definition.h"


int temperature_board_celsius_x10 = 0;
int temperature_motor_celsius_x10 = 0;
int temperature_board_celsius_x1 = 0;
int temperature_motor_celsius_x1 = 0;
   


const static float Rn_val_motor = 2000.0;
const static float B_val_motor = 3470.0;
const static float Tn_val_motor = 298.0;

const static float Rn_val_board = 2200.0;
const static float B_val_board = 3940.0;
const static float Tn_val_board = 298.0;


void InitADC(int trig_mode){    
    ADC0CFG = DEVADC0;
    ADC1CFG = DEVADC1;
    ADC2CFG = DEVADC2;
//    ADC3CFG = DEVADC3;
//    ADC7CFG = DEVADC7;
    
    ADCCON1 = 0;
    ADCCON2 = 0;
    ADCCON2bits.ADCDIV = 1;         // ADC7 clock freq is half of control clock = TAD7
    ADCCON2bits.SAMC = 5;           // ADC7 sampling time = 5 * TAD7
    ADCCON1bits.SELRES = 0b11;      // ADC7 resolution is 12 bits
    ADCCON1bits.STRGSRC = 1;        // Global Software Trigger (GSWTRG) is self-cleared on
    
    ADCANCON = 0;
    ADCANCONbits.WKUPCLKCNT = 5;    // wake up clock = 2^5*Tad
    
    ADCCON3 = 0;
    ADCCON3bits.ADCSEL = 0b00;      // PBCLK5 source
    ADCCON3bits.CONCLKDIV = 0;      // Tq = (CONCLKDIV*2)*Tclk
    ADCCON3bits.VREFSEL = 0;        // SET Vref = basic

    // AN0 -----
    ADC0TIMEbits.ADCDIV = 1;        // Tad = (ADCDIV*2)*Tq
    ADC0TIMEbits.SAMC = 2;          // Sampling time = (SAMC+2)*Tad
    ADC0TIMEbits.SELRES = 0b11;     // resolution 12bit
    
    ADCTRGMODEbits.SH0ALT = 0;      // AN0
    ADCIMCON1bits.SIGN0 = 0;        // AN0 is using unsigned data mode
    ADCIMCON1bits.DIFF0 = 0;        // AN0 is using single ended mode
    
    // AN1 -----
    ADC1TIMEbits.ADCDIV = 1;        // Tad = (ADCDIV*2)*Tq
    ADC1TIMEbits.SAMC = 2;          // Sampling time = (SAMC+2)*Tad
    ADC1TIMEbits.SELRES = 0b11;     // resolution 12bit
    
    ADCTRGMODEbits.SH1ALT = 0;      // AN1
    ADCIMCON1bits.SIGN1 = 0;        // AN1 is using unsigned data mode
    ADCIMCON1bits.DIFF1 = 0;        // AN1 is using single ended mode
      
    // AN2 -----
    ADC2TIMEbits.ADCDIV = 1;        // Tad = (ADCDIV*2)*Tq
    ADC2TIMEbits.SAMC = 2;          // Sampling time = (SAMC+2)*Tad
    ADC2TIMEbits.SELRES = 0b11;     // resolution 12bit
    
    ADCTRGMODEbits.SH2ALT = 0;      // AN2
    ADCIMCON1bits.SIGN2 = 0;        // AN2 is using unsigned data mode
    ADCIMCON1bits.DIFF2 = 0;        // AN2 is using single ended mode
    
    // AN3 -----
//    ADC3TIMEbits.ADCDIV = 1;        // Tad = (ADCDIV*2)*Tq
//    ADC3TIMEbits.SAMC = 5;          // Sampling time = (SAMC+2)*Tad
//    ADC3TIMEbits.SELRES = 0b11;     // resolution 12bit
//    
//    ADCTRGMODEbits.SH3ALT = 0;      // AN3
//    ADCIMCON1bits.SIGN3 = 0;        // AN3 is using unsigned data mode
//    ADCIMCON1bits.DIFF3 = 0;        // AN3 is using single ended mode
    
    
    ADCGIRQEN1 = 0;                 // no interrupt
    ADCGIRQEN2 = 0;                 // no interrupt
//    ADCGIRQEN1bits.AGIEN0 = 1;      // Enable data ready interrupt for AN0
//    ADCGIRQEN1bits.AGIEN1 = 1;      // Enable data ready interrupt for AN1
//    ADCGIRQEN1bits.AGIEN2 = 1;      // Enable data ready interrupt for AN2
//    ADCGIRQEN1bits.AGIEN3 = 1;      // Enable data ready interrupt for AN3
//    ADCGIRQEN1bits.AGIEN4 = 1;      // Enable data ready interrupt for AN4
//    ADCGIRQEN1bits.AGIEN5 = 1;      // Enable data ready interrupt for AN5

    
    
    ADCCSS1 = 0;
    ADCCSS2 = 0;
    ADCCSS1bits.CSS0 = 1;
    ADCCSS1bits.CSS1 = 1;
    ADCCSS1bits.CSS2 = 1;
//    ADCCSS1bits.CSS3 = 1;
//    ADCCSS2bits.CSS51 = 1;
    
    
    ADCCMPCON1 = 0;                 // no digital comparators
    ADCCMPCON2 = 0;                 // no digital comparators
    ADCCMPCON3 = 0;                 // no digital comparators
    ADCCMPCON4 = 0;                 // no digital comparators    
    ADCFLTR1 = 0;                   // no oversampling filters
    ADCFLTR2 = 0;                   // no oversampling filters
    ADCFLTR3 = 0;                   // no oversampling filters
    ADCFLTR4 = 0;                   // no oversampling filters

    
    ADCTRGSNSbits.LVL0 = 0;         // edge trigger
    ADCTRGSNSbits.LVL1 = 0;         // edge trigger
    ADCTRGSNSbits.LVL2 = 0;         // edge trigger
    ADCTRGSNSbits.LVL3 = 0;         // edge trigger
    
    if(trig_mode == 0){
        ADCTRG1bits.TRGSRC0 = 1;  // AN0 Software edge trigger
        ADCTRG1bits.TRGSRC1 = 1;  // AN1 Software edge trigger
        ADCTRG1bits.TRGSRC2 = 1;  // AN2 Software edge trigger
    }else{
        ADCTRG1bits.TRGSRC0 = 0b01010;  // AN0 PWM1 trigger
        ADCTRG1bits.TRGSRC1 = 0b01011;  // AN1 PWM2 trigger
        ADCTRG1bits.TRGSRC2 = 0b01100;  // AN2 PWM3 trigger
    }    
    ADCTRG1bits.TRGSRC3 = 1;        // AN3 Software edge trigger
    
    
    ADCEIEN1 = 0;                   // no early interrupt
    ADCEIEN2 = 0;

    // -------------------------------------------------- //
    
    ADCCON1bits.ON = 1;             // On
    
    
    while(!ADCCON2bits.BGVRRDY);    // Wait until the reference voltage is ready
    while(ADCCON2bits.REFFLT);      // Wait if there is a fault with the reference voltage
    
    
    ADCANCONbits.ANEN0 = 1;         // ADC0 enable the clock to analog bias
    ADCANCONbits.ANEN1 = 1;         // ADC1 enable the clock to analog bias
    ADCANCONbits.ANEN2 = 1;         // ADC2 enable the clock to analog bias
//    ADCANCONbits.ANEN3 = 1;         // ADC3 enable the clock to analog bias
//    ADCANCONbits.ANEN7 = 1;
    
        
    while(!ADCANCONbits.WKRDY0);    // ADC0 wait until module is ready
    while(!ADCANCONbits.WKRDY1);    // ADC1 wait until module is ready
    while(!ADCANCONbits.WKRDY2);    // ADC2 wait until module is ready
//    while(!ADCANCONbits.WKRDY3);    // ADC3 wait until module is ready
//    while(!ADCANCONbits.WKRDY7);
    
    
    ADCCON3bits.DIGEN0 = 1;         // ADC0 enable
    ADCCON3bits.DIGEN1 = 1;         // ADC1 enable
    ADCCON3bits.DIGEN2 = 1;         // ADC2 enable
//    ADCCON3bits.DIGEN3 = 1;         // ADC3 enable
//    ADCCON3bits.DIGEN7 = 1;
    
    
    // set the interrupt -----
//    IPC23bits.AD1IP = 3;
//    IPC23bits.AD1IS = 3;
//    IFS2bits.AD1IF = 0;
//    IEC2bits.AD1IE = 1;
    
//    IPC26bits.AD1G1IP = 7;
//    IPC26bits.AD1G1IS = 4;
//    IFS3bits.AD1GIF = 0;
//    IEC3bits.AD1GIE = 1;
    
}



void Handler_ADC_Trigger(unsigned long cnt){
    static unsigned long prev_cnt = 0;
    static unsigned long trigger_cnt = 0;
    float R_val;
    int tempdigit;
    
    
    
    if(cnt - prev_cnt > 0){
        prev_cnt = cnt;
    }else{
        return;
    }
    
    
    
    ++trigger_cnt;
    if(trigger_cnt%1000 == 0){
        SET_ADC_TRIGGER
    }else if(trigger_cnt%1000 == 500){
#ifdef BOARD_TYPE_14
        if(ADCDSTAT1bits.ARDY3){
            tempdigit = ADCDATA3;
            R_val = tempdigit * 11000 / (14895 - tempdigit);	// 14895=12*4096/3.3   ---   11kOhm
            temperature_board_celsius_x10 = (int)(B_val_board*10.0 / (logf(R_val/Rn_val_board) + B_val_board/Tn_val_board)) - 2730;
            temperature_board_celsius_x1 = temperature_board_celsius_x10/10;
            if(temperature_board_celsius_x1 > 127){
                temperature_board_celsius_x1 = 127;
            }
        }
//        if(ADCDSTAT1bits.ARDY4){
//            tempdigit = ADCDATA4;
//            R_val = tempdigit * 10000 / (14895 - tempdigit);	// 14895=12*4096/3.3   ---   10kOhm
//            temperature_motor_celsius_x10 = (int)(B_val_motor*10.0 / (logf(R_val/Rn_val_motor) + B_val_motor/Tn_val_motor)) - 2730;
//            temperature_motor_celsius_x1 = temperature_motor_celsius_x10/10;
//            if(temperature_motor_celsius_x1 > 127){
//                temperature_motor_celsius_x1 = 127;
//            }
//        }
#else
//        if(ADCDSTAT1bits.ARDY3){
//            tempdigit = ADCDATA3;
//            R_val = tempdigit * 10000 / (14895 - tempdigit);	// 14895=12*4096/3.3   ---   10kOhm
//            temperature_motor_celsius_x10 = (int)(B_val_motor*10.0 / (logf(R_val/Rn_val_motor) + B_val_motor/Tn_val_motor)) - 2730;
//            temperature_motor_celsius_x1 = temperature_motor_celsius_x10/10;
//            if(temperature_motor_celsius_x1 > 127){
//                temperature_motor_celsius_x1 = 127;
//            }
//        }
        if(ADCDSTAT1bits.ARDY3){
            tempdigit = ADCDATA3;
            R_val = tempdigit * 11000 / (14895 - tempdigit);	// 14895=12*4096/3.3   ---   11kOhm
            temperature_board_celsius_x10 = (int)(B_val_board*10.0 / (logf(R_val/Rn_val_board) + B_val_board/Tn_val_board)) - 2730;
            temperature_board_celsius_x1 = temperature_board_celsius_x10/10;
            if(temperature_board_celsius_x1 > 127){
                temperature_board_celsius_x1 = 127;
            }
        }
#endif
    }
}
