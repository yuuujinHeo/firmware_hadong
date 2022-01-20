//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>

#include <sys/attribs.h>
#include <math.h>
#include <string.h>

#include "FOC.h"
#include "qei.h"
#include "sine_table.h"
#include "timer.h"
#include "control.h"
#include "spi.h"
#include "pwm.h"
#include "init.h"
#include "execution.h"
#include "adc.h"
#include "can.h"
#include "status.h"



int Current_R[MAX_CH], RZ[MAX_CH] = {2048,};
int ADIN_RZ[MAX_CH];



#define MV_AVG_MEAS_Q_MAX   5
static int meas_q_sav1[MV_AVG_MEAS_Q_MAX] = {0,};
static int meas_q_sav2[MV_AVG_MEAS_Q_MAX] = {0,};
static int meas_q_sav_cnt[MAX_CH] = {0,};
static int mv_avg_meas_q[MAX_CH] = {0,};
int measured_current_q_avg[MAX_CH] = {0,};



int reference_current_q[MAX_CH] = {0,};


int ref_voltage[MAX_CH] = {0,};

int ref_current_q[MAX_CH] = {0,};      // mA
int measured_current_q[MAX_CH] = {0,}; // mA

double Esum_qd[MAX_CH] = {0.0,};

int Pgain_q1000[MAX_CH] = {DEFAULT_FOC_PGAIN, DEFAULT_FOC_PGAIN};
int Igain_q1000[MAX_CH] = {DEFAULT_FOC_IGAIN, DEFAULT_FOC_IGAIN};

int Pgain_pos[MAX_CH] = {DEFAULT_SPIN_POS_PGAIN, DEFAULT_SPIN_POS_PGAIN};
int Igain_pos[MAX_CH] = {DEFAULT_SPIN_POS_IGAIN, DEFAULT_SPIN_POS_IGAIN};
int Dgain_pos[MAX_CH] = {DEFAULT_SPIN_POS_DGAIN, DEFAULT_SPIN_POS_DGAIN};

void DQ_align(int ch, int dq_save){
    int i, j;

    CONTROL_MODE[ch] = CONTROL_MODE_NO_ACT;
    GateDriver_ON(ch);
        
    
    if(dq_save != 2){
        CurrentNulling(ch);
        GateDriver_ON(ch);
    }
            
    if(dq_save == 2){
//        ResetEncoder();
    }else{
        ResetEncoder(ch);
    }
    
            
    
    //GateDriver_OFF();

    if(ch == 0){
        PDC1 = PWM_STOP;
        PDC2 = PWM_STOP;
    }else{
        PDC3 = PWM_STOP;
        PDC4 = PWM_STOP;
    }
}



void FOC_Init_CurrentControl(int ch){
    Esum_qd[ch] = 0.0;
}


void FOC_OpenLoopControl(int ch){
    static int openloop_not_moving_cnt[MAX_CH] = {0,};
    float temp_ref_voltage = 0;
    
    if(MDR[ch] == 1)
        temp_ref_voltage = -ref_voltage[ch];
    else
        temp_ref_voltage = ref_voltage[ch];
    
    
    if(CONTROL_MODE[ch] == CONTROL_MODE_FOC_OPENLOOP_CONTROL){
        if(abs(temp_ref_voltage) > 1920){   // 48000 * 4% = 1920
            if(abs(VEL[ch]) < 1){
                openloop_not_moving_cnt[ch]++;
                if(openloop_not_moving_cnt[ch] > 4000){ // 200ms at 20K control
                    STAT[ch].b.BIG = 1;
                    openloop_not_moving_cnt[ch] = 0;
                }
            }else{
                openloop_not_moving_cnt[ch] = 0;
            }
        }else{
            openloop_not_moving_cnt[ch] = 0;
        }
    }
    
    PWM_OUT(ch, temp_ref_voltage/48000.0);
}


void PWM_OUT(int ch, float vref){
    if(ch == 0){
        if(vref > 0){
            IOCON1bits.OVRDAT = 0b00;
            IOCON2bits.OVRDAT = 0b01;        
            PDC2 = 0;
            PDC1 = vref*PWM_RES;
        }else if(vref < 0){
            IOCON1bits.OVRDAT = 0b01;
            IOCON2bits.OVRDAT = 0b00;        
            PDC1 = 0;
            PDC2 = -vref*PWM_RES;
        }else{
            IOCON1bits.OVRDAT = 0b00;
            IOCON2bits.OVRDAT = 0b00;
            PDC1 = 0;
            PDC2 = 0;
        }
    }else{
        if(vref > 0){
            IOCON3bits.OVRDAT = 0b00;
            IOCON4bits.OVRDAT = 0b01;        
            PDC4 = 0;
            PDC3 = vref*PWM_RES;
        }else if(vref < 0){
            IOCON3bits.OVRDAT = 0b01;
            IOCON4bits.OVRDAT = 0b00;        
            PDC3 = 0;
            PDC4 = -vref*PWM_RES;
        }else{
            IOCON3bits.OVRDAT = 0b00;
            IOCON4bits.OVRDAT = 0b00;
            PDC3 = 0;
            PDC4 = 0;
        }
    }
}








void MV_AVG_Q(int ch){
    if(ch == 0){
        mv_avg_meas_q[ch] += measured_current_q[ch] - meas_q_sav1[meas_q_sav_cnt[ch]];
        meas_q_sav1[meas_q_sav_cnt[ch]] = measured_current_q[ch];
        meas_q_sav_cnt[ch]++;
        if(meas_q_sav_cnt[ch] >= MV_AVG_MEAS_Q_MAX)
            meas_q_sav_cnt[ch] = 0;
        measured_current_q_avg[ch] = mv_avg_meas_q[ch]/MV_AVG_MEAS_Q_MAX;
    }else{
        mv_avg_meas_q[ch] += measured_current_q[ch] - meas_q_sav2[meas_q_sav_cnt[ch]];
        meas_q_sav2[meas_q_sav_cnt[ch]] = measured_current_q[ch];
        meas_q_sav_cnt[ch]++;
        if(meas_q_sav_cnt[ch] >= MV_AVG_MEAS_Q_MAX)
            meas_q_sav_cnt[ch] = 0;
        measured_current_q_avg[ch] = mv_avg_meas_q[ch]/MV_AVG_MEAS_Q_MAX;
    }
}


void CurrentNulling(int ch){
    int i;
    
    CONTROL_MODE[ch] = CONTROL_MODE_NO_ACT;

    if(ch == 0){
        PDC1 = PWM_STOP;
        PDC2 = PWM_STOP;
    }else{
        PDC3 = PWM_STOP;
        PDC4 = PWM_STOP;
    }
    
    GateDriver_ON(ch);
//    GateDriver_OFF();
    
    if(ch == 0){
        ADIN_RZ[0] = 0;
        for(i=0;i<1000;++i){
            while(!ADCDSTAT1bits.ARDY0)
                ;        
            ADIN_RZ[0] += ADCDATA0;
        }
        ADIN_RZ[0] /= 1000;        
        RZ[0] = (int)ADIN_RZ[0];
    }else{
        ADIN_RZ[1] = 0;
        for(i=0;i<1000;++i){
            while(!ADCDSTAT1bits.ARDY1)
                ;        
            ADIN_RZ[1] += ADCDATA1;
        }
        ADIN_RZ[1] /= 1000;        
        RZ[1] = (int)ADIN_RZ[1];
    }
}



void FOC_CurrentControl_Double(int ch){
    float Vq = 0.0;
    float I_q;
    float E_q;
    
    int ref_current_q_modi = 0;

    
    ref_current_q_modi = ref_current_q[ch];
    if(MDR[ch] == 1){
        ref_current_q_modi = -ref_current_q_modi;
    }
    reference_current_q[ch] = ref_current_q_modi;
    
    if(ch == 0){
        Current_R[ch] = ADCDATA0 - RZ[ch];
        I_q = CURRENT_CONSTANT * (float)(Current_R[ch]);
        measured_current_q[ch] = I_q;
    }else{
        Current_R[ch] = ADCDATA1 - RZ[ch];
        I_q = CURRENT_CONSTANT * (float)(Current_R[ch]);
        measured_current_q[ch] = I_q;
    }
    
    
    // PI current control -----
    E_q = ref_current_q_modi - I_q;     // mA
    
    Esum_qd[ch] += E_q;
    
#ifdef FF_VOLTAGE
    Vq = (Pgain_q1000*E_q + Igain_q1000*Esum_qd)/3000 + MOTOR_R*ref_current_q_modi;   // mV
#else
    Vq = (Pgain_q1000[ch]*E_q + Igain_q1000[ch]*Esum_qd[ch])/1000;// + Vq_ff;   // mV
#endif
   
    float tempMag = fabs(Vq);
    if(tempMag > 48000.0){
        float prev_Vq = Vq;
        Vq = Vq*48000.0/tempMag;
        Esum_qd[ch] += (Vq - prev_Vq)*Igain_q1000[ch]/Pgain_q1000[ch];
    }
    if(Igain_q1000[ch] != 0){
        if(Esum_qd[ch] > 44000000/Igain_q1000[ch]){
            Esum_qd[ch] = 44000000/Igain_q1000[ch];
        }else if(Esum_qd[ch] < -44000000/Igain_q1000[ch]){
            Esum_qd[ch] = -44000000/Igain_q1000[ch];
        }
    }
    
    PWM_OUT(ch, Vq/48000.0);
}



void FOC_MeasureCurrent(int ch){
    if(ch == 0){
        Current_R[ch] = ADCDATA0 - RZ[ch];
        measured_current_q[ch] = CURRENT_CONSTANT * (float)(Current_R[ch]);
    }else{
        Current_R[ch] = ADCDATA1 - RZ[ch];
        measured_current_q[ch] = CURRENT_CONSTANT * (float)(Current_R[ch]);
    }
}


