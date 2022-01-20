#ifndef __FOC_H__
#define __FOC_H__

#include "board_definition.h"


extern double Esum_dd[MAX_CH];
extern double Esum_qd[MAX_CH];

extern int Current_R[MAX_CH], RZ[MAX_CH];
extern int ADIN_RZ[MAX_CH];



extern int measured_current_q_avg[MAX_CH];


extern int Pgain_q1000[MAX_CH];
extern int Igain_q1000[MAX_CH];

extern int Pgain_pos[MAX_CH];
extern int Igain_pos[MAX_CH];
extern int Dgain_pos[MAX_CH];



extern int ref_voltage[MAX_CH];
extern int ref_current_q[MAX_CH];
extern int measured_current_q[MAX_CH];

extern int reference_current_q[MAX_CH];



extern void PWM_OUT(int ch, float vref);


extern void FOC_Init_CurrentControl(int ch);

extern void FOC_OpenLoopControl(int ch);

extern void DQ_align(int ch, int dq_save);
extern void CurrentNulling(int ch);

extern void FOC_CurrentControl_Double(int ch);
extern void FOC_MeasureCurrent(int ch);


#endif