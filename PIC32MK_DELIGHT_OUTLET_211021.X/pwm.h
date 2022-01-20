#ifndef __PWM_H__
#define __PWM_H__

#include "board_definition.h"


#define SOLENOID_STAT_HARD_PUSH     0
#define SOLENOID_STAT_WEAK_PUSH     1
#define SOLENOID_STAT_TURN_OFF      2
#define SOLENOID_STAT_SHAKE_PUSH    3

#define SOLENOID_HARD_PUSH_COUNT    10  //15      // 5
#define SOLENOID_WEAK_PUSH_COUNT    500


#define PWM_RES         3000

#define PWM_STOP    0



extern void InitPWM();
extern void Handler_Solenoid();

#endif