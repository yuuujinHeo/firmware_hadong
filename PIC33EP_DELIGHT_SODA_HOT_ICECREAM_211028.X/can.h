#ifndef __CAN_H__
#define __CAN_H__

#include <xc.h>
#include <p33EP32GP502.h>

#define     BOARD_TYPE_NONE         0
#define     BOARD_TYPE_SODA         1
#define     BOARD_TYPE_HOT          2
#define     BOARD_TYPE_ICECREAM     4


/* CAN Baud Rate Configuration 		*/
#define FCAN            40000000 
#define BITRATE         1000000  
#define NTQ 			20		// 20 Time Quanta in a Bit Time
#define BRP_VAL         ((FCAN/(2*NTQ*BITRATE))-1)

/* message structure in RAM */
typedef struct{
	/* keep track of the buffer status */
	unsigned char buffer_status;
	/* RTR message or data message */
	unsigned char message_type;
	/* frame type extended or standard */
	unsigned char frame_type;
	/* buffer being used to receive messages */
	unsigned char buffer_number;
	/* 29 bit id max of 0x1FFF FFFF 
	*  11 bit id max of 0x7FF */
	unsigned long id; 
	unsigned char data[8];	
	unsigned char data_length;
}MESSAGE;

/* filter and mask defines */
/* Macro used to write filter/mask ID to Register CiRXMxSID and 
CiRXFxSID. For example to setup the filter to accept a value of 
0x123, the macro when called as CAN_FILTERMASK2REG_SID(0x123) will 
write the register space to accept message with ID ox123 
USE FOR STANDARD MESSAGES ONLY */
#define CAN_FILTERMASK2REG_SID(x) ((x & 0x07FF)<< 5)
/* the Macro will set the "MIDE" bit in CiRXMxSID */
#define CAN_SETMIDE(sid) (sid | 0x0008)
/* the macro will set the EXIDE bit in the CiRXFxSID to 
accept extended messages only */
#define CAN_FILTERXTD(sid) (sid | 0x0008)
/* the macro will clear the EXIDE bit in the CiRXFxSID to 
accept standard messages only */
#define CAN_FILTERSTD(sid) (sid & 0xFFF7)
/* Macro used to write filter/mask ID to Register CiRXMxSID, CiRXMxEID and 
CiRXFxSID, CiRXFxEID. For example to setup the filter to accept a value of 
0x123, the macro when called as CAN_FILTERMASK2REG_SID(0x123) will 
write the register space to accept message with ID 0x123 
USE FOR EXTENDED MESSAGES ONLY */
#define CAN_FILTERMASK2REG_EID0(x) (x & 0xFFFF)
#define CAN_FILTERMASK2REG_EID1(x) (((x & 0x1FFC)<< 3)|(x & 0x3))


#define ECAN1_MSG_BUF_LENGTH 	16
typedef unsigned int ECAN1MSGBUF[ECAN1_MSG_BUF_LENGTH][8];



extern void InitCAN();
extern void InitDMA();

extern void rxECAN(MESSAGE *message);

extern void CAN_WRITE(unsigned int address, uint8_t DLC, uint8_t *data);

#endif