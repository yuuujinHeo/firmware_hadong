#ifndef __CONTROL_H__
#define __CONTROL_H__

#include "board_definition.h"

extern int CONTROL_MODE[MAX_CH];

extern long REF[MAX_CH];
extern long PREV_REF[MAX_CH];

extern int MAX_POS1_flag[MAX_CH], MAX_POS2_flag[MAX_CH];
extern long POS_LOWER_LIM[MAX_CH], POS_UPPER_LIM[MAX_CH];
extern float ERROR_sum[MAX_CH];
extern long NewInputRef[MAX_CH], NewInputCurrent[MAX_CH];
extern int MDR[MAX_CH];
extern long PREV_ERROR[MAX_CH];


extern void Initialize_ControlParameter(int ch);
extern void Handler_Control(int ch);



#endif