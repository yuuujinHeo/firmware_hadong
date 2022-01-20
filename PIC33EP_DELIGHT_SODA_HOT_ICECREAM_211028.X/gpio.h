#ifndef __GPIO_H__
#define __GPIO_H__

#include <xc.h>
#include <p33EP32GP502.h>

#define ROBOT_STOP                  LATBbits.LATB1 = 1;
#define ROBOT_READY                 LATBbits.LATB1 = 0;
#define IS_ROBOT_STOP               (LATBbits.LATB1 == 1)

#define DELIGHT_HOT_ON      LATBbits.LATB10 = 1;
#define DELIGHT_HOT_OFF      LATBbits.LATB10 = 0;

#define DELIGHT_SODA_ON     LATBbits.LATB11 = 1;
#define DELIGHT_SODA_OFF     LATBbits.LATB11 = 0;


extern void InitGPIO();



#endif