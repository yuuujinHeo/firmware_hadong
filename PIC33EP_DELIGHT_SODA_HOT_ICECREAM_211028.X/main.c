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



#define ICECREAM_CYLINDER   


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




int BOARD_TYPE = BOARD_TYPE_SODA;


static int OUT_STATE = 0;   // 0: idle  1: start    2: ready    3: out
static int OUT_MODE = 0;
static unsigned long OUT_TIME_COUNT = 0;
static long START_WEIGHT = 0;
static long FLOWRATE_CHECK_WEIGHT1 = 0;
static long FLOWRATE_CHECK_WEIGHT2 = 0;
static long FLOWRATE_CHECK_LOW_VALUE = -1;
static long FLOWRATE_CHECK_HIGH_VALUE = -1;
static long FLOWRATE_COUNT = 0;
static int OUT_TARGET_WEIGHT = 0;
static long OUT_TARGET_TIME_COUNT = 0;
static long OUT_TIMEOUT_COUNT = 0;

static int CUR_WEIGHT = 0;

static int OUT_COUNT = 0;
static int DISPENSE_LAST_ERROR = 0;
enum{
    LAST_ERROR_SUCCESS = 0,
    LAST_ERROR_BUSY,
    LAST_ERROR_TIMEOUT
};
static int STOP_NOW = 0;


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

    TRISAbits.TRISA4 = 1;
    TRISBbits.TRISB4 = 1;
    TRISAbits.TRISA3 = 1;
    TRISBbits.TRISB9 = 1;
    
    int temp_bno = 0;
    if(PORTAbits.RA4 == 0){
        temp_bno += 1;
    }
    if(PORTBbits.RB4 == 0){
        temp_bno += 2;
    }
    if(PORTAbits.RA3 == 0){
        temp_bno += 4;
    }
    if(PORTBbits.RB9 == 0){
        temp_bno += 8;
    }
    if(temp_bno == 1){
        BOARD_TYPE = BOARD_TYPE_SODA;
    }else if(temp_bno == 2){
        BOARD_TYPE = BOARD_TYPE_HOT;
    }else if(temp_bno == 4){
        BOARD_TYPE = BOARD_TYPE_ICECREAM;
    }else{
        BOARD_TYPE = BOARD_TYPE_NONE;
    }
    InitCAN();
    InitDMA();
    InitTimer();
    InitGPIO();
    InitLoadcell();
    
    while(1){
        ;
    }
    
    return 0;
}


void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
    static int robot_stop_auto_idle_cnt = 0;
    static unsigned int cylinder_done = 0;
    static unsigned long cylinder_done_cnt = 0;
    
    static long prev_loadcell_value_1 = 0;
    static long prev_loadcell_value_2 = 0;
    static unsigned int loadcell_abnormal_cnt_1 = 0;
    static unsigned int loadcell_abnormal_cnt_2 = 0;
    static unsigned long t1_cnt = 0;
    t1_cnt++;
    

    if(t1_cnt%20 == 0){ // 20ms
        // check sensor is ok---------------------------------------
        if(prev_loadcell_value_1 == LOADCELL_INFO_1.raw){
            loadcell_abnormal_cnt_1++;
        }else{
            loadcell_abnormal_cnt_1 = 0;
        }
        if(prev_loadcell_value_2 == LOADCELL_INFO_2.raw){
            loadcell_abnormal_cnt_2++;
        }else{
            loadcell_abnormal_cnt_2 = 0;
        }

        if(loadcell_abnormal_cnt_1 > 30){
            LOADCELL_INFO_1.error = 1;
        }else{
            LOADCELL_INFO_1.error = 0;
        }
        if(loadcell_abnormal_cnt_2 > 30){
            LOADCELL_INFO_2.error = 1;
        }else{
            LOADCELL_INFO_2.error = 0;
        }

        prev_loadcell_value_1 = LOADCELL_INFO_1.raw;
        prev_loadcell_value_2 = LOADCELL_INFO_2.raw;


        // Tare-----------------------------------------------------
        int i;
        for(i=0; i<MAX_MODULE; i++){
            if(tare_flag[i] == 1){
                float value;
                if(i == 0){
                    value = LOADCELL_INFO_1.raw;
                }else if(i == 1){
                    value = LOADCELL_INFO_2.raw;
                }
                tare_sum[i] += value;
                tare_cnt[i]++;
                if(tare_cnt[i] >= tare_num[i]){
                    long avg = tare_sum[i]/tare_num[i];
                    HX711_set_offset(i, avg);
                    tare_flag[i] = 0;
                }
            }
        }

        // scale adjust----------------------------------------------
        LOADCELL_INFO_1.value = (LOADCELL_INFO_1.raw-OFFSET[0])/SCALE[0];
        LOADCELL_INFO_2.value = (LOADCELL_INFO_2.raw-OFFSET[1])/SCALE[1];        
    }
    
    if(BOARD_TYPE == BOARD_TYPE_SODA){
        CUR_WEIGHT = LOADCELL_INFO_1.value; //(LOADCELL_INFO_1.value + LOADCELL_INFO_2.value)/2;
    }else if(BOARD_TYPE == BOARD_TYPE_HOT){
        CUR_WEIGHT = (LOADCELL_INFO_1.value + LOADCELL_INFO_2.value)/2;
    }else if(BOARD_TYPE == BOARD_TYPE_ICECREAM){
        CUR_WEIGHT = (LOADCELL_INFO_1.value + LOADCELL_INFO_2.value)/2;
    }

    // Out Process------------------------------------------------
    int DO_STOP = 0;
    if(OUT_STATE == 1){
        if(BOARD_TYPE == BOARD_TYPE_SODA){
            DELIGHT_SODA_OFF
        }else if(BOARD_TYPE == BOARD_TYPE_HOT){
            DELIGHT_HOT_OFF
        }else if(BOARD_TYPE == BOARD_TYPE_ICECREAM){
            ;
        }
        OUT_STATE = 2;
    }else if(OUT_STATE == 2){
        OUT_TIME_COUNT = 0;
        OUT_STATE = 3;
    }else if(OUT_STATE == 3){
        OUT_TIMEOUT_COUNT--;
        OUT_TIME_COUNT++;
        
        if(BOARD_TYPE == BOARD_TYPE_SODA){
            DELIGHT_SODA_ON
        }else if(BOARD_TYPE == BOARD_TYPE_HOT){
            DELIGHT_HOT_ON
        }else if(BOARD_TYPE == BOARD_TYPE_ICECREAM){
#ifdef ICECREAM_CYLINDER
            LATAbits.LATA0 = 1;
            LATAbits.LATA1 = 0;
#endif
        }
        
        if(OUT_MODE == 0){
            // time based control
            if(OUT_TIME_COUNT >= OUT_TARGET_TIME_COUNT){
                if(BOARD_TYPE == BOARD_TYPE_SODA){
                    DELIGHT_SODA_OFF
                }else if(BOARD_TYPE == BOARD_TYPE_HOT){
                    DELIGHT_HOT_OFF
                }else if(BOARD_TYPE == BOARD_TYPE_ICECREAM){
#ifdef ICECREAM_CYLINDER
                    LATAbits.LATA0 = 0;
                    LATAbits.LATA1 = 1;
                    cylinder_done_cnt = 0;
                    cylinder_done = 1;
#endif
                }
                DISPENSE_LAST_ERROR = LAST_ERROR_SUCCESS;
                OUT_COUNT = (OUT_COUNT+1)%256;
                OUT_STATE = 0;
            }
        }else if(OUT_MODE == 1){
            // loadcell based control
            if(FLOWRATE_CHECK_LOW_VALUE < 0){
                if(CUR_WEIGHT >= FLOWRATE_CHECK_WEIGHT1){
                    FLOWRATE_CHECK_LOW_VALUE = CUR_WEIGHT;
                    FLOWRATE_COUNT = 0;
                }
            }
            else if(FLOWRATE_CHECK_HIGH_VALUE < 0){
                FLOWRATE_COUNT++;
                if(CUR_WEIGHT >= FLOWRATE_CHECK_WEIGHT2){
                    FLOWRATE_CHECK_HIGH_VALUE = CUR_WEIGHT;

                    if(FLOWRATE_COUNT < 5){
                        // do not update
                    }else if(FLOWRATE_CHECK_LOW_VALUE >= FLOWRATE_CHECK_HIGH_VALUE){
                        // do not update
                    }else{
                        long flowrate = (FLOWRATE_CHECK_HIGH_VALUE - FLOWRATE_CHECK_LOW_VALUE)*1000/FLOWRATE_COUNT;
                        if(BOARD_TYPE == BOARD_TYPE_SODA){
                            OUT_TARGET_WEIGHT = OUT_TARGET_WEIGHT - flowrate*220/1000;  // 200ms stop time
                        }else if(BOARD_TYPE == BOARD_TYPE_HOT){
                            OUT_TARGET_WEIGHT = OUT_TARGET_WEIGHT - flowrate*100/1000;  // 100ms stop time
                        }else if(BOARD_TYPE == BOARD_TYPE_ICECREAM){
#ifdef ICECREAM_CYLINDER
                            OUT_TARGET_WEIGHT = OUT_TARGET_WEIGHT - flowrate*2500/1000;  // 1000ms stop time
#else
                            OUT_TARGET_WEIGHT = OUT_TARGET_WEIGHT - flowrate*1000/1000;  // 1000ms stop time
#endif
                        }
                    }
                }
            }

            if(CUR_WEIGHT >= OUT_TARGET_WEIGHT){
                if(BOARD_TYPE == BOARD_TYPE_SODA){
                    DELIGHT_SODA_OFF
                }else if(BOARD_TYPE == BOARD_TYPE_HOT){
                    DELIGHT_HOT_OFF
                }else if(BOARD_TYPE == BOARD_TYPE_ICECREAM){
#ifdef ICECREAM_CYLINDER
                    LATAbits.LATA0 = 0;
                    LATAbits.LATA1 = 1;
                    cylinder_done_cnt = 0;
                    cylinder_done = 1;
#else
                    DO_STOP = 1;
#endif
                }
                DISPENSE_LAST_ERROR = LAST_ERROR_SUCCESS;
                OUT_COUNT = (OUT_COUNT+1)%256;
                OUT_STATE = 0;
            }
        }
        

        // timeout condition
        if(OUT_TIMEOUT_COUNT < 0){
            if(BOARD_TYPE == BOARD_TYPE_SODA){
                DELIGHT_SODA_OFF
            }else if(BOARD_TYPE == BOARD_TYPE_HOT){
                DELIGHT_HOT_OFF
            }else if(BOARD_TYPE == BOARD_TYPE_ICECREAM){
#ifdef ICECREAM_CYLINDER
                LATAbits.LATA0 = 0;
                LATAbits.LATA1 = 1;
                cylinder_done_cnt = 0;
                cylinder_done = 1;
#else
                DO_STOP = 1;
#endif
            }
            DISPENSE_LAST_ERROR = LAST_ERROR_TIMEOUT;
            OUT_COUNT = (OUT_COUNT+1)%256;
            OUT_STATE = 0;
        }
    }
    
    
    if(BOARD_TYPE == BOARD_TYPE_ICECREAM){
#ifdef ICECREAM_CYLINDER
        if(cylinder_done == 1){
            cylinder_done_cnt++;
            if(cylinder_done_cnt > 5000){
                cylinder_done_cnt = 0;
                cylinder_done = 0;
                LATAbits.LATA0 = 0;
                LATAbits.LATA1 = 0;
            }
        }
#else
        if(STOP_NOW == 1){
            STOP_NOW = 0;
            DO_STOP = 1;
        }

        // Stop    
        if(DO_STOP == 1){
            robot_stop_auto_idle_cnt = 0;
            ROBOT_STOP
        }

        // recover stop signal-------------------------------------------
        if(IS_ROBOT_STOP){
            robot_stop_auto_idle_cnt++;
            if(robot_stop_auto_idle_cnt > 2000){ // 2000ms
                robot_stop_auto_idle_cnt = 0;
                ROBOT_READY
            }
        }
#endif
    }
    

    if(t1_cnt%100 == 0){
        LATBbits.LATB7 ^= 1;
        LATBbits.LATB8 ^= 1;
        SendStatus();
    }
    
    
    IFS0bits.T1IF = 0;
}


MESSAGE	msg;
void __attribute__((interrupt,no_auto_psv))_C1Interrupt(void) 	//ECAN interrupt 
{		
	msg.buffer_number = (unsigned char)C1FIFObits.FNRB;
	rxECAN(&msg);
	
    if(BOARD_TYPE == BOARD_TYPE_SODA){
        if(msg.id == 0x141){
            if(msg.data[0] == 0){
                // out based on time
                if(OUT_STATE != 0){
                    DISPENSE_LAST_ERROR = LAST_ERROR_BUSY;
                    OUT_COUNT = (OUT_COUNT+1)%256;
                }else{
                    OUT_MODE = 0;
                    unsigned short time_100ms = (unsigned short)(msg.data[2] | (msg.data[3]<<8));
                    OUT_TARGET_TIME_COUNT = (unsigned long)(time_100ms)*100;
                    OUT_TIMEOUT_COUNT = 15000;  // 15sec
                    
                    OUT_STATE = 1;
                }
            }else if(msg.data[0] == 1){
                // out based on loadcell
                if(OUT_STATE != 0){
                    DISPENSE_LAST_ERROR = LAST_ERROR_BUSY;
                    OUT_COUNT = (OUT_COUNT+1)%256;
                }else{
                    OUT_MODE = 1;
                    unsigned short weight_g = (unsigned short)(msg.data[2] | (msg.data[3]<<8));
                    OUT_TARGET_WEIGHT = weight_g;
                    START_WEIGHT = CUR_WEIGHT;
                    FLOWRATE_COUNT = 0;
                    FLOWRATE_CHECK_LOW_VALUE = -1;
                    FLOWRATE_CHECK_HIGH_VALUE = -1;
                    FLOWRATE_CHECK_WEIGHT1 = START_WEIGHT + (OUT_TARGET_WEIGHT-START_WEIGHT)*2/10;
                    FLOWRATE_CHECK_WEIGHT2 = START_WEIGHT + (OUT_TARGET_WEIGHT-START_WEIGHT)*5/10;
                    OUT_TIMEOUT_COUNT = 15000;  // 15sec

                    OUT_STATE = 1;
                }
            }
            else if(msg.data[0] == 2){
                // tare
                HX711_tare(0, 3);
                HX711_tare(1, 3);
            }
            else if(msg.data[0] == 3){
                // set calib param
                int target = msg.data[1];
                unsigned short scale = (unsigned short)(msg.data[2] | (msg.data[3]<<8));
                if(scale < 1){
                    scale = 1;
                }
                if(target == 0){
                    HX711_set_scale(target, scale);
                    LOADCELL_INFO_1.calib_param = scale;
                }else if(target == 1){
                    HX711_set_scale(target, scale);
                    LOADCELL_INFO_2.calib_param = scale;
                }
            }
        }
    }else if(BOARD_TYPE == BOARD_TYPE_HOT){
        if(msg.id == 0x142){
            if(msg.data[0] == 0){
                // out based on time
                if(OUT_STATE != 0){
                    DISPENSE_LAST_ERROR = LAST_ERROR_BUSY;
                    OUT_COUNT = (OUT_COUNT+1)%256;
                }else{
                    OUT_MODE = 0;
                    unsigned short time_100ms = (unsigned short)(msg.data[2] | (msg.data[3]<<8));
                    OUT_TARGET_TIME_COUNT = (unsigned long)(time_100ms)*100;
                    OUT_TIMEOUT_COUNT = 15000;  // 15sec
                    
                    OUT_STATE = 1;
                }
            }else if(msg.data[0] == 1){
                // out based on loadcell
                if(OUT_STATE != 0){
                    DISPENSE_LAST_ERROR = LAST_ERROR_BUSY;
                    OUT_COUNT = (OUT_COUNT+1)%256;
                }else{
                    OUT_MODE = 1;
                    unsigned short weight_g = (unsigned short)(msg.data[2] | (msg.data[3]<<8));
                    OUT_TARGET_WEIGHT = weight_g;
                    START_WEIGHT = CUR_WEIGHT;
                    FLOWRATE_COUNT = 0;
                    FLOWRATE_CHECK_LOW_VALUE = -1;
                    FLOWRATE_CHECK_HIGH_VALUE = -1;
                    FLOWRATE_CHECK_WEIGHT1 = START_WEIGHT + (OUT_TARGET_WEIGHT-START_WEIGHT)*2/10;
                    FLOWRATE_CHECK_WEIGHT2 = START_WEIGHT + (OUT_TARGET_WEIGHT-START_WEIGHT)*5/10;
                    OUT_TIMEOUT_COUNT = 15000;  // 15sec

                    OUT_STATE = 1;
                }
            }
            else if(msg.data[0] == 2){
                // tare
                HX711_tare(0, 3);
                HX711_tare(1, 3);
            }
            else if(msg.data[0] == 3){
                // set calib param
                int target = msg.data[1];
                unsigned short scale = (unsigned short)(msg.data[2] | (msg.data[3]<<8));
                if(scale < 1){
                    scale = 1;
                }
                if(target == 0){
                    HX711_set_scale(target, scale);
                    LOADCELL_INFO_1.calib_param = scale;
                }else if(target == 1){
                    HX711_set_scale(target, scale);
                    LOADCELL_INFO_2.calib_param = scale;
                }
            }
        }
    }else if(BOARD_TYPE == BOARD_TYPE_ICECREAM){
        if(msg.data[0] == 1){
            // out based on loadcell
            if(OUT_STATE != 0){
                STOP_NOW = 1;
                DISPENSE_LAST_ERROR = LAST_ERROR_BUSY;
                OUT_COUNT = (OUT_COUNT+1)%256;
            }else{
                OUT_MODE = 1;
                unsigned short weight_g = (unsigned short)(msg.data[2] | (msg.data[3]<<8));
                OUT_TARGET_WEIGHT = weight_g;
                START_WEIGHT = CUR_WEIGHT;
                FLOWRATE_COUNT = 0;
                FLOWRATE_CHECK_LOW_VALUE = -1;
                FLOWRATE_CHECK_HIGH_VALUE = -1;
                FLOWRATE_CHECK_WEIGHT1 = START_WEIGHT + (OUT_TARGET_WEIGHT-START_WEIGHT)*2/10;
                FLOWRATE_CHECK_WEIGHT2 = START_WEIGHT + (OUT_TARGET_WEIGHT-START_WEIGHT)*5/10;
                OUT_TIMEOUT_COUNT = 15000;  // 15sec

                OUT_STATE = 1;
            }
        }
        else if(msg.data[0] == 2){
            // tare
            HX711_tare(0, 3);
            HX711_tare(1, 3);
        }
        else if(msg.data[0] == 3){
            // set calib param
            int target = msg.data[1];
            unsigned short scale = (unsigned short)(msg.data[2] | (msg.data[3]<<8));
            if(scale < 1){
                scale = 1;
            }
            if(target == 0){
                HX711_set_scale(target, scale);
                LOADCELL_INFO_1.calib_param = scale;
            }else if(target == 1){
                HX711_set_scale(target, scale);
                LOADCELL_INFO_2.calib_param = scale;
            }
        }
    }
    
	
	if((C1RXFUL1&0xFF00)==0 && (C1RXFUL2&0xFFFF)==0){
		C1INTFbits.RBIF = 0;
		IFS2bits.C1IF=0;
	}
}



void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void) {
    static int prev_loadcell_set_1 = 0;
    static int prev_loadcell_set_2 = 0;
    int loadcell_set_1 = LOAD_CELL_DATA_MODULE_1;
    int loadcell_set_2 = LOAD_CELL_DATA_MODULE_2;
    int k;
    
    if(loadcell_set_1 == 0 && prev_loadcell_set_1 == 1){
        CNENBbits.CNIEB14 = 0;
        for(k=0; k<100; k++)
            ;
        LOADCELL_INFO_1.raw = HX711_read_module_1();
    }
    CNENBbits.CNIEB14 = 1;
    
    if(loadcell_set_2 == 0 && prev_loadcell_set_2 == 1){
        CNENBbits.CNIEB12 = 0;
        for(k=0; k<100; k++)
            ;
        LOADCELL_INFO_2.raw = HX711_read_module_2();
    }
    CNENBbits.CNIEB12 = 1;
    
    prev_loadcell_set_1 = loadcell_set_1;
    prev_loadcell_set_2 = loadcell_set_2;
   
    IFS1bits.CNIF = 0;
}


void SendStatus(){
    static unsigned int toggle = 0;
    toggle++;
    uint8_t send_data[8];
    if(BOARD_TYPE == BOARD_TYPE_SODA){
        send_data[0] = LOADCELL_INFO_1.value&0xFF;
        send_data[1] = (LOADCELL_INFO_1.value>>8)&0xFF;
        send_data[2] = LOADCELL_INFO_1.error;
        send_data[3] = LOADCELL_INFO_1.calib_param&0xFF;
        send_data[4] = (LOADCELL_INFO_1.calib_param>>8)&0xFF;
        send_data[5] = OUT_STATE;
        send_data[6] = OUT_COUNT;
        send_data[7] = DISPENSE_LAST_ERROR;
        CAN_WRITE(0x151, 8, send_data);  
    }else if(BOARD_TYPE == BOARD_TYPE_HOT || BOARD_TYPE == BOARD_TYPE_ICECREAM){
        if(toggle%2 == 0){
            send_data[0] = 0;
            send_data[1] = LOADCELL_INFO_1.value&0xFF;
            send_data[2] = (LOADCELL_INFO_1.value>>8)&0xFF;
            send_data[3] = LOADCELL_INFO_1.error;
            send_data[4] = LOADCELL_INFO_1.calib_param&0xFF;
            send_data[5] = (LOADCELL_INFO_1.calib_param>>8)&0xFF;
            send_data[6] = LOADCELL_INFO_2.value&0xFF;
            send_data[7] = (LOADCELL_INFO_2.value>>8)&0xFF;
        }else{
            send_data[0] = 1;
            send_data[1] = LOADCELL_INFO_2.error;
            send_data[2] = LOADCELL_INFO_2.calib_param&0xFF;
            send_data[3] = (LOADCELL_INFO_2.calib_param>>8)&0xFF;
            send_data[4] = OUT_STATE;
            send_data[5] = OUT_COUNT;
            send_data[6] = DISPENSE_LAST_ERROR;
            send_data[7] = 0;
        }
        if(BOARD_TYPE == BOARD_TYPE_HOT){
            CAN_WRITE(0x152, 8, send_data); 
        }else if(BOARD_TYPE == BOARD_TYPE_ICECREAM){
            CAN_WRITE(0x153, 8, send_data);   
        }
    }else{
        CAN_WRITE(0x150, 8, send_data);
    }
}

//void SendValue1(){
//    uint8_t send_data[8];
//    send_data[0] = 0;
//    send_data[1] = LOADCELL_INFO_1.value&0xFF;
//    send_data[2] = (LOADCELL_INFO_1.value>>8)&0xFF;
//    send_data[3] = LOADCELL_INFO_1.raw&0xFF;
//    send_data[4] = (LOADCELL_INFO_1.raw>>8)&0xFF;
//    send_data[5] = (LOADCELL_INFO_1.raw>>16)&0xFF;
//    send_data[6] = (LOADCELL_INFO_1.raw>>24)&0xFF;
//    send_data[7] = LOADCELL_INFO_1.error;
//    CAN_WRITE(0x150, 8, send_data);    
//}
//
//void SendValue2(){
//    uint8_t send_data[8];
//    send_data[0] = 1;
//    send_data[1] = LOADCELL_INFO_2.value&0xFF;
//    send_data[2] = (LOADCELL_INFO_2.value>>8)&0xFF;
//    send_data[3] = LOADCELL_INFO_2.raw&0xFF;
//    send_data[4] = (LOADCELL_INFO_2.raw>>8)&0xFF;
//    send_data[5] = (LOADCELL_INFO_2.raw>>16)&0xFF;
//    send_data[6] = (LOADCELL_INFO_2.raw>>24)&0xFF;
//    send_data[7] = LOADCELL_INFO_2.error;
//    CAN_WRITE(0x150, 8, send_data);
//}
//
//void SendStatus(){
//    uint8_t send_data[8];
//    send_data[0] = 2;
//    send_data[1] = LOADCELL_INFO_1.calib_param&0xFF;
//    send_data[2] = (LOADCELL_INFO_1.calib_param>>8)&0xFF;
//    send_data[3] = LOADCELL_INFO_2.calib_param&0xFF;
//    send_data[4] = (LOADCELL_INFO_2.calib_param>>8)&0xFF;
//    send_data[5] = OUT_STATE;
//    send_data[6] = OUT_COUNT;
//    send_data[7] = DISPENSE_LAST_ERROR;
//    
//    CAN_WRITE(0x150, 8, send_data);
//}