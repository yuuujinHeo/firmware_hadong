#ifndef __GPIO_H__
#define __GPIO_H__

#include <xc.h>
#include <p33EP32GP502.h>

#define SONAR_TRIGGER_UP    LATBbits.LATB10 = 1;
#define SONAR_TRIGGER_DOWN  LATBbits.LATB10 = 0;

#define LIMIT_ROTATE        PORTBbits.RB14
#define LIMIT_EMPTY         PORTBbits.RB15

extern void InitGPIO();
extern void InitInputCapture();


#endif