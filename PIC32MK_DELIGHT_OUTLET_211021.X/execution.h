#ifndef __EXECUTION_H__
#define __EXECUTION_H__

#include "board_definition.h"

#define REQ_NAME_INFO		0x01	// Request Information of Motor Name
#define CMD_REQ_STATUS		0x02	// Request Motor Status
#define CMD_REQ_ENCODER		0x03	// Send Encoder Value			
#define CMD_REQ_CURRENT		0x04	// Send Current Value			
#define CMD_ENC_ZERO		0x06	// Set encoder zero		
#define CMD_INIT_ROM        0x08    // Init EEPROM
#define CMD_FET_ENABLE		0x0B	//				
#define CMD_RUN_CLOSEDLOOP	0x0E	//  Run feedback control loop
#define CMD_CLEAR_ERR		0x75
#define REQ_ROM_DATA        0x24
#define CMD_SET_POS_LOW_LIM 0x50
#define CMD_SET_POS_UP_LIM  0x56
#define SEND_TEMPERATURE    0xA1
#define SET_BOARD_NUM       0xF0
#define SET_CURRENT_LIM     0xF2
#define SET_ERROR_LIM       0xF3

#define CMD_CHECK_UVW               0xC1
#define CMD_DQ_ALIGN                0xC2
#define CMD_INIT_CONTROL            0xC3

#define CMD_HOME_ZERO_OFFSET        0xC6
#define CMD_ENC_RES_DIR             0xC7


#define CMD_FOC_CURRENT_CONTROL     0xD0
#define CMD_FOC_GAIN                0xD2
#define CMD_FOC_OPENLOOP_CONTROL    0xD3
#define CMD_POS_GAIN                0xD5
#define CMD_CURRENT_NULLING         0xD6

#define CMD_SINE_WAVE_TEST          0xE1


extern int ERROR_FLAG[MAX_CH];

extern int InitControlFlag[MAX_CH];
extern int InitControlState[MAX_CH];
extern int OpenPositionPulse[MAX_CH];

extern int InitHomeFlag[MAX_CH];
extern int InitHomeState[MAX_CH];
extern int InitHomeOffset[MAX_CH];
extern int HOME_DIR[MAX_CH];
extern int HOME_ZERO_OFFSET[MAX_CH];


extern int speed_test_target[MAX_CH];
extern int speed_test_acc[MAX_CH];
extern int speed_test_start_pos[MAX_CH];
extern int speed_test_cur_speed[MAX_CH];
extern int speed_test_start[MAX_CH];

extern int override_flag[MAX_CH];
extern int override_target[MAX_CH];
extern int override_speed[MAX_CH];


extern int GO_POS_CNT[MAX_CH];



extern void GateDriver_OFF();
extern void GateDriver_ON();
extern void Initialize_MotorParameter();
extern void CMD_ClearErrors();
extern void CMD_SendEncoder();
extern void CMD_SendCurrent();



extern void SafetyDataSend_Normal();
extern void SafetyDataSend_Invert();

extern void MakeEncoderInsideOneRev();


#endif