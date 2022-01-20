#ifndef __STATUS_H__
#define __STATUS_H__

#include "board_definition.h"

// Status Error Definition
#define STAT_ERR_OK         0x00
#define STAT_ERR_JAM        0x01
#define STAT_ERR_INP        0x02
#define STAT_ERR_PWM        0x03
#define STAT_ERR_BIG        0x04
#define STAT_ERR_FLG        0x05
#define STAT_ERR_CUR        0x06
#define STAT_ERR_ENC        0x07
#define STAT_ERR_TMP        0x08



#define SET_STATUS_FLAG1    STATUS_FLAG[0] = 1;
#define SET_STATUS_FLAG2    STATUS_FLAG[1] = 1;

#define INIT_BOARD_MODE_OK              0
#define INIT_BOARD_MODE_BNO_CHANGED     1
#define INIT_BOARD_MODE_RESET_BTN       2
#define INIT_BOARD_MODE_RESTORE         3

extern int      BOARD_TYPE;
extern int      JMC;    // board id
extern int      INIT_BOARD_MODE;
extern int      NOC;    // number of channel
extern int      NOM;    // number of motor
extern char     BoardName[13];

extern mSTAT    STAT[MAX_CH];
extern int      STATUS_FLAG[MAX_CH];

// error --------------------
extern int T_ERR;
extern long I_ERR[MAX_CH], B_ERR[MAX_CH];
extern unsigned int JAM_cnt[MAX_CH], OVR_cnt[MAX_CH], CUR_BIG_cnt[MAX_CH];
extern unsigned int JAM_LIM_Current[MAX_CH], OVR_LIM_Current[MAX_CH], JAM_LIM_Time[MAX_CH], OVR_LIM_Time[MAX_CH], CUR_BIG_LIM[MAX_CH], CUR_BIG_Time[MAX_CH];





extern void Handler_Status(unsigned long cnt);
extern void Handler_Error(unsigned long cnt);



#endif