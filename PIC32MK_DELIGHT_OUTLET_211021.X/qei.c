//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>
#include <sys/attribs.h>
#include <math.h>

#include "qei.h"
#include "execution.h"
#include "spi.h"
#include "can.h"
#include "control.h"
#include "status.h"
#include "FOC.h"


float VEL[MAX_CH] = {0.0,};
int OldPos[MAX_CH] = {0,}, NewPos[MAX_CH] = {0,}, RPM[MAX_CH] = {0,};
int currentMotorPosition[MAX_CH] = {0,};


#define MV_AVG_VEL_MAX  10

static int vel_sav1[MV_AVG_VEL_MAX] = {0,};
static int vel_sav2[MV_AVG_VEL_MAX] = {0,};
static int vel_sav_cnt[MAX_CH] = {0,};
static int mv_avg_vel[MAX_CH] = {0,};


int pos_old[MAX_CH] = {0,};

int ENC_RES[MAX_CH] = {1024, 1024};

int OVER_CURRENT[MAX_CH] = {10000, 10000}; // 10000mA
int OVER_CURRENT_TIME[MAX_CH] = {10, 10}; // 10ms


void InitQEI(){
//TRISCbits.TRISC8 = 1;   // ENC1 B  // ENC1 A
//TRISCbits.TRISC9 = 1;   // ENC2 A  // ENC1 B
//TRISCbits.TRISC6 = 1;   // ENC2 B  // ENC2 A
//TRISCbits.TRISC7 = 1;   // ENC1 A  // ENC2 B
    
    // QEI #1 =====================================
    QEA1Rbits.QEA1R = 0b0101;   // RPC7
    QEB1Rbits.QEB1R = 0b0110;   // RPC8     -->     see TABLE 13-1 INPUT PIN SELECTION
    //INDX1Rbits.INDX1R = 0b1010; // RPG8
    
    QEI1CONbits.PIMOD = 0b000;  // Index not affect position counter (reset)
    QEI1CONbits.CCM = 0;        // Quadrature encoder mode
    QEI1IOCbits.FLTREN = 1;     // Digital filter enable
    //QEI1IOCbits.QFDIV = 0b100;  // 1:16 clock divider for digital filter
    QEI1IOCbits.QFDIV = 0b010;  // 1:2 clock divider for digital filter
    QEI1CONbits.QEIEN = 1;

     // QEI #2 =====================================
    QEA2Rbits.QEA2R = 0b0101;   // RPC9
    QEB2Rbits.QEB2R = 0b0101;   // RPC6     -->     see TABLE 13-1 INPUT PIN SELECTION
    //INDX2Rbits.INDX2R = 0b1010; // RPG8
    
    QEI2CONbits.PIMOD = 0b000;  // Index not affect position counter (reset)
    QEI2CONbits.CCM = 0;        // Quadrature encoder mode
    QEI2IOCbits.FLTREN = 1;     // Digital filter enable
    //QEI2IOCbits.QFDIV = 0b100;  // 1:16 clock divider for digital filter
    QEI2IOCbits.QFDIV = 0b010;  // 1:2 clock divider for digital filter
    QEI2CONbits.QEIEN = 1;
}


void ResetEncoder(int ch){
    OldPos[ch] = 0;
    if(ch == 0){
        POS1CNT = 0;
    }
    pos_old[ch] = 0;
    currentMotorPosition[ch] = 0;
}


void ReadEncoder(int ch){    
    // calculate velocity        
    int cur_vel = NewPos[ch] - OldPos[ch];
    if(ch == 0){
        mv_avg_vel[ch] += cur_vel - vel_sav1[vel_sav_cnt[ch]];
        vel_sav1[vel_sav_cnt[ch]] = cur_vel;
    }else{
        mv_avg_vel[ch] += cur_vel - vel_sav2[vel_sav_cnt[ch]];
        vel_sav2[vel_sav_cnt[ch]] = cur_vel;
    }
    vel_sav_cnt[ch]++;
    if(vel_sav_cnt[ch] >= MV_AVG_VEL_MAX)
        vel_sav_cnt[ch] = 0;
    VEL[ch] = (float)(mv_avg_vel[ch])/MV_AVG_VEL_MAX;
    
    RPM[ch] = VEL[ch]*1000*60/ENC_RES[ch]/ENC_MUL;
    
        
    OldPos[ch] = NewPos[ch];
    
    
    
    // JAM Error Detection
    if(abs(mv_avg_vel[ch]) < 10 && abs(measured_current_q_avg[ch]) > JAM_LIM_Current[ch]){// 190801 avg
        JAM_cnt[ch]++;
        if(JAM_cnt[ch] > JAM_LIM_Time[ch]*100){
            STAT[ch].b.JAM = 1;
        }
    }else{
        JAM_cnt[ch] = 0;
        //STAT.b.JAM = 0;// 190801
    }
    
    // Over Current Error Detection
    if(abs(measured_current_q_avg[ch]) > OVR_LIM_Current[ch]){// 190801 avg
        OVR_cnt[ch]++;
        if(OVR_cnt[ch] > OVR_LIM_Time[ch]*100){
            STAT[ch].b.CUR = 1;
        }
    }else{
        OVR_cnt[ch] = 0;
        //STAT.b.CUR = 0;// 190801
    }
}


static int prev_motorPosition = 0;
void UpdateEncoder(){
    int pos_temp[2];
    int temp_motor_vel[2];
    
    pos_temp[0] = POS1CNT;
    pos_temp[1] = POS2CNT;
    
    int ch = 0;
    for(ch=0; ch<MAX_CH; ch++){
        if(MDR[ch] == 0){
            temp_motor_vel[ch] = -(pos_temp[ch] - pos_old[ch]);
        }else{
            temp_motor_vel[ch] = pos_temp[ch] - pos_old[ch];
        }
        currentMotorPosition[ch] += temp_motor_vel[ch]*ENC_MUL;    
        NewPos[ch] = currentMotorPosition[ch];
        pos_old[ch] = pos_temp[ch];
    }
}




void InitIC(){    
    IC4Rbits.IC4R = 0b1011; // RPF1(UltraSonic Data)
    IC4CONbits.ICTMR = 1;   // timer x --> Timer2
    IC4CONbits.FEDGE = 1;
    IC4CONbits.ICM = 1;
    IC4CONbits.C32 = 0;
    IC4CONbits.ON = 1;
    IPC5bits.IC4IP = 5;
    IPC5bits.IC4IS = 1;
    IFS0bits.IC4IF = 0;
    IEC0bits.IC4IE = 1;
}