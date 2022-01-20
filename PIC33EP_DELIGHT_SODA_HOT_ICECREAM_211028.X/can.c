#include "can.h"

#ifdef _HAS_DMA_
__eds__ extern ECAN1MSGBUF Ecan1msgBuf __attribute__( (eds, space(dma), aligned(ECAN1_MSG_BUF_LENGTH * 16)) );
#else
__eds__ extern ECAN1MSGBUF Ecan1msgBuf  __attribute__( (eds, space(xmemory), aligned(ECAN1_MSG_BUF_LENGTH * 16)) );
#endif


static void clearRxFlags(unsigned char buffer_number);
static void Ecan1ConfigureRxAcptMask(int mask_number, long mask);
static void Ecan1ConfigureRxAcptFilter(int filter_number, long id, unsigned int buffer_point, unsigned int mask_source);

extern int BOARD_TYPE;

void InitCAN(){
    TRISBbits.TRISB2 = 1;
    TRISBbits.TRISB3 = 0;
    
    RPINR26bits.C1RXR = 34;		// CANRX  RPI34 (RB2)
    RPOR0bits.RP35R = 0b001110; // CANTX  RP35 (RB3)
    
	
	// put the module in configuration mode
	C1CTRL1bits.REQOP=4;
	while(C1CTRL1bits.OPMODE != 4);
			
    
	// FCAN is selected to be FCY
    // FCAN = FCY = 40MHz
	C1CTRL1bits.CANCKS = 0;

	
	// Bit Time = (Sync Segment + Propagation Delay + Phase Segment 1 + Phase Segment 2)=20*TQ
	// Phase Segment 1 = 8TQ
	// Phase Segment 2 = 6Tq
	// Propagation Delay = 5Tq
	// Sync Segment = 1TQ
	// CiCFG1<BRP> =(FCAN /(2 ??BAUD))?1
	// BIT RATE OF 1Mbps	
	C1CFG1bits.BRP = BRP_VAL;
	// Synchronization Jump Width set to 4 TQ
	C1CFG1bits.SJW = 0x3;
	// Phase Segment 1 time is 8 TQ
	C1CFG2bits.SEG1PH=0x7;
	// Phase Segment 2 time is set to be programmable
	C1CFG2bits.SEG2PHTS = 0x1;
	// Phase Segment 2 time is 6 TQ
	C1CFG2bits.SEG2PH = 0x5;
	// Propagation Segment time is 5 TQ
	C1CFG2bits.PRSEG = 0x4;
	// Bus line is sampled three times at the sample point
	C1CFG2bits.SAM = 0x1;
	
	
	C1FCTRLbits.FSA = 8;	// FIFO starts at message buffer 8
	
	// 16 CAN Messages to be buffered in DMA RAM
	C1FCTRLbits.DMABS = 0b100;
	

	// Filter configuration
	C1CTRL1bits.WIN=1;
	C1FEN1 = 0x0000;		// disable all acceptance filters
	C1CTRL1bits.WIN=0;
	
	
	/* Mask configuration */
	Ecan1ConfigureRxAcptMask(0, 0xFFFF);

	//****************************** SETTING CAN IDs ***********************************************//
    if(BOARD_TYPE == BOARD_TYPE_SODA){
        Ecan1ConfigureRxAcptFilter(0, 0x141, 0xFF, 0);			//	store the received message to FIFO
    }else if(BOARD_TYPE == BOARD_TYPE_HOT){
        Ecan1ConfigureRxAcptFilter(0, 0x142, 0xFF, 0);			//	store the received message to FIFO
    }else if(BOARD_TYPE == BOARD_TYPE_ICECREAM){
        Ecan1ConfigureRxAcptFilter(0, 0x143, 0xFF, 0);			//	store the received message to FIFO
    }else{
        Ecan1ConfigureRxAcptFilter(0, 0x140, 0xFF, 0);			//	store the received message to FIFO
    }
    
//    Ecan1ConfigureRxAcptFilter(1, 0x30, 0xFF, 0);			//	store the received message to FIFO
//    Ecan1ConfigureRxAcptFilter(2, 0x40, 0xFF, 0);			//	store the received message to FIFO
	//**********************************************************************************************//
	
	/* put the module in normal mode */
	C1CTRL1bits.REQOP=0;
	while(C1CTRL1bits.OPMODE != 0);	

	/* clear the buffer full flags */
	C1RXFUL1=0x0000;

	/* clear the rx buffer interrupt flag  */
	C1INTFbits.RBIF=0;
	
	/* clear the buffer and overflow flags */
	C1RXFUL1=C1RXFUL2=C1RXOVF1=C1RXOVF2=0x0000;

	C1TR01CONbits.TXEN0 = 1; /* ECAN1, Buffer 0 is a Transmit Buffer */
	C1TR01CONbits.TXEN1 = 1; /* ECAN1, Buffer 1 is a Transmit Buffer */
	C1TR23CONbits.TXEN2 = 1; /* ECAN1, Buffer 2 is a Transmit Buffer */
	C1TR23CONbits.TXEN3 = 1; /* ECAN1, Buffer 3 is a Transmit Buffer */
	C1TR45CONbits.TXEN4 = 1; /* ECAN1, Buffer 4 is a Transmit Buffer */
	C1TR45CONbits.TXEN5 = 1; /* ECAN1, Buffer 5 is a Transmit Buffer */
	C1TR67CONbits.TXEN6 = 1; /* ECAN1, Buffer 6 is a Transmit Buffer */
	C1TR67CONbits.TXEN7 = 1; /* ECAN1, Buffer 7 is a Transmit Buffer */
		
	C1TR01CONbits.TX0PRI=0b11; 	/* Message Buffer 0 Priority Level  ; highest priority*/
	C1TR01CONbits.TX1PRI=0b11; 	/* Message Buffer 1 Priority Level  ; highest priority*/	
	C1TR23CONbits.TX2PRI=0b11; 	/* Message Buffer 2 Priority Level  ; highest priority*/
	C1TR23CONbits.TX3PRI=0b11; 	/* Message Buffer 3 Priority Level  ; highest priority*/
	C1TR45CONbits.TX4PRI=0b11; 	/* Message Buffer 4 Priority Level  ; highest priority*/
	C1TR45CONbits.TX5PRI=0b11; 	/* Message Buffer 5 Priority Level  ; highest priority*/
	C1TR67CONbits.TX6PRI=0b11; 	/* Message Buffer 6 Priority Level  ; highest priority*/
	C1TR67CONbits.TX7PRI=0b11; 	/* Message Buffer 7 Priority Level  ; highest priority*/
	
	C1INTEbits.RBIE=1;	/* Enable Receive interrupt */
	IPC8bits.C1IP = 6;	// Interrupt priority
	IEC2bits.C1IE=1;	/* Enable ECAN1 Interrupt */     
}

void InitDMA(){
    /* setup channel 0 for peripheral indirect addressing mode 
    normal operation, word operation and select as Tx to peripheral */
	DMA0CON = 0x2020; 
    /* setup the address of the peripheral ECAN1 (C1TXD) */ 
	DMA0PAD = (int)&C1TXD;
	/* Set the data block transfer size of 8 words */
	DMA0CNT = 7;
 	/* automatic DMA Tx initiation by DMA request */
	DMA0REQ = 0x0046;
	/* DPSRAM start adddress offset value */ 
#ifdef _HAS_DMA_
	DMA0STAL = __builtin_dmaoffset(Ecan1msgBuf);	
    DMA0STAH = __builtin_dmapage(Ecan1msgBuf);	
#else
    DMA0STAL = (uint16_t)(int_least24_t)(&Ecan1msgBuf);	
    DMA0STAH = 0;	
#endif    
	/* enable the channel */
	DMA0CONbits.CHEN=1;
	

    /* setup channel 2 for peripheral indirect addressing mode 
    normal operation, word operation and select as Rx to peripheral */
   	DMA2CON = 0x0020;
    /* setup the address of the peripheral ECAN1 (C1RXD) */ 
	DMA2PAD = (int)&C1RXD;
 	/* Set the data block transfer size of 8 */
 	DMA2CNT = 7;
 	/* automatic DMA Rx initiation by DMA request */
	DMA2REQ = 0x0022;	
	/* DPSRAM atart adddress offset value */ 
#ifdef _HAS_DMA_
	DMA2STAL = __builtin_dmaoffset(Ecan1msgBuf);
    DMA2STAH = __builtin_dmapage(Ecan1msgBuf);
#else
    DMA2STAL = (uint16_t)(int_least24_t)(&Ecan1msgBuf);
    DMA2STAH = 0;
#endif
	/* enable the channel */
	DMA2CONbits.CHEN=1;
}	 



void rxECAN(MESSAGE *message){
    message->id=(Ecan1msgBuf[message->buffer_number][0] & 0x1FFC) >> 2;		
    message->data_length=(unsigned char)(Ecan1msgBuf[message->buffer_number][2] & 0x000F);

    message->data[7]=(unsigned char)((Ecan1msgBuf[message->buffer_number][6] & 0xFF00) >> 8);
    message->data[6]=(unsigned char)Ecan1msgBuf[message->buffer_number][6];
    message->data[5]=(unsigned char)((Ecan1msgBuf[message->buffer_number][5] & 0xFF00) >> 8);
    message->data[4]=(unsigned char)Ecan1msgBuf[message->buffer_number][5];
    message->data[3]=(unsigned char)((Ecan1msgBuf[message->buffer_number][4] & 0xFF00) >> 8);
    message->data[2]=(unsigned char)Ecan1msgBuf[message->buffer_number][4];
    message->data[1]=(unsigned char)((Ecan1msgBuf[message->buffer_number][3] & 0xFF00) >> 8);
    message->data[0]=(unsigned char)Ecan1msgBuf[message->buffer_number][3];
    
	clearRxFlags(message->buffer_number);	
}


void clearRxFlags(unsigned char buffer_number)
{
	switch(buffer_number)
	{
		case 8:
			C1RXFUL1bits.RXFUL8 = 0;
			C1RXOVF1bits.RXOVF8 = 0;
			break;
		case 9:
			C1RXFUL1bits.RXFUL9 = 0;
			C1RXOVF1bits.RXOVF9 = 0;
			break;
		case 10:
			C1RXFUL1bits.RXFUL10 = 0;
			C1RXOVF1bits.RXOVF10 = 0;
			break;
		case 11:
			C1RXFUL1bits.RXFUL11 = 0;
			C1RXOVF1bits.RXOVF11 = 0;
			break;
		case 12:
			C1RXFUL1bits.RXFUL12 = 0;
			C1RXOVF1bits.RXOVF12 = 0;
			break;
		case 13:
			C1RXFUL1bits.RXFUL13 = 0;
			C1RXOVF1bits.RXOVF13 = 0;
			break;
		case 14:
			C1RXFUL1bits.RXFUL14 = 0;
			C1RXOVF1bits.RXOVF14 = 0;
			break;
		case 15:
			C1RXFUL1bits.RXFUL15 = 0;
			C1RXOVF1bits.RXOVF15 = 0;
			break;
		case 16:
			C1RXFUL2bits.RXFUL16 = 0;
			C1RXOVF2bits.RXOVF16 = 0;
			break;
		case 17:
			C1RXFUL2bits.RXFUL17 = 0;
			C1RXOVF2bits.RXOVF17 = 0;
			break;
		case 18:
			C1RXFUL2bits.RXFUL18 = 0;
			C1RXOVF2bits.RXOVF18 = 0;
			break;
		case 19:
			C1RXFUL2bits.RXFUL19 = 0;
			C1RXOVF2bits.RXOVF19 = 0;
			break;
		case 20:
			C1RXFUL2bits.RXFUL20 = 0;
			C1RXOVF2bits.RXOVF20 = 0;
			break;
		case 21:
			C1RXFUL2bits.RXFUL21 = 0;
			C1RXOVF2bits.RXOVF21 = 0;
			break;
		case 22:
			C1RXFUL2bits.RXFUL22 = 0;
			C1RXOVF2bits.RXOVF22 = 0;
			break;
		case 23:
			C1RXFUL2bits.RXFUL23 = 0;
			C1RXOVF2bits.RXOVF23 = 0;
			break;
	}
}

void Ecan1ConfigureRxAcptFilter(int filter_number, long id, unsigned int buffer_point, unsigned int mask_source){
	unsigned long sid10_0 = 0;
	unsigned int *C1RXFnSID,*C1BUFPNTn,*C1FMSKSELn, *C1FEN;
		
	C1CTRL1bits.WIN=1;
	
	// Obtain the Address of C1RXFnSID, CiBUFPNTn, CiFMSKSELn and CiFEN register for a given filter number
	C1RXFnSID = (unsigned int *)(&C1RXF0SID + (filter_number << 1));
	C1BUFPNTn = (unsigned int *)(&C1BUFPNT1 + (filter_number >> 2));
	C1FMSKSELn = (unsigned int *)(&C1FMSKSEL1 + (filter_number >> 3));
	C1FEN = (unsigned int *)(&C1FEN1);
	
	
	// Bit-filed manupulation to write to Filter identifier register
	// Filter Standard Identifier
	sid10_0 = (id & 0x7FF);			
	*C1RXFnSID = (sid10_0)<<5;					// Write to C1RXFnSID Register and C1RXFnSIDbits.EXIDE = 0
	*(C1RXFnSID+1) = 0;							// Write to CiRXFnEID Register
	
	*C1BUFPNTn = (*C1BUFPNTn) & (0xFFFF - (0xF << (4 *(filter_number & 3))));		 // clear nibble
	*C1BUFPNTn = ((buffer_point << (4 *(filter_number & 3))) | (*C1BUFPNTn));       	        // Write to C1BUFPNTn Register
	
	*C1FMSKSELn = (*C1FMSKSELn) & (0xFFFF - (0x3 << ((filter_number & 7) * 2)));		 // clear 2 bits
	*C1FMSKSELn = (( mask_source << (2 * (filter_number & 7))) | (*C1FMSKSELn));     	 // Write to C1FMSKSELn Register
	
	*C1FEN = ((0x1 << filter_number) | (*C1FEN)); // Write to C1FEN1 Register
	
	C1CTRL1bits.WIN=0;
}

void Ecan1ConfigureRxAcptMask(int mask_number, long mask){

	unsigned long mask10 = 0;
	unsigned int *C1RXMnSID;

	C1CTRL1bits.WIN=1;

	// Obtain the Address of CiRXMnSID register for given Mask number "mask_number"
	C1RXMnSID = (unsigned int *)(&C1RXM0SID + (mask_number << 1));

	// Bit-filed manupulation to write to Filter Mask register
	// Filter Standard Identifier
	mask10 = (mask & 0x7FF);			
	
	*C1RXMnSID=(mask10)<<5;					// Write to CiRXMnSID Register	and C1RXMnSIDbits.MIDE = 0
		
	*(C1RXMnSID+1)=0;							// Write to CiRXMnEID Register
	C1CTRL1bits.WIN=0;	

}


//void Ecan1ConfigureTxMsgBuffer(unsigned int buffer_number, long tx_id){
//	unsigned long word0=0, word1=0, word2=0;
//	unsigned long sid10_0=0;
//	unsigned int ide = 0;
//	
//	sid10_0 = (tx_id & 0x7FF);	
//	
//    word0 = ((sid10_0 << 2) | ide);
//    word2 = 0;
//			
//    // Obtain the Address of Transmit Buffer in DMA RAM for a given Transmit Buffer number
//	Ecan1msgBuf[buffer_number][0] = word0;
//	Ecan1msgBuf[buffer_number][1] = word1;
//	Ecan1msgBuf[buffer_number][2] = word2;
//}
//
//void Ecan1WriteTxMsg2Buffer(unsigned int buffer_number, unsigned int dataLength, unsigned int data10, unsigned int data32, unsigned int data54, unsigned int data76){
//	Ecan1msgBuf[buffer_number][2] = ((Ecan1msgBuf[buffer_number][2] & 0xFFF0) + dataLength);
//	
//	Ecan1msgBuf[buffer_number][3] = data10;
//	Ecan1msgBuf[buffer_number][4] = data32;
//	Ecan1msgBuf[buffer_number][5] = data54;
//	Ecan1msgBuf[buffer_number][6] = data76;
//
//	switch(buffer_number)	{
//		case 0:
//			C1TR01CONbits.TXREQ0 = 1;
//			break;
//		case 1:
//			C1TR01CONbits.TXREQ1 = 1;
//			break;
//		case 2:
//			C1TR23CONbits.TXREQ2 = 1;
//			break;
//		case 3:
//			C1TR23CONbits.TXREQ3 = 1;
//			break;
//		case 4:
//			C1TR45CONbits.TXREQ4 = 1;
//			break;
//		case 5:
//			C1TR45CONbits.TXREQ5 = 1;
//			break;
//		case 6:
//			C1TR67CONbits.TXREQ6 = 1;
//			break;
//		case 7:
//			C1TR67CONbits.TXREQ7 = 1;
//			break;			
//	}
//}
//
//void EcanTxMsg(unsigned int buffer_number, unsigned int dataLength, unsigned char* data)
//{
//	unsigned int data10;
//	unsigned int data32;
//	unsigned int data54;
//	unsigned int data76;
//
//	data10 = (unsigned int)((data[1]<<8) | data[0]);
//	data32 = (unsigned int)((data[3]<<8) | data[2]);
//	data54 = (unsigned int)((data[5]<<8) | data[4]);
//	data76 = (unsigned int)((data[7]<<8) | data[6]);
//
//	Ecan1WriteTxMsg2Buffer(buffer_number,dataLength,data10,data32, data54, data76);	
//}

void CAN_WRITE(unsigned int address, uint8_t DLC, uint8_t *data){
    static int buffer_number = 0;
    unsigned int data10;
	unsigned int data32;
	unsigned int data54;
	unsigned int data76;

	data10 = (unsigned int)((data[1]<<8) | data[0]);
	data32 = (unsigned int)((data[3]<<8) | data[2]);
	data54 = (unsigned int)((data[5]<<8) | data[4]);
	data76 = (unsigned int)((data[7]<<8) | data[6]);
    
        
	Ecan1msgBuf[buffer_number][0] = (address&0x7FF)<<2;
	Ecan1msgBuf[buffer_number][1] = 0;
    Ecan1msgBuf[buffer_number][2] = DLC;
	
	Ecan1msgBuf[buffer_number][3] = data10;
	Ecan1msgBuf[buffer_number][4] = data32;
	Ecan1msgBuf[buffer_number][5] = data54;
	Ecan1msgBuf[buffer_number][6] = data76;

	switch(buffer_number)	{
		case 0:
			C1TR01CONbits.TXREQ0 = 1;
			break;
		case 1:
			C1TR01CONbits.TXREQ1 = 1;
			break;
		case 2:
			C1TR23CONbits.TXREQ2 = 1;
			break;
		case 3:
			C1TR23CONbits.TXREQ3 = 1;
			break;
		case 4:
			C1TR45CONbits.TXREQ4 = 1;
			break;
		case 5:
			C1TR45CONbits.TXREQ5 = 1;
			break;
		case 6:
			C1TR67CONbits.TXREQ6 = 1;
			break;
		case 7:
			C1TR67CONbits.TXREQ7 = 1;
			break;			
	}
    buffer_number = (buffer_number+1)%8;
}


