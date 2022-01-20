#include <proc/p32mk1024mcf064.h>
#include <sys/attribs.h>

#include "uart.h"


void InitUART(unsigned int baud_rate){
    // Rx only
    // RX: pin81  RPC8
    TRISCbits.TRISC8 = 1;
    
    U2RXRbits.U2RXR = 0b0110;
    
    // Inversion
    U2MODEbits.RXINV = 0;           // idle == 1
    
    U2MODEbits.UEN = 0;             // RXTX mode
    U2STAbits.UTXISEL = 0;          // not use tx interrupt
    U2STAbits.URXISEL = 0;          // Rx interrupt not empty
    //U2MODEbits.RTSMD = 0;
    
    U2MODEbits.PDSEL = 0;           // 8 bit no parity
    U2MODEbits.STSEL = 0;           // 1stop bit
    U2MODEbits.BRGH = 0;            // standard mode
    U2MODEbits.CLKSEL = 0;          // PBCLK
    
    U2BRG = 60000000/(16*baud_rate) - 1;
    
    U2STAbits.URXEN = 1;            // enalbe RX
    U2MODEbits.ON = 1;              // enable U1    
    
    
    IPC14bits.U2RXIP = 2;           // priority
    IPC14bits.U2RXIS = 1;           // sub priority
    IFS1bits.U2RXIF = 0;            // U1RX INT flag clear
    IEC1bits.U2RXIE = 1;            // U1RX INT enalbe
}



void __ISR(_UART2_RX_VECTOR, IPL2AUTO) Handler_UART2_RX(){
    unsigned char temp_ch;
    if(U2STAbits.URXDA == 1){
        temp_ch = U1RXREG;
        // echo
        while(U1STAbits.UTXBF == 1)
            ;
        U1TXREG = temp_ch;
        
    }
    IFS1bits.U2RXIF = 0;
}