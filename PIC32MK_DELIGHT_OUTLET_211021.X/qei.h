#ifndef __QEI_H__
#define __QEI_H__

#include "board_definition.h"

#define AUTO_SCALE_RES  6000        //4000
#define AUTO_SCALE_RES_DIV_BY_100   60

extern void InitQEI();
extern void ResetEncoder();
extern void ReadEncoder(int ch);
extern void UpdateEncoder();
extern void EncoderTest();

    

extern int ENC_RES[MAX_CH];

extern float VEL[MAX_CH];
extern int OldPos[MAX_CH], NewPos[MAX_CH], RPM[MAX_CH];
extern int currentMotorPosition[MAX_CH];



extern int pos_old[MAX_CH];


extern int OVER_CURRENT[MAX_CH];
extern int OVER_CURRENT_TIME[MAX_CH];
extern int CURRENT_LIMIT[MAX_CH];



#endif