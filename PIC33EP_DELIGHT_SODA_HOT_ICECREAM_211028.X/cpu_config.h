#ifndef __CPU_CONFIG_H__
#define __CPU_CONFIG_H__

#include <p33EP32GP502.h>

#pragma config ICS = PGD2
#pragma config JTAGEN = OFF

#pragma config ALTI2C1 = OFF
#pragma config ALTI2C2 = OFF

//#pragma config WDTWIN = WIN25
//#pragma config WDTPOST = PS64
//#pragma config WDTPRE = PR32
//#pragma config WINDIS = OFF
#pragma config FWDTEN = OFF

#pragma config PLLKEN = ON

#pragma config POSCMD = EC
#pragma config OSCIOFNC = ON    // ON: gpio     OFF: ocs2 output
#pragma config IOL1WAY = OFF    // ON: rp fix    OFF: rp reconfigurable
#pragma config FCKSM = CSECMD
#pragma config FNOSC = PRIPLL
#pragma config IESO = OFF       // start up - user selected clock

//#pragma config GWRP = ON
//#pragma config GCP = ON

//#pragma config UID0 = 


#endif