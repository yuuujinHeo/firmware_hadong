/*
 * File:   main.c
 * Author: rainbow
 *
 */


#include "xc.h"
#include "cpu_config.h"
#include "can.h"
#include "timer.h"
#include "loadcell.h"
#include "gpio.h"

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



#define PLATFORM_OPENED     0
#define PLATFORM_CLOSED     1
static int PLATFORM_SENSOR_A = PLATFORM_OPENED;   // 0: opened    1: closed
static int PLATFORM_SENSOR_B = PLATFORM_OPENED;

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
 
   
    InitCAN();
    InitDMA();
    InitTimer();
    InitGPIO();
    
    while(1){
        ;
    }
    
    return 0;
}


void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
    static unsigned long t1_cnt = 0;
    static int go_to_open_cnt_A = 0;
    static int go_to_open_cnt_B = 0;
    static int go_to_close_cnt_A = 0;
    static int go_to_close_cnt_B = 0;
    
    t1_cnt++;
    
    if(PLATFORM_SENSOR_A == PLATFORM_OPENED){
        if(PORTBbits.RB10 == PLATFORM_CLOSED){
            go_to_close_cnt_A++;
        }else{
            go_to_close_cnt_A = 0;
        }
        if(go_to_close_cnt_A > 200){
            PLATFORM_SENSOR_A = PLATFORM_CLOSED;
            go_to_close_cnt_A = 0;
            go_to_open_cnt_A = 0;
        }
    }else{
        if(PORTBbits.RB10 == PLATFORM_OPENED){
            go_to_open_cnt_A++;
        }else{
            go_to_open_cnt_A = 0;
        }
        if(go_to_open_cnt_A > 200){
            PLATFORM_SENSOR_A = PLATFORM_OPENED;
            go_to_close_cnt_A = 0;
            go_to_open_cnt_A = 0;
        }
    }
    
    if(PLATFORM_SENSOR_B == PLATFORM_OPENED){
        if(PORTBbits.RB11 == PLATFORM_CLOSED){
            go_to_close_cnt_B++;
        }else{
            go_to_close_cnt_B = 0;
        }
        if(go_to_close_cnt_B > 200){
            PLATFORM_SENSOR_B = PLATFORM_CLOSED;
            go_to_close_cnt_B = 0;
            go_to_open_cnt_B = 0;
        }
    }else{
        if(PORTBbits.RB11 == PLATFORM_OPENED){
            go_to_open_cnt_B++;
        }else{
            go_to_open_cnt_B = 0;
        }
        if(go_to_open_cnt_B > 200){
            PLATFORM_SENSOR_B = PLATFORM_OPENED;
            go_to_close_cnt_B = 0;
            go_to_open_cnt_B = 0;
        }
    }
    
    

    if(t1_cnt%100 == 0){
        LATAbits.LATA0 ^= 1;
        LATAbits.LATA1 ^= 1;
        SendStatus();
    }
    
    
    IFS0bits.T1IF = 0;
}


MESSAGE	msg;
void __attribute__((interrupt,no_auto_psv))_C1Interrupt(void) 	//ECAN interrupt 
{		
	msg.buffer_number = (unsigned char)C1FIFObits.FNRB;
	rxECAN(&msg);
	    
	
	if((C1RXFUL1&0xFF00)==0 && (C1RXFUL2&0xFFFF)==0){
		C1INTFbits.RBIF = 0;
		IFS2bits.C1IF=0;
	}
}


void SendStatus(){
    uint8_t send_data[8];
    send_data[0] = PLATFORM_SENSOR_A;
    send_data[1] = PLATFORM_SENSOR_B;

    CAN_WRITE(0x410, 2, send_data);
}
