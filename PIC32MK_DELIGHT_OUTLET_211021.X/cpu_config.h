#ifndef __CPU_CONFIG_H__
#define __CPU_CONFIG_H__

//#define INTERNAL_FRC



/*** DEVCFG0 ***/
#pragma config DEBUG =      OFF
#pragma config JTAGEN =     OFF
#pragma config ICESEL =     ICS_PGx2
#pragma config TRCEN =      OFF
#pragma config BOOTISA =    MIPS32
#pragma config FSLEEP =     VREGS//OFF
#pragma config DBGPER =     PG_ALL
#pragma config SMCLR =      MCLR_NORM
//#pragma config SOSCGAIN =   GAIN_2X
//#pragma config SOSCBOOST =  ON
#pragma config POSCGAIN =   GAIN_LEVEL_3
#pragma config POSCBOOST =  ON
#pragma config EJTAGBEN =   NORMAL
#pragma config CP =         OFF

/*** DEVCFG1 ***/
#pragma config FNOSC =      SPLL                // Oscillator Selection Bits
#ifdef INTERNAL_FRC
#pragma config POSCMOD =    OFF//EC             // Primary Oscillator Configuration
#else
#pragma config POSCMOD =    EC             // Primary Oscillator Configuration
#endif
#pragma config FSOSCEN =    OFF                 // Secondary Oscillator Enable
#pragma config IESO =       ON
#pragma config OSCIOFNC =   ON
#pragma config FCKSM =      CSECME
#pragma config FWDTEN =     OFF                 // Watchdog Timer Enable
//#pragma config WDTPS =      PS1048576
//#pragma config WDTSPGM =    STOP
//#pragma config WINDIS =     NORMAL
#pragma config FDMTEN =     OFF                 // Deadman Timer Enable
//#pragma config DMTINTV =    WIN_127_128
//#pragma config DMTCNT =     DMT31
//#pragma config FWDTWINSZ =  WINSZ_25

/*** DEVCFG2 ***/


#ifdef INTERNAL_FRC
// FRC
#pragma config FPLLICLK =   PLL_FRC//PLL_POSC
#pragma config FPLLIDIV =   DIV_1
#pragma config FPLLRNG =    RANGE_5_10_MHZ
#pragma config FPLLMULT =   MUL_60
#pragma config FPLLODIV =   DIV_4
#else
// POSC
#pragma config FPLLICLK =   PLL_POSC
#pragma config FPLLIDIV =   DIV_1
#pragma config FPLLRNG =    RANGE_13_26_MHZ
#pragma config FPLLMULT =   MUL_20
#pragma config FPLLODIV =   DIV_4
#endif

#pragma config VBATBOREN =  OFF                 // VBAT BOR Enable
#pragma config DSBOREN =    OFF                 // Deep Sleep BOR Enable
#pragma config DSWDTEN =    OFF                 // Deep Sleep Watchdog Timer Enable
#pragma config FDSEN =      OFF                 // Deep Sleep Enable
//#pragma config DSWDTPS =    DSPS32
//#pragma config DSWDTOSC =   LPRC
#pragma config BORSEL =     HIGH
#pragma config UPLLEN =     OFF

/*** DEVCFG3 ***/
#pragma config USERID =     0xffff
#pragma config FUSBIDIO1 =  OFF
#pragma config FVBUSIO1 =   OFF
#pragma config FUSBIDIO2 =  OFF
#pragma config FVBUSIO2 =   OFF

#pragma config PGL1WAY =    OFF
#pragma config PMDL1WAY =   OFF
#pragma config IOL1WAY =    OFF

#pragma config PWMLOCK =    OFF

/*** BF1SEQ0 ***/

#pragma config TSEQ =       0x0000
#pragma config CSEQ =       0xffff




//#pragma config DEBUG =      OFF
//#pragma config JTAGEN =     OFF
//#pragma config ICESEL =     ICS_PGx2
//#pragma config TRCEN =      OFF
//#pragma config BOOTISA =    MIPS32
//#pragma config FSLEEP =     OFF
//#pragma config DBGPER =     PG_ALL
//#pragma config SMCLR =      MCLR_NORM
//#pragma config SOSCGAIN =   GAIN_2X
//#pragma config SOSCBOOST =  ON
//#pragma config POSCGAIN =   GAIN_LEVEL_3
//#pragma config POSCBOOST =  ON
//#pragma config EJTAGBEN =   NORMAL
//#pragma config CP =         OFF
//
///*** DEVCFG1 ***/
//
//#pragma config FNOSC =      SPLL
//#pragma config DMTINTV =    WIN_127_128
//#pragma config FSOSCEN =    ON
//#pragma config IESO =       ON
//#pragma config POSCMOD =    EC
//#pragma config OSCIOFNC =   OFF
//#pragma config FCKSM =      CSECME
//#pragma config WDTPS =      PS1048576
//#pragma config WDTSPGM =    STOP
//#pragma config FWDTEN =     OFF
//#pragma config WINDIS =     NORMAL
//#pragma config FWDTWINSZ =  WINSZ_25
//#pragma config DMTCNT =     DMT31
//#pragma config FDMTEN =     OFF
///*** DEVCFG2 ***/
//
//#pragma config FPLLIDIV =   DIV_1
//#pragma config FPLLRNG =    RANGE_5_10_MHZ
//#pragma config FPLLICLK =   PLL_FRC
//#pragma config FPLLMULT =   MUL_60
//#pragma config FPLLODIV =   DIV_4
//#pragma config VBATBOREN =  ON
//#pragma config DSBOREN =    ON
//#pragma config DSWDTPS =    DSPS32
//#pragma config DSWDTOSC =   LPRC
//#pragma config DSWDTEN =    OFF
//#pragma config FDSEN =      ON
//#pragma config BORSEL =     HIGH
//#pragma config UPLLEN =     OFF
///*** DEVCFG3 ***/
//
//#pragma config USERID =     0xffff
//#pragma config FUSBIDIO2 =   ON
//#pragma config FVBUSIO2 =  ON
//#pragma config PGL1WAY =    ON
//#pragma config PMDL1WAY =   ON
//#pragma config IOL1WAY =    ON
//#pragma config FUSBIDIO1 =   ON
//#pragma config FVBUSIO1 =  ON
//#pragma config PWMLOCK =  OFF

/*** BF1SEQ0 ***/

//#pragma config TSEQ =       0x0000
//#pragma config CSEQ =       0xffff



#define SYS_CLK_FREQ                        120000000ul
#define SYS_CLK_BUS_PERIPHERAL_1            60000000ul
#define SYS_CLK_BUS_PERIPHERAL_2            60000000ul
#define SYS_CLK_BUS_PERIPHERAL_3            60000000ul
#define SYS_CLK_BUS_PERIPHERAL_4            60000000ul
#define SYS_CLK_BUS_PERIPHERAL_5            120000000ul
#define SYS_CLK_BUS_PERIPHERAL_6            30000000ul      //DSCTRL, RTCC
#define SYS_CLK_BUS_PERIPHERAL_7            60000000ul



#endif