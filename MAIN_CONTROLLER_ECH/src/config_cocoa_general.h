#ifndef CONFIG_COCOA_GENERAL_H
#define CONFIG_COCOA_GENERAL_H

/*******************************************************************************
 * COMMON INCLUDE
 ******************************************************************************/
#include <proc/p32mz1024ech064.h>
#include <xc.h>
#include <sys/kmem.h>
#include <sys/attribs.h>
#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#include <math.h>


/*******************************************************************************
 * COMMON CONSTANTS
 ******************************************************************************/
#define             SYSFREQ             200000000
#define             PBCLK               100000000
#define             TMR_FREQ            1000

#define             FALSE               0
#define             TRUE                1
#define             OUTPUT              0
#define             INPUT               1
#define             LOW                 0
#define             HIGH                1
#define             DIGITAL             0
#define             ANALOG              1
#define             TURN_OFF            0
#define             TURN_ON             1

typedef double      REAL8;
static double       PI      = 3.141592653589793;
static double       D2R     = 0.017453292519943;
static double       R2D     = 57.295779513082323;

/*******************************************************************************
 * BOARD PIN SETTING
 ******************************************************************************/
#define             ANSEL_ALL               ANSELB = ANSELE = ANSELG
#define             TRIS_ALL                TRISB = TRISC = TRISD = TRISE = TRISF = TRISG
// Primary Osc
#define             TRIS_POSC               TRISCbits.TRISC12


/*******************************************************************************
 * HEADER INCLUDE
 ******************************************************************************/
#include "function_initialization.h"
#include "function_utilities.h"


#define MAX_CUP         4
#define MAX_ICE         2
#define MAX_SODA        1
#define MAX_HOT         1
#define MAX_ICECREAM    1
#define MAX_SYRUP       1
#define MAX_BARCODE     1
#define MAX_OUTLET      2
#define MAX_MILK        1
#define MAX_PLATFORM    1


typedef struct{
    unsigned char connection_status;
    unsigned char ultra_sonic_data;     // cm unit
    unsigned char status;               // (LIMIT_ROTATE<<3) | (LIMIT_EMPTY<<2) | (ULTRA_SONIC_ERROR<<1) | CUP_DETECT
    unsigned char cup_out_count;
    unsigned char last_error;           // 0: no error  other: error
    unsigned char dispense_op_flag;     // 0: idle  other: working
}CUP_DISPENSER_INFO;

typedef struct{
    unsigned char connection_status;
    unsigned char data_connection;
    unsigned char comm_mode;
    unsigned char cup_ignore;
    unsigned char setting_ambient_temp_high;
    unsigned char setting_ambient_temp_low;
    unsigned char ambient_temp;
    unsigned char evaporator_temp;
    unsigned char condensor_temp;
    unsigned char timeout_sec;
    unsigned char last_ice_out_time;
    unsigned char last_water_out_time;
    unsigned char status1;
    unsigned char status2;
    unsigned char ice_out_count;
}ICE_DISPENSER_INFO;


typedef struct{
    short   value;      // gram
    short   calib_param;
    int     raw;
    unsigned char   error;
}ST_LOADCELL_DATA;


typedef struct{
    unsigned char connection_status;
    ST_LOADCELL_DATA    loadcell[2];
    unsigned char out_state;
    unsigned char out_count;
    unsigned char last_error;
}SODA_DISPENSER_INFO;

typedef struct{
    unsigned char connection_status;
    ST_LOADCELL_DATA    loadcell[2];
    unsigned char out_state;
    unsigned char out_count;
    unsigned char last_error;
}HOT_DISPENSER_INFO;

typedef struct{
    unsigned char connection_status;
    ST_LOADCELL_DATA    loadcell[2];
    unsigned char out_state;
    unsigned char out_count;
    unsigned char last_error;
}ICECREAM_DISPENSER_INFO;

typedef struct{
    unsigned char connection_status;
    ST_LOADCELL_DATA    loadcell[2];
    unsigned char out_state;
    unsigned char out_count;
    unsigned char last_error;
}SYRUP_DISPENSER_INFO;


#define MAX_BARCODE_DATA    24
typedef struct{
    unsigned char connection_status;
    unsigned char barcode_data[MAX_BARCODE_DATA];
    unsigned char barcode_count;
}BARCODE_INFO;


typedef struct{
    unsigned char connection_status;
    int position[2];
    int current[2];
    unsigned char status[2];
    unsigned char init_status[2];
    unsigned char operating_status[2];    
    unsigned char sonar_sensor_data;
    unsigned char sonar_sensor_error;
    int open_position[2];
    unsigned char velocity[2];
}OUTLET_INFO;



typedef struct{
    unsigned char connection_status;
    int loadcell_raw[4];
}MILK_INFO;


typedef struct{
    unsigned char connection_status;
    unsigned char sensorA;
    unsigned char sensorB;
}PLATFORM_INFO;

#endif