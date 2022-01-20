/*
 * File:   main.c
 * Author: rainbow
 *
 */


#include "xc.h"
#include "cpu_config.h"
#include "can.h"
#include "timer.h"
#include "gpio.h"
#include "adc.h"

#ifdef _HAS_DMA_
__eds__ ECAN1MSGBUF Ecan1msgBuf __attribute__( (eds, space(dma), aligned(ECAN1_MSG_BUF_LENGTH * 16)) );
#else
__eds__ ECAN1MSGBUF Ecan1msgBuf  __attribute__( (eds, space(xmemory), aligned(ECAN1_MSG_BUF_LENGTH * 16)) );
#endif


void ConfigOSC(void) {
    OSCCONbits.NOSC = 0b011; // EC with PLL

//    // M/(N1*N2)*F = 40/(4*2)*24M = 120M
//    PLLFBDbits.PLLDIV = 38; // M = PLLFBD + 2 = 40
//    CLKDIVbits.PLLPOST = 0; // N2 = 2*(PLLPOST+1) = 2
//    CLKDIVbits.PLLPRE = 2;  // N1 = PLLPRE+2 = 4
    
    // M/(N1*N2)*F = 20/(3*2)*24M = 80M
    PLLFBDbits.PLLDIV = 18; // M = PLLFBD + 2 = 20
    CLKDIVbits.PLLPOST = 0; // N2 = 2*(PLLPOST+1) = 2
    CLKDIVbits.PLLPRE = 1;  // N1 = PLLPRE+2 = 3

    // disable Watch Dog Timer
    RCONbits.SWDTEN = 0;

//    // Clock Switching
//    // oscillator select
//    __builtin_write_OSCCONH(0x03);
//    // clock Switch
//    __builtin_write_OSCCONL(OSCCON | 0x01);  //turn on OSWEN
//    
//    // wait until OSWEN to low
//    while (OSCCONbits.OSWEN); 
//    
//    // wait for Clock switch to occur
//    while (OSCCONbits.COSC!= 0b011);
    
    // wait for PLL to lock
    while (OSCCONbits.LOCK!= 1);
}



unsigned int rising_time, falling_time;
int falling_edge_rcvd = 0;
int rising_edge = 1;

unsigned short ultra_sonic_avg;
unsigned int ultra_sonic_state = 1;
unsigned int ultra_sonic_fail_count = 0;

static int InputCaptureCount = 0;
static int glitch_count = 0;
unsigned char ULTRA_SONIC_ERROR = 0;

// -------------------

//int motor_current = 0;


int CUP_DETECT = 0;

int CUP_DISPENSE_FLAG = 0;
int CUP_DISPENSE_MIN_TIME = 50;
int CUP_CHECK_COUNT = 10;


int CUP_DISPENSE_LAST_ERROR = 0;
enum{
    LAST_ERROR_SUCCESS = 0,
    LAST_ERROR_BUSY,
    LAST_ERROR_CUP_EXIST,
    LAST_ERROR_DISPENSE_FAIL,
    LAST_ERROR_TIMEOUT
};

static unsigned int CUP_OUT_COUNT = 0;

static void SendStatus();


int main(void) {
    ConfigOSC();
    // apply initial delay for stable operation ---
    volatile long initial_delay;
    for(initial_delay=0; initial_delay<600000; initial_delay++)
        ;
    volatile long initial_delay2 = 600000;
    while(initial_delay2--)
        ;
    // --------------------------------------------
    ConfigOSC();
    
    ANSELA = ANSELB = 0;

    TRISAbits.TRISA1 = 1;
    TRISAbits.TRISA3 = 1;
    TRISAbits.TRISA4 = 1;
    
    
    
    BNO = 0;
    
//    if(PORTAbits.RA1 == 1){
//        BNO += 1;
//    }
//    if(PORTAbits.RA3 == 1){
//        BNO += 2;
//    }
//    if(PORTAbits.RA4 == 1){
//        BNO += 4;
//    }
    
    if(PORTAbits.RA1 == 0 && PORTAbits.RA3 == 0 && PORTAbits.RA4 == 0){
        BNO = 0;
    }else if(PORTAbits.RA1 == 1 && PORTAbits.RA3 == 0 && PORTAbits.RA4 == 0){
        BNO = 1;
    }else if(PORTAbits.RA1 == 0 && PORTAbits.RA3 == 1 && PORTAbits.RA4 == 0){
        BNO = 2;
    }else if(PORTAbits.RA1 == 1 && PORTAbits.RA3 == 1 && PORTAbits.RA4 == 0){
        BNO = 3;
    }else if(PORTAbits.RA1 == 0 && PORTAbits.RA3 == 0 && PORTAbits.RA4 == 1){
        BNO = 4;
    }else if(PORTAbits.RA1 == 1 && PORTAbits.RA3 == 0 && PORTAbits.RA4 == 1){
        BNO = 5;
    }else if(PORTAbits.RA1 == 0 && PORTAbits.RA3 == 1 && PORTAbits.RA4 == 1){
        BNO = 6;
    }else if(PORTAbits.RA1 == 1 && PORTAbits.RA3 == 1 && PORTAbits.RA4 == 1){
        BNO = 7;
    }else{
        BNO = 8;
    }
    

    InitCAN();
    InitDMA();
    InitTimer();
    InitTimer2();
    InitGPIO();
    InitInputCapture();
//    InitADC();
    
    
    int k;
    while(1){
//        AD1CON1bits.SAMP = 1;       // Start sampling
//        for(k=0; k<3000; k++)
//            ;
//        AD1CON1bits.SAMP = 0;       // Start the conversion
//        while(!AD1CON1bits.DONE)    // Wait for the conversion to complete
//            ; 
//        int temp = (ADC1BUF0-512)*CURRENT_CONSTANT;   // Read the ADC conversion result
//        if(temp<0)
//            temp = 0;
//        motor_current = temp;
        
        
        switch(ultra_sonic_state){
            case 1:
            {
                rising_edge = 1;
                falling_edge_rcvd = 0;
                SONAR_TRIGGER_UP;
                int tt = 0;
                for(tt=0; tt<50; tt++)
                    ;
                SONAR_TRIGGER_DOWN;
                ultra_sonic_state = 3;
                break;
            }
            case 2:
                SONAR_TRIGGER_DOWN;
                ultra_sonic_state = 3;
                break;
            case 3:
                if(falling_edge_rcvd == 1){
                    falling_edge_rcvd = 0;

                    glitch_count = 0;
                    ultra_sonic_state = 4;
                }
                break;
            case 4:
            {
//                glitch_count++;
//                if(glitch_count > 10){ // 0.5ms
                    if(InputCaptureCount == 0){
                        // no glitch
                        int temptemp = 0;
                        if (falling_time > rising_time) {
                            temptemp = falling_time - rising_time;
                        }else {
                            temptemp = (PR2 - rising_time) + falling_time;
                        }

//                        uint8_t send_data[8];
//                        send_data[0] = temptemp;
//                        CAN_WRITE(0x131, 1, send_data);

                        // 40,000,000 / 64 = 625,000
                        // (1,000,000 / 625,000) * (1/58) = 0.0275862 ==> 0.0276
                        int ultra_sonic_data = (unsigned short)((float)(temptemp)*0.0276);
                        ultra_sonic_avg = ultra_sonic_data;
                        if(ultra_sonic_avg > 0xFF){
                            ultra_sonic_avg = 0xFF;
                        }
                        ultra_sonic_fail_count = 0;
                    }else{
                        // there is glitch --> ignore data
                        ;
                    }
                    ultra_sonic_state = 0;
//                }
                break;
            }
            default:
                break;
        }   
    }
    
    return 0;
}


void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
    static unsigned long t1_cnt = 0;
    t1_cnt++;
    
    
    if(t1_cnt%100 == 0){   // 10Hz
        ultra_sonic_state = 1;
        ultra_sonic_fail_count++;
        if(ultra_sonic_fail_count > 10){
            ULTRA_SONIC_ERROR = 1;
        }else{
            ULTRA_SONIC_ERROR = 0;
        }
    }
        
    
    if(ultra_sonic_avg >=2 && ultra_sonic_avg <= 6){
        CUP_DETECT = 1;
    }else{
        CUP_DETECT = 0;
    }
    
    //1000hz 1ms
    
    if(CUP_DISPENSE_FLAG == 1){
        CUP_CHECK_COUNT--;
        if(CUP_CHECK_COUNT < 0){
            CUP_DISPENSE_FLAG = 2;
        }else if(ultra_sonic_avg >=2 && ultra_sonic_avg <= 6){
            CUP_DISPENSE_LAST_ERROR = LAST_ERROR_CUP_EXIST;
            CUP_OUT_COUNT = (CUP_OUT_COUNT+1)%256;
            CUP_DISPENSE_FLAG = 0;
        }
    }else if(CUP_DISPENSE_FLAG == 2){
        LATBbits.LATB4 = 1;
        LATBbits.LATB11 = 0;
        LATBbits.LATB13 = 1;
        
        CUP_DISPENSE_MIN_TIME--;
        if(CUP_DISPENSE_MIN_TIME < 0){
            if(LIMIT_ROTATE == 0){
                LATBbits.LATB11 = 0;
                LATBbits.LATB13 = 0;
                LATBbits.LATB4 = 0;
                CUP_CHECK_COUNT = 100;
                CUP_DISPENSE_FLAG = 3;
            }else if(CUP_DISPENSE_MIN_TIME < -5000){
                LATBbits.LATB11 = 0;
                LATBbits.LATB13 = 0;
                LATBbits.LATB4 = 0;
                CUP_DISPENSE_LAST_ERROR = LAST_ERROR_TIMEOUT;
                CUP_OUT_COUNT = (CUP_OUT_COUNT+1)%256;
                CUP_DISPENSE_FLAG = 0;
            }
        }
    }else if(CUP_DISPENSE_FLAG == 3){
        CUP_CHECK_COUNT--;
        if(CUP_CHECK_COUNT < 0){
            CUP_DISPENSE_LAST_ERROR = LAST_ERROR_SUCCESS;
            CUP_OUT_COUNT = (CUP_OUT_COUNT+1)%256;
            CUP_DISPENSE_FLAG = 0;
        }else if(ultra_sonic_avg > 8){
            CUP_DISPENSE_LAST_ERROR = LAST_ERROR_DISPENSE_FAIL;
            CUP_OUT_COUNT = (CUP_OUT_COUNT+1)%256;
            CUP_DISPENSE_FLAG = 0;
        }
    }
            
    if(t1_cnt%200 == 0){
        LATBbits.LATB1 ^= 1;   
        SendStatus();
    }
    
    IFS0bits.T1IF = 0;
}


MESSAGE	msg;
void __attribute__((interrupt,no_auto_psv))_C1Interrupt(void) 	//ECAN interrupt 
{		
	msg.buffer_number = (unsigned char)C1FIFObits.FNRB;
	rxECAN(&msg);
	
    if(msg.id == 0x120+BNO){
        if(msg.data[0] == 0){
            // cup dispense
            if(CUP_DISPENSE_FLAG == 0){
                CUP_CHECK_COUNT = 100;
                //yujin
                CUP_DISPENSE_MIN_TIME = 1000; //200;
                CUP_DISPENSE_FLAG = 1;
            }else{
                CUP_DISPENSE_LAST_ERROR = LAST_ERROR_BUSY;
                CUP_OUT_COUNT = (CUP_OUT_COUNT+1)%256;
            }
        }
    }
    	
	if((C1RXFUL1&0xFF00)==0 && (C1RXFUL2&0xFFFF)==0){
		C1INTFbits.RBIF = 0;
		IFS2bits.C1IF=0;
	}
}


void __attribute__((interrupt,no_auto_psv))_IC1Interrupt(void){
    unsigned int t1;
    t1 = IC1BUF;
    
    InputCaptureCount++;
    if (rising_edge == 1) {
        rising_edge = 0;
        rising_time = t1;
    } else {
        rising_edge = 1;
        falling_edge_rcvd = 1;
        falling_time = t1;
        InputCaptureCount = 0;
    }
    //Clear IF
    IFS0bits.IC1IF = 0;
}


void SendStatus(){
    uint8_t send_data[8];
    send_data[0] = ultra_sonic_avg;
    send_data[1] = (LIMIT_ROTATE<<3) | (LIMIT_EMPTY<<2) | (ULTRA_SONIC_ERROR<<1) | CUP_DETECT;
    send_data[2] = CUP_OUT_COUNT;
    send_data[3] = CUP_DISPENSE_LAST_ERROR;
    send_data[4] = CUP_DISPENSE_FLAG;
    CAN_WRITE(0x130+BNO, 5, send_data);
}

