#include "config_cocoa_general.h"

void INIT_PINS(void){
    // Init all AN pin as digital
    ANSEL_ALL = DIGITAL;
        
    TRISCbits.TRISC12 = 1;  // OSC
    TRISBbits.TRISB13 = 0;  // Ethernet Reset
    
    
    // UART-------------------------------------
    TRISDbits.TRISD10 = 0;  // U3 TX
    TRISDbits.TRISD11 = 1;  // U3 RX
    RPD10Rbits.RPD10R = 0b0001;
    U3RXRbits.U3RXR = 0b0011;
            
    TRISCbits.TRISC13 = 0;  // U1 TX
    TRISCbits.TRISC14 = 1;  // U1 RX
    RPC13Rbits.RPC13R = 0b0001;
    U1RXRbits.U1RXR = 0b0111;
    // -----------------------------------------
    
    
    // CAN--------------------------------------
    TRISFbits.TRISF0 = 1;   // CAN RX
    TRISFbits.TRISF1 = 0;   // CAN TX
    
    TRISDbits.TRISD4 = 1;   // CAN RX
    TRISDbits.TRISD5 = 0;   // CAN TX
    // -----------------------------------------
    
    
    // LED--------------------------------------
    TRISGbits.TRISG6 = 0;   // Debug LED A
    TRISGbits.TRISG7 = 0;   // Debug LED B
    // -----------------------------------------
    
    
    // ID---------------------------------------
    TRISCbits.TRISC15 = 1;  // ID Selection
    TRISBbits.TRISB14 = 1;  // ID Selection
    // -----------------------------------------
    
}


void INIT_UART_1(unsigned int baudRate, unsigned int stop_bit, unsigned int parity_mode){
    // baudRate = XX (bps)
    
    // stop_bit = 1 or 2
    
    // parity_mode 0 = none
    //             1 = odd
    //             2 = even                   
    
    // clear uart
    U1MODE = 0;
    
    // setting
    U1MODEbits.UEN = 0;             // RXTX mode
    U1STAbits.UTXISEL0=0;           // UART_INTERRUPT_ON_TX_NOT_FULL
	U1STAbits.UTXISEL1=0;           // UART_INTERRUPT_ON_TX_NOT_FULL
    U1STAbits.URXISEL = 0;          // Rx interrupt mode selection
    U1STAbits.URXISEL0 = 0;
    U1STAbits.URXISEL1 = 0;
    //U1MODEbits.RTSMD = 0;
    
    
    if(parity_mode == 0){
        // 8bit+none
        U1MODEbits.PDSEL = 0b00;
    }else if(parity_mode == 1){
        // 8bit+odd
        U1MODEbits.PDSEL = 0b10;
    }else if(parity_mode == 2){
        // 8bit+even
        U1MODEbits.PDSEL = 0b01;
    }
    U1MODEbits.STSEL = stop_bit - 1;
    U1MODEbits.BRGH = 0;            // standard mode
    
    if(baudRate > 230400){
        U1MODEbits.BRGH = 1;
        U1BRG = (unsigned int)(((double)PBCLK)/((double)baudRate)/4. - 1.);
    }else{
        U1MODEbits.BRGH = 0;
        U1BRG = (unsigned int)(((double)PBCLK)/((double)baudRate)/16. - 1.);
    }

    U1STAbits.UTXEN = 1;            // enable TX
    U1STAbits.URXEN = 0;            // enalbe RX
    U1MODEbits.ON = 1;              // enable U1  
}

void INIT_UART_3(unsigned int baudRate, unsigned int stop_bit, unsigned int parity_mode){
    // baudRate = XX (bps)
    
    // stop_bit = 1 or 2
    
    // parity_mode 0 = none
    //             1 = odd
    //             2 = even                   
    
    // clear uart
    U3MODE = 0;
    
    // setting
    U3MODEbits.UEN = 0;             // RXTX mode
    U3STAbits.UTXISEL0=0;           // UART_INTERRUPT_ON_TX_NOT_FULL
	U3STAbits.UTXISEL1=0;           // UART_INTERRUPT_ON_TX_NOT_FULL
    U3STAbits.URXISEL = 0;          // Rx interrupt mode selection
    U3STAbits.URXISEL0 = 0;
    U3STAbits.URXISEL1 = 0;
    //U3MODEbits.RTSMD = 0;
    
    
    if(parity_mode == 0){
        // 8bit+none
        U3MODEbits.PDSEL = 0b00;
    }else if(parity_mode == 1){
        // 8bit+odd
        U3MODEbits.PDSEL = 0b10;
    }else if(parity_mode == 2){
        // 8bit+even
        U3MODEbits.PDSEL = 0b01;
    }
    U3MODEbits.STSEL = stop_bit - 1;
    U3MODEbits.BRGH = 0;            // standard mode
    
    if(baudRate > 230400){
        U3MODEbits.BRGH = 1;
        U3BRG = (unsigned int)(((double)PBCLK)/((double)baudRate)/4. - 1.);
    }else{
        U3MODEbits.BRGH = 0;
        U3BRG = (unsigned int)(((double)PBCLK)/((double)baudRate)/16. - 1.);
    }

    U3STAbits.UTXEN = 1;            // enable TX
    U3STAbits.URXEN = 0;            // enalbe RX
    U3MODEbits.ON = 1;              // enable U3 
    
    IPC39bits.U3RXIP = 4;           // priority
    IPC39bits.U3RXIS = 1;           // sub priority
    IFS4bits.U3RXIF = 0;            // U2RX INT flag clear
    IEC4bits.U3RXIE = 1;            // U2RX INT enalbe
}

