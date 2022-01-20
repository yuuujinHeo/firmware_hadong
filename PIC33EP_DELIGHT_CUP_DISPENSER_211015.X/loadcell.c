#include "loadcell.h"




#define MSBFIRST 0
#define LSBFIRST 1

#define SHIFT_CNT   50        // 1000-->10usec  100-->1usec


unsigned char GAIN[MAX_MODULE] = {1, 1};		// amplification factor
long OFFSET[MAX_MODULE] = {0, 0};	// used for tare weight
long SCALE[MAX_MODULE] = {1, 1};

int tare_flag[MAX_MODULE] = {0, 0};
int tare_num[MAX_MODULE] = {0, 0};
int tare_cnt[MAX_MODULE] = {0, 0};
long tare_sum[MAX_MODULE] = {0, 0};


ST_LOADCELL_DATA LOADCELL_INFO_1;
ST_LOADCELL_DATA LOADCELL_INFO_2;

void InitLoadcell(){
    TRISBbits.TRISB15 = 0;
    TRISBbits.TRISB14 = 1;
    TRISBbits.TRISB13 = 0;
    TRISBbits.TRISB12 = 1;
    
    CNENBbits.CNIEB14 = 1;  // Data A
    CNENBbits.CNIEB12 = 1;  // Data B
    
    IPC4bits.CNIP = 7; // CN interrupt priority
    IFS1bits.CNIF = 0; // Reset CN interrupt
    IEC1bits.CNIE = 1; // Enable CN interrupts
    
    
    HX711_set_gain(0, 128);
    HX711_set_gain(1, 128);
    
    HX711_set_scale(0, 205);
    HX711_set_scale(1, 205);
        
    LOADCELL_INFO_1.calib_param = 205;
    LOADCELL_INFO_2.calib_param = 205;
    
    HX711_tare(0, 5);
    HX711_tare(1, 5);
}



void HX711_set_gain(int module, unsigned char gain) {
	switch (gain) {
		case 128:		// channel A, gain factor 128
			GAIN[module] = 1;
			break;
		case 64:		// channel A, gain factor 64
			GAIN[module] = 3;
			break;
		case 32:		// channel B, gain factor 32
			GAIN[module] = 2;
			break;
	}
}

long HX711_read_module_1() {
    unsigned int i, k=0;
	// Wait for the chip to become ready.
        
	// Define structures for reading data into.
	unsigned long value = 0;
	uint8_t data[3] = { 0 };
	uint8_t filler = 0x00;
	
	// Pulse the clock pin 24 times to read the data.
	data[2] = shiftIn_module_1();
	data[1] = shiftIn_module_1();
	data[0] = shiftIn_module_1();

	// Set the channel and the gain factor for the next reading using the clock pin.
	for (i = 0; i < GAIN[0]; i++) {
        LOAD_CELL_CLOCK_MODULE_1 = 1;
        for(k=0; k<SHIFT_CNT; k++)
            ;
        LOAD_CELL_CLOCK_MODULE_1 = 0;
        for(k=0; k<SHIFT_CNT; k++)
            ;
	}

	// Replicate the most significant bit to pad out a 32-bit signed integer
	if (data[2] & 0x80) {
		filler = 0xFF;
	} else {
		filler = 0x00;
	}

	// Construct a 32-bit signed integer
	value = ( (unsigned long)(filler) << 24
			| (unsigned long)(data[2]) << 16
			| (unsigned long)(data[1]) << 8
			| (unsigned long)(data[0]) );

	return (unsigned long)(value);
}
long HX711_read_module_2() {
    unsigned int i, k=0;
	// Wait for the chip to become ready.
          
	// Define structures for reading data into.
	unsigned long value = 0;
	uint8_t data[3] = { 0 };
	uint8_t filler = 0x00;
	
	// Pulse the clock pin 24 times to read the data.
	data[2] = shiftIn_module_2();
	data[1] = shiftIn_module_2();
	data[0] = shiftIn_module_2();

	// Set the channel and the gain factor for the next reading using the clock pin.
	for (i = 0; i < GAIN[1]; i++) {
        LOAD_CELL_CLOCK_MODULE_2 = 1;
        for(k=0; k<SHIFT_CNT; k++)
            ;
        LOAD_CELL_CLOCK_MODULE_2 = 0;
        for(k=0; k<SHIFT_CNT; k++)
            ;
	}

	// Replicate the most significant bit to pad out a 32-bit signed integer
	if (data[2] & 0x80) {
		filler = 0xFF;
	} else {
		filler = 0x00;
	}

	// Construct a 32-bit signed integer
	value = ( (unsigned long)(filler) << 24
			| (unsigned long)(data[2]) << 16
			| (unsigned long)(data[1]) << 8
			| (unsigned long)(data[0]) );

	return (unsigned long)(value);
}


long HX711_get_single_value(int module) {
    if(module == 0){
        return HX711_read_module_1() - OFFSET[module];
    }else if(module == 1){
        return HX711_read_module_2() - OFFSET[module];
    }
    return 0;
}

long HX711_get_single_units(int module){
    return HX711_get_single_value(module) / SCALE[module];
}

void HX711_tare(int module, unsigned char times) {
    tare_num[module] = times;
    tare_cnt[module] = 0;
    tare_sum[module] = 0;    
    tare_flag[module] = 1;
}

void HX711_set_scale(int module, float scale) {
	SCALE[module] = scale;
}

long HX711_get_scale(int module) {
	return SCALE[module];
}

void HX711_set_offset(int module, long offset) {
	OFFSET[module] = offset;
}

long HX711_get_offset(int module) {
	return OFFSET[module];
}


uint8_t shiftIn_module_1() {
    uint8_t value = 0;
    uint8_t i;
    unsigned int dd;
    for (i = 0; i < 8; ++i) {
        LOAD_CELL_CLOCK_MODULE_1 = 1;
        for(dd = 0; dd < SHIFT_CNT; dd++)
            ;
        value |= (LOAD_CELL_DATA_MODULE_1) << (7 - i);
//        for(dd = 0; dd < 100; dd++)
//            ;
        LOAD_CELL_CLOCK_MODULE_1 = 0;
        for(dd = 0; dd < SHIFT_CNT; dd++)
            ;
    }
    return value;  
}
uint8_t shiftIn_module_2() {
    uint8_t value = 0;
    uint8_t i;
    unsigned int dd;
    for (i = 0; i < 8; ++i) {
        LOAD_CELL_CLOCK_MODULE_2 = 1;
        for(dd = 0; dd < SHIFT_CNT; dd++)
            ;
        value |= (LOAD_CELL_DATA_MODULE_2) << (7 - i);
//        for(dd = 0; dd < 100; dd++)
//            ;
        LOAD_CELL_CLOCK_MODULE_2 = 0;
        for(dd = 0; dd < SHIFT_CNT; dd++)
            ;
    }
    return value;  
}