#ifndef __UART_H__
#define __UART_H__

#include <xc.h>
#include <p33EP32GP502.h>

#define FCY 60000000
#define BAUDRATE 9600
#define BRGVAL ((FCY/BAUDRATE)/16)-1

void InitUART();

#endif