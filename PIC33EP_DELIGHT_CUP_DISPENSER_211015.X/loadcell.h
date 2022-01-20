#ifndef __LOADCELL_H__
#define __LOADCELL_H__

#include <xc.h>
#include <p33EP32GP502.h>

#define LOAD_CELL_CLOCK_MODULE_1     LATBbits.LATB15
#define LOAD_CELL_DATA_MODULE_1      PORTBbits.RB14
#define LOAD_CELL_CLOCK_MODULE_2     LATBbits.LATB13
#define LOAD_CELL_DATA_MODULE_2      PORTBbits.RB12

#define MAX_MODULE  2

typedef struct _ST_LOADCELL_DATA_{
    short   value;      // gram
//    short   flowrate;   // gram/sec
//    short   threshold;  // gram
    short   calib_param;
    long    raw;
//    unsigned char   start;
    unsigned char   status;
}ST_LOADCELL_DATA;


extern ST_LOADCELL_DATA LOADCELL_INFO_1;
extern ST_LOADCELL_DATA LOADCELL_INFO_2;


extern unsigned char GAIN[MAX_MODULE];
extern long OFFSET[MAX_MODULE];
extern long SCALE[MAX_MODULE];

extern int tare_flag[MAX_MODULE];
extern int tare_num[MAX_MODULE];
extern int tare_cnt[MAX_MODULE];
extern long tare_sum[MAX_MODULE];

extern void InitLoadcell();

extern void HX711_set_gain(int module, unsigned char gain);

extern long HX711_read_module_1();
extern long HX711_read_module_2();

extern long HX711_get_single_value(int module);
extern long HX711_get_single_units(int module);

extern void HX711_tare(int module, unsigned char times);
extern void HX711_set_scale(int module, float scale);
extern long HX711_get_scale(int module);
extern void HX711_set_offset(int module, long offset);
extern long HX711_get_offset(int module);

extern uint8_t shiftIn_module_1();
extern uint8_t shiftIn_module_2();

#endif