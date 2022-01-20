#ifndef __SPI_H__
#define __SPI_H__

#include "board_definition.h"

// EEPROM Chip Select
#define     EEPROM_SELECT       LATEbits.LATE12 = 0;
#define     EEPROM_DESELECT     LATEbits.LATE12 = 1;



extern void InitSPI(unsigned int data_bit, unsigned int mode);

extern void EEPROM_WRITE(unsigned int ch, unsigned int type, unsigned int data);
extern unsigned int EEPROM_READ(unsigned int ch, unsigned int type);


extern int Push_EEPROM_Ring(unsigned int ch, unsigned int type, unsigned int data);
extern int Handler_EEPROM_Ring();



// ========================================
// ========================================
// ========================================


extern void InitSPI3(unsigned int data_bit, unsigned int mode);
extern unsigned int _SPI_WRITE3(unsigned int data);

    

#define EE_TYPE_MAX                 100

#define EE_COMMON_JMC               0x00
#define EE_COMMON_BOARDTYPE         0x01
#define EE_COMMON_JMC_NOT           0x02
#define EE_COMMON_BOOTLOADER        0x03
#define EE_TYPE_ENC_RES             0x06

#define EE_COMMON_TEMP_ERROR        0x10


#define EE_TYPE_MOTOR_DIR           0x08
#define EE_TYPE_HOME_DIR            0x09


#define EE_TYPE_JAM_LIMIT_CURRENT   0x11
#define EE_TYPE_OVR_LIMIT_CURRENT   0x12
#define EE_TYPE_JAM_LIMIT_TIME      0x13
#define EE_TYPE_OVR_LIMIT_TIME      0x14
#define EE_TYPE_INPUT_ERROR         0x15
#define EE_TYPE_BIG_ERROR           0x16
#define EE_TYPE_POS_LOWER_LIMIT     0x17
#define EE_TYPE_POS_UPPER_LIMIT     0x18


#define EE_TYPE_FOC_PGAIN           0x24
#define EE_TYPE_FOC_IGAIN           0x25
#define EE_TYPE_POS_PGAIN           0x26
#define EE_TYPE_POS_IGAIN           0x27
#define EE_TYPE_POS_DGAIN           0x28


#define EE_TYPE_HOME_ZERO_OFFSET        0x33


#define EE_TYPE_CURRENT_NULL_RZ     0x60


extern unsigned int RomData[MAX_CH+1][EE_TYPE_MAX];



#endif