#include "config_cocoa_general.h"
#include "LAN2CAN.h"

void WriteUART1(unsigned char *data, int len){
    int i=0;
    for(i=0; i<len; i++){
        while(U1STAbits.UTXBF==1);
            U1TXREG =data[i];
    }
}

void WriteUART3(unsigned char *data, int len){
    int i=0;
    for(i=0; i<len; i++){
        while(U3STAbits.UTXBF==1);
            U3TXREG =data[i];
    }
}
