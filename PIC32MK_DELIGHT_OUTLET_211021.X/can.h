#ifndef _CAN_DEFINITION_H    /* Guard against multiple inclusion */
#define _CAN_DEFINITION_H

#include <sys/types.h>

typedef volatile uint32_t SFR_TYPE;

#define _SFR_BIT_READ(reg,pos)       (  ( ( *((SFR_TYPE *)(reg)) )&(1<<(pos)) ) ? 1 : 0  )
#define _SFR_BIT_WRITE(reg,pos,val)  (  *((SFR_TYPE *)(reg)) = ( ( *((SFR_TYPE *)(reg)) ) & ~(1<<(pos)) ) | (0x1&(val))<<(pos)  )
#define _SFR_BIT_SET(reg,pos)        ( *((SFR_TYPE *)(reg)) |=  1<<(pos) )
#define _SFR_BIT_CLEAR(reg,pos)      ( *((SFR_TYPE *)(reg)) &= ~(1<<(pos)) )
#define _SFR_BIT_INVERT(reg,pos)     ( *((SFR_TYPE *)(reg)) ^=  1<<(pos) )

#define _SFR_FIELD_READ(reg,mask,pos)       (  ( *((SFR_TYPE *)(reg))&(mask) ) >> (pos)  )
#define _SFR_FIELD_WRITE(reg,mask,pos,val)  \
    (  *((SFR_TYPE *)(reg)) = \
        ( (*((SFR_TYPE *)(reg))) & ~(mask) ) | ( (mask)&((val)<<(pos)) )  )
#define _SFR_FIELD_SET(reg,mask,pos,val)    (  *((SFR_TYPE *)(reg)) |=   ( (mask)&((val)<<(pos)) )  )
#define _SFR_FIELD_CLEAR(reg,mask,pos,val)  (  *((SFR_TYPE *)(reg)) &=  ~( (mask)&((val)<<(pos)) )  )
#define _SFR_FIELD_INVERT(reg,mask,pos,val) (  *((SFR_TYPE *)(reg)) ^=   ( (mask)&((val)<<(pos)) )  )

#define _SFR_READ(reg)       ( *((SFR_TYPE *)(reg)) )
#define _SFR_WRITE(reg,val)  ( *((SFR_TYPE *)(reg))  =  (val) )


typedef struct{
    /* CAN Transmit Message Standard ID. This value should be between
    0x0 - 0x7FF */
    unsigned sid:11;
    unsigned :21;
}CAN_TX_MSG_SID;


typedef struct{
    /* Data Length Control. Specifies the size of the data payload section of
    the CAN packet. Valid values are 0x0 - 0x8 */
    unsigned data_length_code:4;

    /* Reserved bit. Should be always 0. */
    unsigned reserved0:1;

    /* Unimplemented bit. Should be always 0. */
    unsigned unimplemented1:3;

    /* Reserved bit. Should be always 0. */
    unsigned reserved1:1;

    /* Remote Transmit Request bit. Should be set for RTR messages, clear
    otherwise.*/
    unsigned remote_request:1;

    /* CAN Transmit and Receive Extended ID field. Valid values are in range
    0x0 - 0x3FFFF */
    unsigned eid:18;

    /* Identifier bit. If 0 means that message is SID. If 1 means
    that message is EID type.*/
    unsigned ide:1;

    /* Substitute Remote request bit. This bit should always be clear for an
    EID message. It is ignored for an SID message.*/
    unsigned sub_remote_req:1;

    /* Unimplemented bit. Should be always 0. */
    unsigned unimplemented2:2;
}CAN_MSG_EID;



typedef union{
    struct{
        /* This is SID portion of the CAN Transmit message */
        CAN_TX_MSG_SID msgSID;

        /* This is EID portion of the CAN Transmit message */
        CAN_MSG_EID msgEID;

        /* This is the data portion of the CAN Transmit message */
        unsigned char data[8];
    };

    /* This is CAN Transmit message organized as a set of 32 bit words */
    uint32_t messageWord[4];
}CAN_TX_MSG_BUFFER;


typedef	struct{
    /* SID of the Received CAN Message */
	unsigned sid:11;

    /* Filter which accepted this message */
	unsigned filter_hit:5;

    /* Time stamp of the received message. This is valid only if the
    Timestamping is enabled */
	unsigned msg_timestamp:16;
}CAN_RX_MSG_SID;



typedef union{
	struct{
        /* This is SID portion of the CAN Receive message */
		CAN_RX_MSG_SID msgSID;

        /* This is EID portion of the CAN Receive message */
		CAN_MSG_EID msgEID;

        /* This is the data payload section of the received message */
		uint8_t data[8];
	};

    /* This can be used if the message buffer is to
    be read from a Data-Only type of CAN Receive Channel */
    uint8_t dataOnlyMsgData[8];

    /* This is CAN Receive message organized as a set of 32-bit words */
    uint32_t messageWord[4];
}CAN_RX_MSG_BUFFER;



#define REFBASE_TXDF		0x40
#define ENCBASE_RXDF		0x50
#define STATBASE_RXDF		0x30
#define DATABASE_RXDF		0x20
#define GENBASE_RXDF        0x10


extern int CANID_REF1;
extern int CANID_ENC1;
extern int CANID_STAT1;
extern int CANID_DATA1;
extern int CANID_GEN1;

extern int CANID_REF2;
extern int CANID_ENC2;
extern int CANID_STAT2;
extern int CANID_DATA2;
extern int CANID_GEN2;


extern void InitCAN();
extern void InitCANID(int jmc);
   
extern char CAN_WRITE(int address, uint8_t DLC, uint8_t *message);
extern char CAN_WRITE_TMR3(int address, uint8_t DLC, uint8_t *message);

extern char CAN_READ(uint32_t *address, uint8_t *message, uint8_t *length);

extern int Push_CAN_TX_Ring(unsigned int id, unsigned int dlc, unsigned char *data);	
extern int Handler_CAN_TX_Ring();

extern int Push_CAN_TX_Ring_TMR3(unsigned int id, unsigned int dlc, unsigned char *data);	
extern int Handler_CAN_TX_Ring_TMR3();







#endif /* _CAN_DEFINITION_H */

