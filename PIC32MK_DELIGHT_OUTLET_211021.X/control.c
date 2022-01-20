//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>

#include <math.h>

#include "control.h"
#include "status.h"
#include "execution.h"
#include "qei.h"
#include "FOC.h"
#include "timer.h"


int CONTROL_MODE[MAX_CH] = {CONTROL_MODE_NO_ACT, CONTROL_MODE_NO_ACT};


long REF[MAX_CH] = {0,};

int MAX_POS1_flag[MAX_CH]={0,}, MAX_POS2_flag[MAX_CH]={0,};
long POS_LOWER_LIM[MAX_CH], POS_UPPER_LIM[MAX_CH];
float ERROR_sum[MAX_CH] = {0.0, 0.0};
long NewInputRef[MAX_CH]={0,}, NewInputCurrent[MAX_CH]={0,};
int MDR[MAX_CH] = {0,};
long PREV_ERROR[MAX_CH] = {0,};
long PREV_REF[MAX_CH] = {0,};


static void ControlLoop(int ch);


void Handler_Control(int ch){
    if(STAT[ch].b.RUN == 1){
        ControlLoop(ch);
    }
}

void Initialize_ControlParameter(int ch){
    PREV_ERROR[ch] = 0;
}

    

static void ControlLoop(int ch){
	long temp;
    long temp_error;
    long error_dot;
    int pgain, dgain, igain;
    
    
    REF[ch] = NewInputRef[ch];
   
    
    // Position Limit Error
	if(MAX_POS1_flag[ch] == 1){
		if(REF[ch] < POS_LOWER_LIM[ch]*ENC_MUL){
			STAT[ch].b.PS1 = 1;
			REF[ch] = POS_LOWER_LIM[ch]*ENC_MUL;
		}else
			STAT[ch].b.PS1 = 0;
	}
	if(MAX_POS2_flag[ch] == 1){
		if(REF[ch] > POS_UPPER_LIM[ch]*ENC_MUL){
			STAT[ch].b.PS2 = 1;
			REF[ch] = POS_UPPER_LIM[ch]*ENC_MUL;
		}else 
			STAT[ch].b.PS2 = 0;
	}
    
    long cur_ref_error = (REF[ch]-PREV_REF[ch])/ENC_MUL;
    
//    // Input Error
//    if(cur_ref_error > I_ERR || cur_ref_error < -I_ERR){
//        STAT.b.INP = 1;
//        
//        // not update reference
//        REF = PREV_REF;
//    }
    
    
    PREV_REF[ch] = REF[ch];
  
    pgain = Pgain_pos[ch];
    dgain = Dgain_pos[ch];
    igain = Igain_pos[ch];

    // ===================================
        
    
    temp_error = (REF[ch] - NewPos[ch])/ENC_MUL;
    
    // Big Error
    if(temp_error > B_ERR[ch] || temp_error < -B_ERR[ch]){
        STAT[ch].b.BIG = 1;
    }
    
    
    
    error_dot = temp_error-PREV_ERROR[ch];
    PREV_ERROR[ch] = temp_error;
    
    ERROR_sum[ch] += temp_error/100.0;
    float tempERS_max;    
    if(igain >= 1){
        tempERS_max = ERS_max/igain;
    }else{
        tempERS_max = ERS_max;
    }
	if(ERROR_sum[ch] > tempERS_max)
		ERROR_sum[ch] = tempERS_max;
	else if(ERROR_sum[ch] < -tempERS_max)
		ERROR_sum[ch] = -tempERS_max;
     
    
    temp = (long)((float)(temp_error*pgain + error_dot*dgain + igain*ERROR_sum[ch])/10.0);
    
    if(temp > MAX_CUR_LIMIT) temp = MAX_CUR_LIMIT;
    if(temp < -MAX_CUR_LIMIT) temp = -MAX_CUR_LIMIT;
        
    ref_current_q[ch] = temp;
}


	


