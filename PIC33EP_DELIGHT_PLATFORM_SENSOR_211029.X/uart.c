#include "uart.h"

void InitUART(){
    RPINR18bits.U1RXR = 44;     // UART RX  RPI44 (RB12)
    RPOR4bits.RP43R = 0b000001; // UART TX  RP43 (RB11)
    
    U1MODEbits.STSEL = 0;   // 1 stop bit
    U1MODEbits.PDSEL = 0;   // no parity, 8bit data
    U1MODEbits.ABAUD = 0;   // auto-baud disable
    U1MODEbits.BRGH = 0;    // high-baud disable
    U1MODEbits.UEN = 0;
    U1BRG = BRGVAL;
    
    U1STAbits.UTXISEL0 = 0; // Interrupt after one TX character is transmitted
    U1STAbits.UTXISEL1 = 0;
    U1STAbits.URXISEL = 0; // Interrupt after one RX character is received
    U1MODEbits.UARTEN = 1; // Enable UART
    U1STAbits.UTXEN = 1; // Enable UART TX
}

 void __attribute__((interrupt, no_auto_psv)) _U1TXInterrupt(void){ 
    IFS0bits.U1TXIF = 0; // Clear TX Interrupt flag 
 } 
 void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void){ 
    IFS0bits.U1RXIF = 0; // Clear RX Interrupt flag
 } 