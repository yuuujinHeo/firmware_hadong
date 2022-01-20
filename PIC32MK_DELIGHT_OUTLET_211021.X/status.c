//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>

#include <math.h>

#include "status.h"
#include "can.h"
#include "adc.h"
#include "qei.h"
#include "control.h"


#define MOTOR_FAULT1    !PORTEbits.RE14
#define MOTOR_FAULT2    !PORTEbits.RE13

int     JMC;
int     INIT_BOARD_MODE = INIT_BOARD_MODE_OK;
char    BoardName[13] = "OUTLET";


mSTAT   STAT[MAX_CH];
int     STATUS_FLAG[MAX_CH] = {0,};



// error --------------------
int T_ERR;
long I_ERR[MAX_CH], B_ERR[MAX_CH];
unsigned int JAM_cnt[MAX_CH] = {0,}, OVR_cnt[MAX_CH] = {0,}, CUR_BIG_cnt[MAX_CH] = {0,};
unsigned int JAM_LIM_Current[MAX_CH], OVR_LIM_Current[MAX_CH], JAM_LIM_Time[MAX_CH], OVR_LIM_Time[MAX_CH], CUR_BIG_LIM[MAX_CH], CUR_BIG_Time[MAX_CH] = {10,};





static void SendStatus(int ch){
 	unsigned int dlc;
 	unsigned char canTxData[8] = {0,};
    unsigned char temp_board = temperature_board_celsius_x10/10;
    unsigned char temp_motor = temperature_motor_celsius_x10/10;
    
	canTxData[0] = (unsigned char)(STAT[ch].B&0xFF);		// STAT Byte
	canTxData[1] = (unsigned char)((STAT[ch].B>>8)&0xFF);	// STAT Byte
	canTxData[2] = (unsigned char)((STAT[ch].B>>16)&0xFF);	// STAT Byte
    canTxData[3] = (unsigned char)(temp_board);
    canTxData[4] = (unsigned char)(temp_motor);
    dlc = 5;
	
    if(ch == 0){
        Push_CAN_TX_Ring(CANID_STAT1, dlc, canTxData);
    }else{
        Push_CAN_TX_Ring(CANID_STAT2, dlc, canTxData);
    }
}

void Handler_Status(unsigned long cnt){
    static unsigned long prev_cnt = 0;
    static unsigned int old_stat[MAX_CH] = {0,};
    static unsigned int error_report_cnt[MAX_CH] = {0,};
//    static unsigned int periodic_send_cnt = 0;
    
    int ch;    
    for(ch=0; ch<MAX_CH; ch++){
        if(old_stat[ch] != STAT[ch].B){
            STATUS_FLAG[ch] = 1;
        }
        old_stat[ch] = STAT[ch].B;
    }
    
    if(cnt - prev_cnt > 0){
        prev_cnt = cnt;
        
        int ch;
        for(ch=0; ch<MAX_CH; ch++){
            error_report_cnt[ch]++;
    //        periodic_send_cnt++;
            if(error_report_cnt[ch] > 1000){
                error_report_cnt[ch] = 0;

                if((STAT[ch].B&0x0000FF00) != 0)
                    error_report_cnt[ch]++;

    //            if(STAT.b.HME != 6 && STAT.b.FET == 1 && STAT.b.RUN == 1)
    //                error_report_cnt++;

                if(error_report_cnt[ch] > 0){
                    if(ch == 0){
                        SET_STATUS_FLAG1
                        error_report_cnt[ch] = 0;
                    }else{
                        SET_STATUS_FLAG2
                        error_report_cnt[ch] = 0;        
                    }
                }
            }
        }
//        if(periodic_send_cnt > 1000){
//            periodic_send_cnt = 0;
//            SET_STATUS_FLAG
//        }
    }
    
    for(ch=0; ch<MAX_CH; ch++){
        if(STATUS_FLAG[ch] == 1){
            SendStatus(ch);
            STATUS_FLAG[ch] = 0;
        }
    }
}


void Handler_Error(unsigned long cnt){
    static unsigned long prev_cnt = 0;
    static unsigned int temperature_cnt = 0;
    static unsigned int fault_error_cnt[MAX_CH] = {0,};
    int tick_passed = 0;
    
    
    if(cnt - prev_cnt > 0){
        prev_cnt = cnt;
        tick_passed = 1;
    }
    
    
    // ERROR - motor fault -----------------------------
    if((MOTOR_FAULT1 == 1) && (MOTOR_HIP_STAT1 == 1)){
        STAT[0].b.FLT = 1;
        fault_error_cnt[0] = 30;
    }else{
        fault_error_cnt[0] = 0;
    }
	if((MOTOR_FAULT2 == 1) && (MOTOR_HIP_STAT2 == 1)){
        STAT[1].b.FLT = 1;
        fault_error_cnt[1] = 30;
    }else{
        fault_error_cnt[1] = 0;
    }	
	
    // from here -- more than one tick has been passed -----------------------------------------------
    if(tick_passed == 0)
        return;
        
    
    // ERROR - motor fault -----------------------------
    int ch;
    for(ch=0; ch<MAX_CH; ch++){
         if(fault_error_cnt[ch] > 0){
            --fault_error_cnt[ch];
        }else if(fault_error_cnt[ch] == 0){
            if(STAT[ch].b.FLT == 1){
                STAT[ch].b.FLT = 0;
            }
        }
    }
   
    
    // ERROR - temperature high -----------------------------
    temperature_cnt++;
    if(temperature_cnt > 1000){
        temperature_cnt = 0;
        
        // temperature error
        if(temperature_board_celsius_x10 > T_ERR*10){
			STAT[0].b.TMP = 1;
            STAT[1].b.TMP = 1;
        }else{
            STAT[0].b.TMP = 0;
            STAT[1].b.TMP = 0;
        }
    }
}