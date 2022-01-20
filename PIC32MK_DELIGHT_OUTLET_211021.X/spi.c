//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>

#include <string.h>

#include "spi.h"
#include "execution.h"

// EEPROM Instruction
#define		WRITE_ENABLE		0x06
#define		WRITE_DISABLE		0x04
#define		READ_STAT_REG		0x05
#define		WRITE_STAT_REG		0x01
#define		READ_MEMORY         0x03
#define		WRITE_MEMORY		0x02

// EEPROM Status Register Format
#define		SRWD				0x80	// Status Register Write Disable bit
#define		BP1					0x08	// Block Protect bit
#define		BP0					0x04	// Block Protect bit
#define		WEL					0x02	// Write Enable Latch bit
#define		WIP					0x01	// Write In Progress bit

#define     EEPROM_BUFF_SIZE    30

typedef struct{
    unsigned int head;
    unsigned int tail;
	unsigned int ch[EEPROM_BUFF_SIZE];
	unsigned int type[EEPROM_BUFF_SIZE];
    unsigned int data[EEPROM_BUFF_SIZE];
} EEPROM_RING;
static EEPROM_RING _eeprom_ring;


unsigned int RomData[MAX_CH+1][EE_TYPE_MAX];


static unsigned int _EEPROM_ReadStatus();
unsigned int _SPI_WRITE(unsigned int data);

static void _Init_EEPROM_Ring();


extern int JMC;


void InitSPI(unsigned int data_bit, unsigned int mode){
    int dummy;
    char bit_mode[2];
    char mode_select[2];
    
    _Init_EEPROM_Ring();
    
    
    RPB1Rbits.RPB1R = 0b00011;  // RB1 SDO1     -->     see TABLE 13-2 OUTPUT PIN SELECTION
    SDI1Rbits.SDI1R = 0b0001;   // RB3 SDI1     -->     see TABLE 13-1 INPUT PIN SELECTION
    
    // bit : SPI data length, 8 or 16 or 32
    if(data_bit == 8){
        bit_mode[0] = 0;
        bit_mode[1] = 0;
    }else if(data_bit == 16){
        bit_mode[0] = 1;
        bit_mode[1] = 0;
    }else if(data_bit == 32){
        bit_mode[0] = 0;
        bit_mode[1] = 1;
    }else{
        bit_mode[0] = 1;
        bit_mode[1] = 0;
    }
    
    // mode : SPI operation mode
    // 0 = idle low & rising edge
    // 1 = idle low & falling edge
    // 2 = idle high & falling edge
    // 3 = idle high & rising edge
    if(mode == 0){
        mode_select[0] = 0;
        mode_select[1] = 0;
    }else if(mode == 1){
        mode_select[0] = 0;
        mode_select[1] = 1;
    }else if(mode == 2){
        mode_select[0] = 1;
        mode_select[1] = 0;
    }else if(mode == 3){
        mode_select[0] = 1;
        mode_select[1] = 1;
    }
    
    // Register setting
    //SPI1BRG = 299;                  // Fsck = Fpb/(2*(SPIxBRG+1)), 100k = 60M/(2*300)
    SPI1BRG = 49;                  // Fsck = Fpb/(2*(SPIxBRG+1)), 600k = 60M/(2*50)
    SPI1STATCLR=0x40;               // clear the Overflow
    SPI1CON = 0x0000;               // clear the register
    dummy = SPI1BUF;                // clear the buffer by using dummy
    SPI1CONbits.MSTEN = 1;          // 1 = master mode / 0 = slave mode
    SPI1CONbits.MODE16 = bit_mode[0];
    SPI1CONbits.MODE32 = bit_mode[1];
    SPI1CONbits.SMP = 1;            // 1 = input data sampled at "end" of data output time/ 0 = middle
    SPI1CONbits.CKP = mode_select[0];       // 1: high idle      0: low idle
    SPI1CONbits.CKE = mode_select[1];       // 1: active->idle   0: idle->active
    SPI1CONbits.ON = 1;             // SPI Start
}

unsigned int _SPI_WRITE(unsigned int data){
	unsigned int ret;
	
 	ret = SPI1BUF;                 // dummy read of the SPI1BUF register to clear the SPIRBF flag
	SPI1BUF = data;                 // write the data out to the SPI peripheral
	
	while(!SPI1STATbits.SPIRBF);	// wait for the data to be sent out
	
	ret = SPI1BUF;	
	return ret;
}



static unsigned int _EEPROM_ReadStatus(){
	unsigned int ret;
	EEPROM_SELECT
	ret = _SPI_WRITE(READ_STAT_REG);
	ret = _SPI_WRITE(0x00);
	EEPROM_DESELECT
	return ret;
}

void EEPROM_WRITE(unsigned int ch, unsigned int type, unsigned int data){
	unsigned int temp, addr;
    unsigned int a1, a2, b1, b2, b3, b4;
    addr = ((ch*EE_TYPE_MAX + type)<<2) & 0x3FFF;
    	
    a1 = (addr>>8)&0xFF;
	a2 = addr&0xFF;
	b1 = (data>>24)&0xFF;
    b2 = (data>>16)&0xFF;
    b3 = (data>>8)&0xFF;
	b4 = data&0xFF;
   
    do{
  		EEPROM_SELECT
		_SPI_WRITE(WRITE_ENABLE);
		EEPROM_DESELECT
		temp = _EEPROM_ReadStatus();
		temp = temp & (WIP | WEL);
	}while(temp != WEL);//temp != WEL
 
	EEPROM_SELECT
	_SPI_WRITE(WRITE_MEMORY);
	temp = _SPI_WRITE(a1);
	temp = _SPI_WRITE(a2);
	temp = _SPI_WRITE(b1);
	temp = _SPI_WRITE(b2);
    temp = _SPI_WRITE(b3);
    temp = _SPI_WRITE(b4);
	EEPROM_DESELECT
}

unsigned int EEPROM_READ(unsigned int ch, unsigned int type){
	unsigned int temp, addr;
    unsigned int a1, a2, b1, b2, b3, b4;

    addr = ((ch*EE_TYPE_MAX + type)<<2) & 0x3FFF;

	a1 = (addr>>8)&0xFF;
	a2 = addr&0xFF;

	EEPROM_SELECT
	temp = _SPI_WRITE(READ_MEMORY);// Read Commend 
	temp = _SPI_WRITE(a1);		// MSB address
	temp = _SPI_WRITE(a2);		// LSB address
	b1 = _SPI_WRITE(0x00);		// Dummy data for clock
	b2 = _SPI_WRITE(0x00);		// Dummy data for clock
    b3 = _SPI_WRITE(0x00);		// Dummy data for clock
    b4 = _SPI_WRITE(0x00);		// Dummy data for clock
	EEPROM_DESELECT
	return (b1<<24) | (b2<<16) | (b3<<8) | b4;
}




//
//void write_romL(unsigned int addr1,unsigned int addr2,  unsigned long data)
//{
//unsigned int L1,L2;
//
////	addr = addr1*MAX_DATA + addr2; 
//
//	L1 =(unsigned int)((data>>16)&0xFFFF);
//	L2 =(unsigned int)(data & 0x0000FFFF);
//	write_rom(addr1,addr2,L2);
//	write_rom(addr1,addr2+1,L1);
//}
//void write_romf(unsigned int addr1,unsigned int addr2,  float val)
//{
//	unsigned int c1,c2;
//
////	addr = addr1*MAX_DATA + addr2; 
//
//	fl2l.f=val;
//	c1=(unsigned int)(fl2l.l>>16);
//	c2=(unsigned int)(fl2l.l);
//	write_rom(addr1,addr2,c2);
//	write_rom(addr1,addr2+1,c1);
//
//}
//float read_romf(unsigned int addr1,unsigned int addr2)
//{
//unsigned long c1,c2;
//
//	c1=((unsigned long)read_rom(addr1, addr2)) << 16;
//	c2=((unsigned long)read_rom(addr1, addr2+1))&0x0000FFFF;
//	fl2l.l=c2 + c1;
//	return fl2l.f;
//}
//
//long read_romL(unsigned int addr1,unsigned int addr2)
//{
//long LL;
//
//	LL=(long)read_rom(addr1, addr2+1);
//	LL<<=16;
//	LL+=read_rom(addr1, addr2);
//	return LL;
//}




int Push_EEPROM_Ring(unsigned int ch, unsigned int type, unsigned int data){
	unsigned int new_head = _eeprom_ring.head = (_eeprom_ring.head+1)%EEPROM_BUFF_SIZE;

	if(new_head == _eeprom_ring.tail){
		if(new_head == 0)
			_eeprom_ring.head = EEPROM_BUFF_SIZE-1;
		else
			_eeprom_ring.head -= 1;
		return -1;
	}
	_eeprom_ring.ch[new_head] = ch;
	_eeprom_ring.type[new_head] = type;
    _eeprom_ring.data[new_head] = data;
		
	return 0;
}

int Handler_EEPROM_Ring(){
	unsigned int new_tail;
	if(_eeprom_ring.head != _eeprom_ring.tail){
		new_tail = (_eeprom_ring.tail+1)%EEPROM_BUFF_SIZE;

        EEPROM_WRITE(_eeprom_ring.ch[new_tail], _eeprom_ring.type[new_tail], _eeprom_ring.data[new_tail]);
        _eeprom_ring.tail = new_tail;
		return 1;
	}
	return 0;
}

static void _Init_EEPROM_Ring(){
	memset(&_eeprom_ring, 0, sizeof(EEPROM_RING));
}

