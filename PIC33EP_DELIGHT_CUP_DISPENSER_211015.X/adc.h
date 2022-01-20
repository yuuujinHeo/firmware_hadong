#ifndef __ADC_H__
#define __ADC_H__

#include <xc.h>
#include <p33EP32GP502.h>

extern void InitADC();

// I * R * G = V   
// I = V/R/G
// (3.3/1024) / 0.05 / 20 = 3.223 mA/digit
#define CURRENT_CONSTANT    3


#endif