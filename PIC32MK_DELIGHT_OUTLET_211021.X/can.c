//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>
#include "sys/kmem.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>  
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <sys/attribs.h>

#include "can.h"
#include "board_definition.h"

#define NUM_OF_CAN_TX_CHANNEL       2
#define NUM_OF_CAN_RX_CHANNEL       3
#define CAN_TX_BUFF_SIZE            200


// CAN_TX_MSG_BUFFER --> 16byte
// size : buffer number * FIFO number
// FIFO0 - Transmit - 16 Message Buffer
// FIFO1 - Transmit - 16 Message Buffer
// FIFO2 - Receive  - 16 Message Buffer
// FIFO3 - Receive  - 16 Message Buffer
// FIFO4 - FIFO31   - Not used
static CAN_TX_MSG_BUFFER __attribute__((coherent, aligned(16))) can_message_buffer[16*5];
static CAN_TX_MSG_BUFFER  *drv_Message;
static CAN_TX_MSG_BUFFER  *drv_Message_tmr3;



typedef struct{
	unsigned int head;
	unsigned int tail;
	unsigned int id[CAN_TX_BUFF_SIZE];
	unsigned int dlc[CAN_TX_BUFF_SIZE];
	unsigned char data[CAN_TX_BUFF_SIZE][8];
} CAN_TX_RING;

static CAN_TX_RING _can_tx_ring;
static CAN_TX_RING _can_tx_ring_tmr3;


static void _CAN_RECEIVE_CHANNEL_SET(int channel_num, uint32_t channel_size);
static void _CAN_ENABLE_FILTER(int filter_num);
static void _CAN_DISABLE_FILTER(int filter_num);
static void _CAN_FILTER_MASK_CONFIG(int mask_num, uint32_t mask_bit);
static void _CAN_FILTER_CONFIG(int filter_num, uint32_t id);
static void _CAN_FILTER_2_CHANNEL_LINK(int filter_num, int mask_num, int channel_num);

static void _Init_CAN_TX_Ring();
static void _Init_CAN_TX_Ring_TMR3();




int CANID_REF1 = REFBASE_TXDF;
int CANID_ENC1 = ENCBASE_RXDF;
int CANID_STAT1 = STATBASE_RXDF;
int CANID_DATA1 = DATABASE_RXDF;
int CANID_GEN1 = GENBASE_RXDF;

int CANID_REF2 = REFBASE_TXDF;
int CANID_ENC2 = ENCBASE_RXDF;
int CANID_STAT2 = STATBASE_RXDF;
int CANID_DATA2 = DATABASE_RXDF;
int CANID_GEN2 = GENBASE_RXDF;



void InitCAN(){
    _Init_CAN_TX_Ring();
    //_Init_CAN_TX_Ring_TMR3();
    
    
    RPC0Rbits.RPC0R = 0b01100;      // C1TX     -->     see TABLE 13-2 OUTPUT PIN SELECTION
    C1RXRbits.C1RXR = 0b0110;       // C1RX     -->     see TABLE 13-1 INPUT PIN SELECTION
    
    C1CONbits.ON = 1;               // switch the CAN module on
    
    C1CONbits.REQOP = 0x04;         // switch the CAN module to configuration mode
    while(C1CONbits.OPMOD != 0x04); // wait until the switch is complete
   
    

    // set the baud rate to 1Mbps-----
    // (Propagation Segment + Phase Segment 1) must be greater than or equal to the length of Phase Segment 2
    // Phase Segment 2 must be greater than SJW
    // F_TQ = N * F_baud, 8 <= N <= 25
    // Propagation Segment = 1 Tq
    // Phase Seg 1 = 4 Tq
    // Phase Seg 2 = 4 Tq
    // Sync Jump Width = 2 Tq
    C1CFGbits.SEG2PHTS = 1;         // Phase seg 2 is freely programmable
    C1CFGbits.PRSEG = 0;            // Propagation seg is 1 Tq
    C1CFGbits.SEG1PH = 3;           // Phase seg 1 is 4 Tq
    C1CFGbits.SEG2PH = 3;           // Phase seg 2 is 4 Tq    
    C1CFGbits.SJW = 0;              // Sync jump width is 1 Tq
    C1CFGbits.SAM = 1;              // Sample bit 3 times
    C1CFGbits.BRP = 2;              // BRP = F_sys/(2*F_TQ)-1

    
    // assign the buffer area to the CAN module -----
    C1FIFOBA = KVA_TO_PA(can_message_buffer);
    // FIFO0 - Transmit - 16 Message Buffer
    // FIFO1 - Transmit - 16 Message Buffer
    // FIFO2 - Receive  - 16 Message Buffer
    // FIFO3 - Receive  - 16 Message Buffer
    // FIFO4 - Receive  - 16 Message Buffer
    // FIFO5 - FIFO31   - Not used
    C1FIFOCON0bits.FSIZE = 15;
    C1FIFOCON0bits.TXEN = 1;        //tx   
    C1FIFOCON1bits.FSIZE = 15;
    C1FIFOCON1bits.TXEN = 1;        //tx 
    _CAN_RECEIVE_CHANNEL_SET(2, 16);  // CAN channel 2, size 16
    C1FIFOINT2bits.RXNEMPTYIE = 1;
    _CAN_RECEIVE_CHANNEL_SET(3, 16);  // CAN channel 3, size 16
    C1FIFOINT3bits.RXNEMPTYIE = 1;
    _CAN_RECEIVE_CHANNEL_SET(4, 16);  // CAN channel 4, size 16
    C1FIFOINT4bits.RXNEMPTYIE = 1;
    
    
    // *** *** *** *** *** *** Filter *** *** *** *** *** *** //
    // Filter disable before the filter mask setting
    int filter_num;
    for(filter_num=0; filter_num<16; ++filter_num){
        _CAN_DISABLE_FILTER(filter_num);
    }

    // Filter mask setting
    _CAN_FILTER_MASK_CONFIG(0, 0x7FF);         // mask0 

    // Filter setting
    _CAN_FILTER_CONFIG(0, CANID_REF1);       // filter 0 -- ID: 0x10+BNO
    _CAN_FILTER_CONFIG(1, CANID_GEN1);       // filter 1 -- ID: 0x210+BNO
    _CAN_FILTER_CONFIG(2, CANID_REF2);       // filter 2 -- ID: 0x04
    _CAN_FILTER_CONFIG(3, CANID_GEN2);       // filter 3 -- ID: 0x05
    _CAN_FILTER_2_CHANNEL_LINK(0, 0, 2);     // filter0, mask0, FIFO channel2
    _CAN_FILTER_2_CHANNEL_LINK(1, 0, 3);     // filter1, mask0, FIFO channel3
    _CAN_FILTER_2_CHANNEL_LINK(2, 0, 4);     // filter2, mask0, FIFO channel4
    _CAN_FILTER_2_CHANNEL_LINK(3, 0, 4);     // filter3, mask0, FIFO channel4
    
    // Filter enable
    _CAN_ENABLE_FILTER(0);
    _CAN_ENABLE_FILTER(1);
    _CAN_ENABLE_FILTER(2);
    _CAN_ENABLE_FILTER(3);
    // *** *** *** *** *** *** Filter *** *** *** *** *** *** //
        
    
    C1CONbits.REQOP = 0x00;         // switch the CAN module to normal mode.
    while(C1CONbits.OPMOD != 0x00); // wait until the switch is complete
    
    
    // set the interrupt
    C1INTbits.RBIE = 1;             // receive interrupt
    IPC41bits.CAN1IP = 4;           // priority level
    IPC41bits.CAN1IS = 0;           // subpriority level
    IFS5bits.CAN1IF = 0;            // interrupt flag clear
    IEC5bits.CAN1IE = 1;            // interrupt enable
}

void InitCANID(int jmc){
    CANID_REF1 = REFBASE_TXDF + jmc*2+0;
    CANID_ENC1 = ENCBASE_RXDF + jmc*2+0;
    CANID_STAT1 = STATBASE_RXDF + jmc*2+0;
    CANID_DATA1 = DATABASE_RXDF + jmc*2+0;
    CANID_GEN1 = GENBASE_RXDF + jmc*2+0;
    
    CANID_REF2 = REFBASE_TXDF + jmc*2+1;
    CANID_ENC2 = ENCBASE_RXDF + jmc*2+1;
    CANID_STAT2 = STATBASE_RXDF + jmc*2+1;
    CANID_DATA2 = DATABASE_RXDF + jmc*2+1;
    CANID_GEN2 = GENBASE_RXDF + jmc*2+1;    
}



char CAN_WRITE(int address, uint8_t DLC, uint8_t *message){
    int count = 0;
        
    if( (C1FIFOINT1>>10) & 0x00000001 == 1 ){  // if TXNFULLIF
        drv_Message = (CAN_TX_MSG_BUFFER*)PA_TO_KVA1(C1FIFOUA1);
        drv_Message->msgSID.sid     = address;
        drv_Message->msgEID.eid     = 0;
        drv_Message->msgEID.ide     = 0; 

        if (DLC > 8) DLC = 8;

        drv_Message->msgEID.data_length_code = DLC;
        while(count < DLC){
            drv_Message->data[count++] = *message++;
        }

        //Update CAN module and then transmit data on the bus;
        C1FIFOCON1 = C1FIFOCON1 | (1<<13);  // set UINC
        C1FIFOCON1 = C1FIFOCON1 | (1<<3);   // set TXREQ
        return true;
    }
    return false;
}


char CAN_WRITE_TMR3(int address, uint8_t DLC, uint8_t *message){    
    int count = 0;
         
    if( (C1FIFOINT0>>10) & 0x00000001 == 1 ){  // if TXNFULLIF
        drv_Message_tmr3 = (CAN_TX_MSG_BUFFER*)PA_TO_KVA1(C1FIFOUA0);
        drv_Message_tmr3->msgSID.sid     = address;
        drv_Message_tmr3->msgEID.eid     = 0;
        drv_Message_tmr3->msgEID.ide     = 0; 

        if (DLC > 8) DLC = 8;

        drv_Message_tmr3->msgEID.data_length_code = DLC;
        while(count < DLC){
            drv_Message_tmr3->data[count++] = *message++;
        }

        //Update CAN module and then transmit data on the bus;
        C1FIFOCON0 = C1FIFOCON0 | (1<<13);  // set UINC
        C1FIFOCON0 = C1FIFOCON0 | (1<<3);   // set TXREQ
        return true;
    }
    return false;
}



char CAN_READ(uint32_t *address, uint8_t *message, uint8_t *length){
    static unsigned int *initFIFOINT = (unsigned int*)&C1FIFOINT0;
    static unsigned int *initFIFOUA  = (unsigned int*)&C1FIFOUA0;    
    static unsigned int *initFIFOCON = (unsigned int*)&C1FIFOCON0;
    
    unsigned int *tempFIFOINT;
    unsigned int *tempFIFOUA;
    unsigned int *tempFIFOCON;
    
    int channel = -1;
    int dlc = 0;
    CAN_RX_MSG_BUFFER *RxMessage = NULL;    
            
    int ch;
    for(ch=NUM_OF_CAN_TX_CHANNEL; ch<NUM_OF_CAN_TX_CHANNEL+NUM_OF_CAN_RX_CHANNEL; ++ch){
        tempFIFOINT = &(initFIFOINT[16*ch]);
        if( (*tempFIFOINT) & 0x00000001 == 1 ){  // if RXNEMPTYIF
            channel = ch;
            break;
        }
    }
    
    if(channel < 0)
        return false;
    
    tempFIFOUA = &(initFIFOUA[16*channel]);
    RxMessage = (CAN_RX_MSG_BUFFER*)PA_TO_KVA1(*tempFIFOUA);
    
    if(RxMessage != NULL){
//        if(RxMessage->msgEID.ide){ //EID Address
//            *address = (RxMessage->msgSID.sid << 18);
//            *address |= ((0x3FFFF)&(RxMessage->msgEID.eid));
//        }else{
            *address = RxMessage->msgSID.sid; 
//        }

        *length = RxMessage->msgEID.data_length_code;
        if(RxMessage->msgEID.data_length_code > 0){
            while(dlc < RxMessage->msgEID.data_length_code){ 
                 *message++ = RxMessage->data[dlc++];
            }
        }
        //Process the message received

        /* Call the PLIB_CAN_ChannelUpdate function to let
        the CAN module know that the message processing
        is done. */
        tempFIFOCON = &(initFIFOCON[16*channel]);
        *tempFIFOCON = *tempFIFOCON | (1<<13);  // set UINC
        
        return true;
    }
    return false;
}



int Push_CAN_TX_Ring(unsigned int id, unsigned int dlc, unsigned char *data){
    unsigned int new_head = (_can_tx_ring.head+1)%CAN_TX_BUFF_SIZE;

	if(new_head == _can_tx_ring.tail){
		return -1;
	}
	_can_tx_ring.dlc[new_head] = dlc;
	_can_tx_ring.id[new_head] = id;
	memcpy(&_can_tx_ring.data[new_head], data, 8);
    _can_tx_ring.head = new_head;
    
	return 0;
}
int Handler_CAN_TX_Ring(){
	unsigned int new_tail;
	if(_can_tx_ring.head != _can_tx_ring.tail){
		new_tail = (_can_tx_ring.tail+1)%CAN_TX_BUFF_SIZE;

        if(CAN_WRITE(_can_tx_ring.id[new_tail], _can_tx_ring.dlc[new_tail], (unsigned char*)_can_tx_ring.data[new_tail]) == true)
            _can_tx_ring.tail = new_tail;
		return 1;
	}
	return 0;
}
static void _Init_CAN_TX_Ring(){
	memset(&_can_tx_ring, 0, sizeof(CAN_TX_RING));
}



int Push_CAN_TX_Ring_TMR3(unsigned int id, unsigned int dlc, unsigned char *data){
    unsigned int new_head = (_can_tx_ring_tmr3.head+1)%CAN_TX_BUFF_SIZE;

	if(new_head == _can_tx_ring_tmr3.tail){
		return -1;
	}
	_can_tx_ring_tmr3.dlc[new_head] = dlc;
	_can_tx_ring_tmr3.id[new_head] = id;
	memcpy(&_can_tx_ring_tmr3.data[new_head], data, 8);
    _can_tx_ring_tmr3.head = new_head;
    
	return 0;
}
int Handler_CAN_TX_Ring_TMR3(){
	unsigned int new_tail;
	if(_can_tx_ring_tmr3.head != _can_tx_ring_tmr3.tail){
		new_tail = (_can_tx_ring_tmr3.tail+1)%CAN_TX_BUFF_SIZE;

        if(CAN_WRITE_TMR3(_can_tx_ring_tmr3.id[new_tail], _can_tx_ring_tmr3.dlc[new_tail], (unsigned char*)_can_tx_ring_tmr3.data[new_tail]) == true)
            _can_tx_ring_tmr3.tail = new_tail;
		return 1;
	}
	return 0;
}
static void _Init_CAN_TX_Ring_TMR3(){
	memset(&_can_tx_ring_tmr3, 0, sizeof(CAN_TX_RING));
}



static void _CAN_RECEIVE_CHANNEL_SET(int channel_num, uint32_t channel_size){
    unsigned int *ChannelRcvSetControlReg = (unsigned int*)&C1FIFOCON0;
    
    // Set the channel for receive
    //#define _C1FIFOCON0_TXEN_POSITION                0x00000007
    _SFR_BIT_CLEAR(&ChannelRcvSetControlReg[16 * channel_num], _C1FIFOCON0_TXEN_POSITION);
    
    //#define _C1FIFOCON0_DONLY_LENGTH                 0x00000001
    _SFR_BIT_CLEAR(&ChannelRcvSetControlReg[16 * channel_num], _C1FIFOCON0_DONLY_LENGTH);
    
    // write the FIFO size
    //#define _C1FIFOCON0_FSIZE_POSITION               0x00000010
    //#define _C1FIFOCON0_FSIZE_MASK                   0x001F0000
    _SFR_FIELD_WRITE(&ChannelRcvSetControlReg[16 * channel_num], _C1FIFOCON0_FSIZE_MASK, _C1FIFOCON0_FSIZE_POSITION, channel_size-1);
}

static void _CAN_ENABLE_FILTER(int filter_num){
    unsigned int *filterControlReg = (unsigned int*)&C1FLTCON0;
    int regIndex = filter_num/4;
    int regOffset = filter_num%4;
    
    //#define _C1FLTCON0_FLTEN0_POSITION               0x00000007
    _SFR_BIT_SET(&filterControlReg[4 * regIndex], (_C1FLTCON0_FLTEN0_POSITION+(regOffset*8)));
}

static void _CAN_DISABLE_FILTER(int filter_num){
    unsigned int *filterControlReg = (unsigned int*)&C1FLTCON0;
    int regIndex = filter_num/4;
    int regOffset = filter_num%4;
    
    //#define _C1FLTCON0_FLTEN0_POSITION               0x00000007
    _SFR_BIT_CLEAR(&filterControlReg[4 * regIndex], (_C1FLTCON0_FLTEN0_POSITION+(regOffset*8)));
}

static void _CAN_FILTER_MASK_CONFIG(int mask_num, uint32_t mask_bit){
    unsigned int *maskControlReg = (unsigned int*)&C1RXM0;
    
    //#define _C1RXM0_SID_POSITION                     0x00000015
    //#define _C1RXM0_SID_MASK                         0xFFE00000
    mask_bit &= 0x7FF;  //sid
    _SFR_FIELD_WRITE(&maskControlReg[4 * mask_num], _C1RXM0_SID_MASK, _C1RXM0_SID_POSITION, mask_bit);
    
    //#define _C1RXM1_MIDE_POSITION                    0x00000013
    _SFR_BIT_SET(&maskControlReg[4 * mask_num], _C1RXM1_MIDE_POSITION);
}

static void _CAN_FILTER_CONFIG(int filter_num, uint32_t id){
    unsigned int *filterControlReg = (unsigned int*)&C1RXF0;
    id &= 0x7FF;    //sid
    
    //#define _C1RXF0_SID_POSITION                     0x00000015
    //#define _C1RXF0_SID_MASK                         0xFFE00000
    _SFR_FIELD_WRITE(&filterControlReg[4 * filter_num], _C1RXF0_SID_MASK, _C1RXF0_SID_POSITION, id );
    
    //#define _C1RXF0_EXID_POSITION                    0x00000013
    _SFR_BIT_CLEAR(&filterControlReg[4 * filter_num], _C1RXF0_EXID_POSITION);
}

static void _CAN_FILTER_2_CHANNEL_LINK(int filter_num, int mask_num, int channel_num){
    unsigned int *filterControlReg = (unsigned int*)&C1FLTCON0;
    int regIndex = filter_num/4;
    int regOffset = filter_num%4;
    
    //#define _C1FLTCON0_MSEL0_POSITION                0x00000005
    //#define _C1FLTCON0_MSEL0_MASK                    0x00000060
    _SFR_FIELD_WRITE(&filterControlReg[regIndex*4], (_C1FLTCON0_MSEL0_MASK<<(regOffset*8)), (_C1FLTCON0_MSEL0_POSITION+(regOffset*8)), mask_num);
    
    //#define _C1FLTCON0_FSEL0_POSITION                0x00000000
    //#define _C1FLTCON0_FSEL0_MASK                    0x0000001F
    _SFR_FIELD_WRITE(&filterControlReg[regIndex*4], (_C1FLTCON0_FSEL0_MASK<<(regOffset*8)), (_C1FLTCON0_FSEL0_POSITION+(regOffset*8)), channel_num);
}
