//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>

#include <sys/attribs.h>
#include <math.h>
#include <string.h>
#include <xc-pic32m.h>


#include "execution.h"
#include "status.h"
#include "can.h"
#include "spi.h"
#include "adc.h"
#include "qei.h"
#include "control.h"
#include "timer.h"
#include "FOC.h"
#include "pwm.h"
#include "home.h"
#include "init.h"



// extern 
int ERROR_FLAG[MAX_CH] = {0,};

int InitControlFlag[MAX_CH] = {0,};
int InitControlState[MAX_CH] = {0,};
int OpenPositionPulse[MAX_CH] = {-115000, -115000};

int InitHomeFlag[MAX_CH] = {0,};
int InitHomeState[MAX_CH] = {0,};
int InitHomeOffset[MAX_CH] = {0,};
int HOME_DIR[MAX_CH] = {1,};
int HOME_ZERO_OFFSET[MAX_CH] = {0,};


int speed_test_target[MAX_CH] = {0,};
int speed_test_acc[MAX_CH] = {0,};
int speed_test_start_pos[MAX_CH] = {0,};
int speed_test_cur_speed[MAX_CH] = {0,};
int speed_test_start[MAX_CH] = {0,};


int override_flag[MAX_CH] = {0,};
int override_target[MAX_CH] = {0,};
int override_speed[MAX_CH] = {0,};


int GO_POS_CNT[MAX_CH] = {0,};



void GateDriver_OFF(int ch);
void GateDriver_ON(int ch);
void Initialize_MotorParameter(int ch);
void CMD_EncZero(int ch, uint8_t data[], uint8_t dlc);
void CMD_InitROM(int ch, uint8_t data[], uint8_t dlc);
void CMD_ClearErrors(int ch);
void CMD_FetEnable(int ch, int onoff);
void CMD_RunClosedLoop(int ch, int onoff, int mode);
void CMD_SendEncoder(int ch);



static void ExecuteGeneralCMD(int ch, uint8_t data[], uint8_t dlc);
static void ExecuteReferenceCMD(int ch, uint8_t data[], uint8_t dlc);

static void Send_Temperature(int ch);
static void Send_MotorName(int ch);
static void CMD_SetPosLowLimit(int ch, uint8_t data[], uint8_t dlc);
static void CMD_SetPosUpLimit(int ch, uint8_t data[], uint8_t dlc);
static void Set_Curr_lim(int ch, uint8_t data[], uint8_t dlc);
static void Set_Err_lim(int ch, uint8_t data[], uint8_t dlc);
static void Send_data1(int ch, uint8_t data[], uint8_t dlc);


static void CMD_DQ_Align(int ch, uint8_t data[], uint8_t dlc);
static void CMD_InitControl(int ch, uint8_t data[], uint8_t dlc);


static void CMD_FOC_Current_Control(int ch, uint8_t data[], uint8_t dlc);
static void CMD_FOC_Gain(int ch, uint8_t data[], uint8_t dlc);
static void CMD_FOC_OpenLoop_Control(int ch, uint8_t data[], uint8_t dlc);
static void CMD_POS_Gain(int ch, uint8_t data[], uint8_t dlc);
static void CMD_CurrentNulling(int ch, uint8_t data[], uint8_t dlc);
static void CMD_SineWaveTest(int ch, uint8_t data[], uint8_t dlc);
static void CMD_HomeZeroOffset(int ch, uint8_t data[], uint8_t dlc);
static void CMD_EncResDir(int ch, uint8_t data[], uint8_t dlc);


static unsigned char t_mode_flag_old[MAX_CH] = {0,};


static void ExecuteReferenceCMD(int ch, uint8_t data[], uint8_t dlc) {
    int itemp;
    short stemp;


    itemp = data[0] | (data[1] << 8) | (data[2] << 16);
    stemp = data[3] | (data[4] << 8);
    //    ENC_SHOT_CNT = (char)data[5];//(unsigned int)data[5]&0x0F;
//    ENC_SHOT_CNT = (char) (data[5] & 0b10111111);
    unsigned char t_mode_flag_cur = data[5]&0b01000000;
    
   
    
    if (t_mode_flag_cur == 64 && t_mode_flag_old[ch] == 0) {
        // goto current mode
        if (CONTROL_MODE[ch] != CONTROL_MODE_FOC_CUR_CONTROL) {
            STAT[ch].b.RUN = 0;
            STAT[ch].b.MOD = CTRL_MOD_CURRENT;
            FOC_Init_CurrentControl(ch);
            CONTROL_MODE[ch] = CONTROL_MODE_FOC_CUR_CONTROL;
        }
    } else if (t_mode_flag_cur == 0 && t_mode_flag_old[ch] == 64) {
        // goto position mode
        CMD_RunClosedLoop(ch, 1, 1);
    }
    t_mode_flag_old[ch] = t_mode_flag_cur;



    if (itemp & 0x800000) {
        itemp |= (0xFF000000);
    }

#ifdef BOARD_TYPE_32
    NewInputCurrent[ch] = stemp*10;
#else
    NewInputCurrent[ch] = stemp;
#endif


    if (CONTROL_MODE[ch] == CONTROL_MODE_FOC_POS_CONTROL) {
        NewInputRef[ch] = itemp*ENC_MUL;
        GO_POS_CNT[ch] = 0;
        Handler_Control(ch);        
    }else if (CONTROL_MODE[ch] == CONTROL_MODE_FOC_CUR_CONTROL){
        ;
    }
}

void __ISR(_CAN1_VECTOR, IPL4AUTO) Handler_CAN_1() {
    static uint8_t message[8];
    static uint32_t can_id;
    static uint8_t dlc;

    while (CAN_READ(&can_id, message, &dlc) == 1) {
        if(can_id == CANID_REF1) {
            ExecuteReferenceCMD(0, message, dlc);
        }else if(can_id == CANID_REF2){
            ExecuteReferenceCMD(1, message, dlc);
        }else if(can_id == CANID_GEN1) {
            ExecuteGeneralCMD(0, message, dlc);
        }else if(can_id == CANID_GEN2) {
            ExecuteGeneralCMD(1, message, dlc);
        }
    }

    IFS5bits.CAN1IF = 0;
}

static void ExecuteGeneralCMD(int ch, uint8_t data[], uint8_t dlc) {
    int itemp;
    int itemp2;

    switch (data[0]) {

        case REQ_NAME_INFO:
            Send_MotorName(ch);
            break;

        case CMD_REQ_STATUS: // Request motor status
            if(ch == 0){
                SET_STATUS_FLAG1
            }else{
                SET_STATUS_FLAG2
            }
            break;

        case CMD_REQ_ENCODER: // Request Encoder value
            CMD_SendEncoder(ch);
            break;

        case CMD_REQ_CURRENT: // Request Current value
            CMD_SendCurrent(ch);
            break;

        case CMD_ENC_ZERO:
            CMD_EncZero(ch, data, dlc);
            break;

        case CMD_INIT_ROM:
            CMD_InitROM(ch, data, dlc);
            break;
            
        case CMD_CLEAR_ERR: // Set Error flag clear
            CMD_ClearErrors(ch);
            break;

        case CMD_FET_ENABLE:
            itemp = data[2]&0x01;
            CMD_FetEnable(ch, itemp);
            break;

        case CMD_RUN_CLOSEDLOOP:
            itemp = data[2]&0x01;
            itemp2 = data[3];
            CMD_RunClosedLoop(ch, itemp, itemp2);
            break;

        case REQ_ROM_DATA:
            Send_data1(ch, data, dlc);
            break;

        case CMD_SET_POS_LOW_LIM:
            CMD_SetPosLowLimit(ch, data, dlc);
            break;

        case CMD_SET_POS_UP_LIM:
            CMD_SetPosUpLimit(ch, data, dlc);
            break;

        case SEND_TEMPERATURE:
            Send_Temperature(ch);
            break;

        case SET_CURRENT_LIM:
            Set_Curr_lim(ch, data, dlc);
            break;

        case SET_ERROR_LIM:
            Set_Err_lim(ch, data, dlc);
            break;

            // =======================================

        case CMD_DQ_ALIGN:
            CMD_DQ_Align(ch, data, dlc);
            break;


        case CMD_INIT_CONTROL:
            CMD_InitControl(ch, data, dlc);
            break;

            // =======================================

        case CMD_FOC_CURRENT_CONTROL:
            CMD_FOC_Current_Control(ch, data, dlc);
            break;

        case CMD_FOC_GAIN:
            CMD_FOC_Gain(ch, data, dlc);
            break;

        case CMD_FOC_OPENLOOP_CONTROL:
            CMD_FOC_OpenLoop_Control(ch, data, dlc);
            break;

        case CMD_POS_GAIN:
            CMD_POS_Gain(ch, data, dlc);
            break;

        case CMD_CURRENT_NULLING:
            CMD_CurrentNulling(ch, data, dlc);
            break;

        case CMD_SINE_WAVE_TEST:
            CMD_SineWaveTest(ch, data, dlc);
            break;

        case CMD_HOME_ZERO_OFFSET:
            CMD_HomeZeroOffset(ch, data, dlc);
            break;

        case CMD_ENC_RES_DIR:
            CMD_EncResDir(ch, data, dlc);
            break;
    }
}

void Send_MotorName(int ch) {
    char canTxData[8] = {0,};

    canTxData[0] = REQ_NAME_INFO;
    canTxData[1] = 4;
    canTxData[2] = 1;
    canTxData[3] = VERSION & 0xFF;
    canTxData[4] = (VERSION >> 8) &0xFF;
    canTxData[5] = (VERSION >> 16)&0xFF;
    canTxData[6] = (VERSION >> 24)&0xFF;

    if(ch == 0){
        SET_STATUS_FLAG1
        Push_CAN_TX_Ring(CANID_DATA1, 7, canTxData);
    }else{
        SET_STATUS_FLAG2
        Push_CAN_TX_Ring(CANID_DATA2, 7, canTxData);
    }
}

static int temp_send_toggle = 0;
extern unsigned short ultra_sonic_avg;

extern unsigned char LIMIT_STAT[2];
extern unsigned char LIMIT_ADDITIONAL_STAT;
extern unsigned char ULTRA_SONIC_ERROR;
void CMD_SendEncoder(int ch) {
    unsigned char canTxData[8] = {0,};
    long ltemp1 = NewPos[ch]/ENC_MUL;
    int send_temp = ltemp1 & 0x007FFFFF;    
    if (ltemp1 < 0) {
        send_temp |= 0x800000;
    }

    canTxData[0] = send_temp & 0xFF;
    canTxData[1] = (send_temp >> 8) & 0xFF;
    canTxData[2] = (send_temp >> 16) & 0xFF;

    short send_current_q = 0;
    if (measured_current_q_avg[ch] > 32767)
        send_current_q = 32767;
    else if (measured_current_q_avg[ch] < -32767)
        send_current_q = -32767;
    else
        send_current_q = measured_current_q_avg[ch];

    if (MDR[ch] == 1) {
        send_current_q = -send_current_q;
    }    
    canTxData[3] = send_current_q & 0xFF;
    canTxData[4] = (send_current_q >> 8) & 0xFF;
    
//    canTxData[3] = CUR_AS5047 & 0xFF;
//    canTxData[4] = (CUR_AS5047 >> 8) & 0xFF;
    
    canTxData[5] = ((STAT[ch].b.RUN) | ((STAT[ch].b.INIT) << 1) | ((STAT[ch].b.JAM) << 2) | ((STAT[ch].b.CUR) << 3) | ((STAT[ch].b.BIG) << 4) | ((STAT[ch].b.INP) << 5) | ((STAT[ch].b.OP_STATUS) << 6) | ((STAT[ch].b.COLLISION_ERR) << 7));

//    temp_send_toggle ^= 1;

    canTxData[6] = ultra_sonic_avg&0xFF;
    canTxData[7] = ((LIMIT_STAT[ch]&0x01)<<0) | ((LIMIT_ADDITIONAL_STAT&0x01)<<1) | ((ULTRA_SONIC_ERROR&0x01)<<7);
    
//    Push_CAN_TX_Ring_TMR3(CANID_ENC, 7, canTxData);
    if(ch == 0){
        CAN_WRITE_TMR3(CANID_ENC1, 8, canTxData);
    }else{
        CAN_WRITE_TMR3(CANID_ENC2, 8, canTxData);
    }
}

void CMD_SendCurrent(int ch) {
    char canTxData[8] = {0,};
    if (RPM[ch] < 0)
        RPM[ch] = -RPM[ch];
    int send_rpm = RPM[ch] / 10;
    //send_rpm = temperature_board_celsius_x1;

    
    canTxData[0] = measured_current_q_avg[ch] & 0xFF;
    canTxData[1] = (measured_current_q_avg[ch] >> 8)&0xFF;
    canTxData[2] = send_rpm & 0xFF;
    canTxData[3] = (send_rpm >> 8)&0xFF;

    if(ch == 0){
        Push_CAN_TX_Ring(CANID_DATA1, 4, canTxData);
    }else{
        Push_CAN_TX_Ring(CANID_DATA2, 4, canTxData);
    }
}

void CMD_EncZero(int ch, uint8_t data[], uint8_t dlc) {
    CMD_ClearErrors(ch);
    STAT[ch].b.INIT = 0;
    Initialize_MotorParameter(ch);
    //STAT.B=0;

    CONTROL_MODE[ch] = CONTROL_MODE_NO_ACT;
    
    DQ_align(ch, data[2]);

    if(ch == 0){
        SET_STATUS_FLAG1
    }else{
        SET_STATUS_FLAG2
    }
}

void CMD_InitROM(int ch, uint8_t data[], uint8_t dlc){
    if(data[1] == 0){
        // EEPROM
        if(data[2] == 0){
            // Init EEPROM
            if((data[3] == 0x77) && (data[4] == 0x89)){ // KEY
                InitROM(ch, 1); // push to buffer
            }
        }
    }
}



void CMD_ClearErrors(int ch) {
    STAT[ch].B &= 0xFFFF00FF;
    JAM_cnt[ch] = 0;
    OVR_cnt[ch] = 0;
    ERROR_FLAG[ch] = 0;
    if(ch == 0){
        SET_STATUS_FLAG1
    }else{
        SET_STATUS_FLAG2
    }
        
}

void CMD_FetEnable(int ch, int onoff) {
    //UpdateEncoder();
    //ERROR_sum = 0;
    STAT[ch].b.RUN = 0;
    if (onoff == 1) {
        GateDriver_ON(ch);
    } else {
        GateDriver_OFF(ch);
    }
}

void CMD_RunClosedLoop(int ch, int onoff, int mode) {
    // mode 0: original
    // mode 1: gravity <--> position

    if (STAT[ch].b.FET == 1) {
        ERROR_sum[ch] = 0;
        
        if (onoff == 1) {
            Initialize_ControlParameter(ch);
            //UpdateEncoder();
            NewInputRef[ch] = REF[ch] = PREV_REF[ch] = NewPos[ch];
            if (mode == 0) {
                ref_current_q[ch] = 0;
                FOC_Init_CurrentControl(ch);
            }
            STAT[ch].b.RUN = 1;
            STAT[ch].b.MOD = CTRL_MOD_POSITION;
            CONTROL_MODE[ch] = CONTROL_MODE_FOC_POS_CONTROL;
        } else {
            STAT[ch].b.RUN = 0;

            CONTROL_MODE[ch] = CONTROL_MODE_NO_ACT;

            if(ch == 0){
                PDC1 = PWM_STOP;
                PDC2 = PWM_STOP;
            }else{
                PDC3 = PWM_STOP;
                PDC4 = PWM_STOP;
            }
        }
    } else {
        STAT[ch].b.RUN = 0;

        CONTROL_MODE[ch] = CONTROL_MODE_NO_ACT;

        if(ch == 0){
            PDC1 = PWM_STOP;
            PDC2 = PWM_STOP;
        }else{
            PDC3 = PWM_STOP;
            PDC4 = PWM_STOP;
        }
    }
}

void Initialize_MotorParameter(int ch) {
    Initialize_ControlParameter(ch);

//    currentMotorPosition = 0;     // for not change encoder value when DQ SAVE
    REF[ch] = 0; // Reference Command
    PREV_REF[ch] = 0;

    NewInputRef[ch] = 0; // New Reference

    NewPos[ch] = 0; // Current Encoder Position
    OldPos[ch] = 0; // Previous Encoder Position
}

void GateDriver_ON(int ch) {
    if ((STAT[ch].B & 0x00000F00) != 0)
        return;
    
    if(ch == 0){
        MOTOR_HIP_ON1
    }else{
        MOTOR_HIP_ON2
    }
    
    STAT[ch].b.FET = 1;
}

void GateDriver_OFF(int ch) {
    if(ch == 0){
        PDC1 = PWM_STOP;
        PDC2 = PWM_STOP;
        MOTOR_HIP_OFF1
    }else{
        PDC3 = PWM_STOP;
        PDC4 = PWM_STOP;
        MOTOR_HIP_OFF2
    }
    
    STAT[ch].b.FET = 0;
    STAT[ch].b.RUN = 0;
}

void Send_data1(int ch, uint8_t data[], uint8_t dlc) {
    unsigned int send_dlc;
    unsigned int itemp;
    int data_type, is_err = 0;
    char canTxData[8] = {0,};

    data_type = data[2];

    switch (data_type) {
        case 1:
            send_dlc = 8;
            canTxData[0] = 0;
            canTxData[1] = 0;
            canTxData[2] = 0;
            canTxData[3] = 0;
            canTxData[4] = 0;
            canTxData[5] = 0;
            itemp = 16384 & 0x3FFF | ((MDR[ch] & 0x01) << 15);
            canTxData[6] = itemp & 0xFF;
            canTxData[7] = (itemp >> 8)&0xFF;
            break;
        case 2:
            send_dlc = 8;
            canTxData[0] = 0;
            canTxData[1] = 0;
            canTxData[2] = 0;
            canTxData[3] = 0;
            canTxData[4] = 0;
            canTxData[5] = 0;
            canTxData[6] = 0;
            canTxData[7] = 0;
            break;
        case 3:
            send_dlc = 8;
            canTxData[0] = 0;
            canTxData[1] = 0;
            canTxData[2] = RomData[ch][EE_TYPE_POS_LOWER_LIMIT] & 0xFF;
            canTxData[3] = (RomData[ch][EE_TYPE_POS_LOWER_LIMIT] >> 8)&0xFF;
            canTxData[4] = (RomData[ch][EE_TYPE_POS_LOWER_LIMIT] >> 16)&0xFF;
            canTxData[5] = (RomData[ch][EE_TYPE_POS_LOWER_LIMIT] >> 24)&0xFF;
            canTxData[6] = RomData[ch][EE_TYPE_POS_UPPER_LIMIT] & 0xFF;
            canTxData[7] = (RomData[ch][EE_TYPE_POS_UPPER_LIMIT] >> 8)&0xFF;
            break;
        case 4:
            send_dlc = 8;
            canTxData[0] = (RomData[ch][EE_TYPE_POS_UPPER_LIMIT] >> 16)&0xFF;
            canTxData[1] = (RomData[ch][EE_TYPE_POS_UPPER_LIMIT] >> 24)&0xFF;
            canTxData[2] = 0;
            canTxData[3] = 0;
            canTxData[4] = 0;
            canTxData[5] = 0;
            break;
        case 5:
            send_dlc = 8;
            canTxData[0] = 0;
            canTxData[1] = 0;
            canTxData[2] = 0;
            canTxData[3] = 0;
            canTxData[4] = 0;
            canTxData[5] = 0;
            canTxData[6] = RomData[ch][EE_TYPE_JAM_LIMIT_CURRENT] & 0xFF;
            canTxData[7] = (RomData[ch][EE_TYPE_JAM_LIMIT_CURRENT] >> 8)&0xFF;
            break;
        case 6:
            send_dlc = 8;
            canTxData[0] = RomData[ch][EE_TYPE_OVR_LIMIT_CURRENT] & 0xFF;
            canTxData[1] = (RomData[ch][EE_TYPE_OVR_LIMIT_CURRENT] >> 8)&0xFF;
            canTxData[2] = RomData[ch][EE_TYPE_JAM_LIMIT_TIME] & 0xFF;
            canTxData[3] = (RomData[ch][EE_TYPE_JAM_LIMIT_TIME] >> 8)&0xFF;
            canTxData[4] = RomData[ch][EE_TYPE_OVR_LIMIT_TIME] & 0xFF;
            canTxData[5] = (RomData[ch][EE_TYPE_OVR_LIMIT_TIME] >> 8)&0xFF;
            canTxData[6] = 0;
            canTxData[7] = 0;
            break;
        case 7:
            send_dlc = 8;
            canTxData[0] = 0;
            canTxData[1] = 0;
            canTxData[2] = 0;
            canTxData[3] = 0;
            canTxData[4] = RomData[ch][EE_TYPE_INPUT_ERROR] & 0xFF;
            canTxData[5] = (RomData[ch][EE_TYPE_INPUT_ERROR] >> 8)&0xFF;
            canTxData[6] = RomData[ch][EE_TYPE_BIG_ERROR] & 0xFF;
            canTxData[7] = (RomData[ch][EE_TYPE_BIG_ERROR] >> 8)&0xFF;
            break;
        case 8:
            send_dlc = 6;
            canTxData[0] = RomData[MAX_CH][EE_COMMON_JMC] & 0xFF;
            canTxData[1] = RomData[MAX_CH][EE_COMMON_JMC]&0xFF;
            canTxData[2] = 0;
            canTxData[3] = RomData[MAX_CH][EE_COMMON_BOARDTYPE]&0xFF;
            canTxData[4] = RomData[MAX_CH][EE_COMMON_TEMP_ERROR] & 0xFF;
            canTxData[5] = (RomData[MAX_CH][EE_COMMON_TEMP_ERROR] >> 8)&0xFF;
            break;
        default:
            is_err = 1;
            break;
    }
    if (is_err == 0){
        if(ch == 0){
            Push_CAN_TX_Ring(CANID_DATA1, send_dlc, canTxData);
        }else{
            Push_CAN_TX_Ring(CANID_DATA2, send_dlc, canTxData);
        }
    }
        
}

void CMD_SetPosLowLimit(int ch, uint8_t data[], uint8_t dlc) {

    MAX_POS1_flag[ch] = (data[2])&0x01;
    if ((data[2]&0x02) == 0x02) {
        POS_LOWER_LIM[ch] = (data[6]);
        POS_LOWER_LIM[ch] = (POS_LOWER_LIM[ch] << 8) | data[5];
        POS_LOWER_LIM[ch] = (POS_LOWER_LIM[ch] << 8) | data[4];
        POS_LOWER_LIM[ch] = (POS_LOWER_LIM[ch] << 8) | data[3];
    }
    if (MAX_POS1_flag[ch] == 1)
        POS_LOWER_LIM[ch] |= 0x01;
    else
        POS_LOWER_LIM[ch] &= 0xFFFFFFFE;

    Push_EEPROM_Ring(ch, EE_TYPE_POS_LOWER_LIMIT, POS_LOWER_LIM[ch]);
    RomData[ch][EE_TYPE_POS_LOWER_LIMIT] = POS_LOWER_LIM[ch];
}

void CMD_SetPosUpLimit(int ch, uint8_t data[], uint8_t dlc) {
 
    MAX_POS2_flag[ch] = (data[2])&0x01;
    if ((data[2]&0x02) == 0x02) {
        POS_UPPER_LIM[ch] = (data[6]);
        POS_UPPER_LIM[ch] = (POS_UPPER_LIM[ch] << 8) | data[5];
        POS_UPPER_LIM[ch] = (POS_UPPER_LIM[ch] << 8) | data[4];
        POS_UPPER_LIM[ch] = (POS_UPPER_LIM[ch] << 8) | data[3];
    }
    if (MAX_POS2_flag[ch] == 1)
        POS_UPPER_LIM[ch] |= 0x01;
    else
        POS_UPPER_LIM[ch] &= 0xFFFFFFFE;

    Push_EEPROM_Ring(ch, EE_TYPE_POS_UPPER_LIMIT, POS_UPPER_LIM[ch]);
    RomData[ch][EE_TYPE_POS_UPPER_LIMIT] = POS_UPPER_LIM[ch];
}


void Send_Temperature(int ch) {
    unsigned char canTxData[8] = {0,};

    canTxData[0] = SEND_TEMPERATURE;
    canTxData[1] = temperature_board_celsius_x10 & 0xFF;
    canTxData[2] = (temperature_board_celsius_x10 >> 8) &0xFF;
    canTxData[3] = temperature_motor_celsius_x10 & 0xFF;
    canTxData[4] = (temperature_motor_celsius_x10 >> 8)&0xFF;
    canTxData[5] = 0 & 0xFF;
    canTxData[6] = (0 >> 8)&0xFF;

    if(ch == 0){
        Push_CAN_TX_Ring(CANID_DATA1, 7, canTxData);
    }else{
        Push_CAN_TX_Ring(CANID_DATA2, 7, canTxData);
    }
}

void Set_Curr_lim(int ch, uint8_t data[], uint8_t dlc) {
   
    JAM_LIM_Current[ch] = (short) (data[2] | (data[3] << 8));
    OVR_LIM_Current[ch] = (short) (data[4] | (data[5] << 8));
    JAM_LIM_Time[ch] = (int) ((unsigned char) data[6]);
    OVR_LIM_Time[ch] = (int) ((unsigned char) data[7]);

    Push_EEPROM_Ring(ch, EE_TYPE_JAM_LIMIT_CURRENT, JAM_LIM_Current[ch]);
    Push_EEPROM_Ring(ch, EE_TYPE_OVR_LIMIT_CURRENT, OVR_LIM_Current[ch]);
    Push_EEPROM_Ring(ch, EE_TYPE_JAM_LIMIT_TIME, JAM_LIM_Time[ch]);
    Push_EEPROM_Ring(ch, EE_TYPE_OVR_LIMIT_TIME, OVR_LIM_Time[ch]);
    RomData[ch][EE_TYPE_JAM_LIMIT_CURRENT] = JAM_LIM_Current[ch];
    RomData[ch][EE_TYPE_OVR_LIMIT_CURRENT] = OVR_LIM_Current[ch];
    RomData[ch][EE_TYPE_JAM_LIMIT_TIME] = JAM_LIM_Time[ch];
    RomData[ch][EE_TYPE_OVR_LIMIT_TIME] = OVR_LIM_Time[ch];
}

void Set_Err_lim(int ch, uint8_t data[], uint8_t dlc) {
    unsigned int i, b, e;
  
    i = (data[3]);
    i = (i << 8) | data[2];
    b = (data[5]);
    b = (b << 8) | data[4];
    e = (data[7]);
    e = (e << 8) | data[6];
    I_ERR[ch] = (long) i;
    B_ERR[ch] = (long) b;
    T_ERR = e;

    if (I_ERR[ch] > 0xFFFF)
        I_ERR[ch] = 0xFFFF;

    if (B_ERR[ch] > 0xFFFF)
        B_ERR[ch] = 0xFFFF;

    Push_EEPROM_Ring(ch, EE_TYPE_BIG_ERROR, B_ERR[ch]);
    Push_EEPROM_Ring(ch, EE_TYPE_INPUT_ERROR, I_ERR[ch]);
    Push_EEPROM_Ring(MAX_CH, EE_COMMON_TEMP_ERROR, T_ERR);
    RomData[ch][EE_TYPE_BIG_ERROR] = B_ERR[ch];
    RomData[ch][EE_TYPE_INPUT_ERROR] = I_ERR[ch];
    RomData[ch][EE_COMMON_TEMP_ERROR] = T_ERR;
}

void CMD_DQ_Align(int ch, uint8_t data[], uint8_t dlc) {
    CMD_ClearErrors(ch);

    DQ_align(ch, data[1]);
}

void CMD_InitControl(int ch, uint8_t data[], uint8_t dlc) {
    if(InitControlFlag[ch] == 1 || InitHomeFlag[ch] == 1){
        return;
    }
    goto_param[ch].move_flag = 0;
    goto_param[ch].stop_flag = 0;
    override_flag[ch] = 0;
    
    InitControlState[ch] = 1;
    InitControlFlag[ch] = 1;
    
    OpenPositionPulse[ch] = (int) (data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24));
}


void CMD_FOC_Current_Control(int ch, uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // stop
        CONTROL_MODE[ch] = CONTROL_MODE_NO_ACT;
        if(ch == 0){
            PDC1 = PWM_STOP;
            PDC2 = PWM_STOP;
        }else{
            PDC3 = PWM_STOP;
            PDC4 = PWM_STOP;
        }
    } else if (data[1] == 1) {
        // run
        if (CONTROL_MODE[ch] != CONTROL_MODE_FOC_CUR_CONTROL) {
            STAT[ch].b.RUN = 0;
            STAT[ch].b.MOD = CTRL_MOD_CURRENT;
            FOC_Init_CurrentControl(ch);
            CONTROL_MODE[ch] = CONTROL_MODE_FOC_CUR_CONTROL;
        }
    }
}

void CMD_FOC_Gain(int ch, uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // set params
        Pgain_q1000[ch] = (int) ((short) (data[2] | (data[3] << 8)));
        Igain_q1000[ch] = (int) ((short) (data[4] | (data[5] << 8)));
        Push_EEPROM_Ring(ch, EE_TYPE_FOC_PGAIN, Pgain_q1000[ch]);
        Push_EEPROM_Ring(ch, EE_TYPE_FOC_IGAIN, Igain_q1000[ch]);
    } else if (data[1] == 1) {
        // request params
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_FOC_GAIN;
        canTxData[1] = Pgain_q1000[ch] & 0xFF;
        canTxData[2] = (Pgain_q1000[ch] >> 8) & 0xFF;
        canTxData[3] = Igain_q1000[ch] & 0xFF;
        canTxData[4] = (Igain_q1000[ch] >> 8) & 0xFF;
        if(ch == 0){
            Push_CAN_TX_Ring(CANID_DATA1, 5, canTxData);
        }else{
            Push_CAN_TX_Ring(CANID_DATA2, 5, canTxData);
        }
    }
}

void CMD_FOC_OpenLoop_Control(int ch, uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // stop
        CONTROL_MODE[ch] = CONTROL_MODE_NO_ACT;

        if(ch == 0){
            PDC1 = PWM_STOP;
            PDC2 = PWM_STOP;
        }else{
            PDC3 = PWM_STOP;
            PDC4 = PWM_STOP;
        }
    } else if (data[1] == 1) {
        // run
        ref_voltage[ch] = (int) ((short) (data[2] | (data[3] << 8)))*10;
        if (CONTROL_MODE[ch] != CONTROL_MODE_FOC_OPENLOOP_CONTROL) {
            STAT[ch].b.RUN = 0;
            CONTROL_MODE[ch] = CONTROL_MODE_FOC_OPENLOOP_CONTROL;
        }
    }
}


void CMD_POS_Gain(int ch, uint8_t data[], uint8_t dlc) {
    // Main Gain 1
    if (data[1] == 0) {
        // set params
        Pgain_pos[ch] = (int) ((unsigned short) (data[2] | (data[3] << 8)));
        Igain_pos[ch] = (int) ((unsigned short) (data[4] | (data[5] << 8)));
        Dgain_pos[ch] = (int) ((unsigned short) (data[6] | (data[7] << 8)));
        Push_EEPROM_Ring(ch, EE_TYPE_POS_PGAIN, Pgain_pos[ch]);
        Push_EEPROM_Ring(ch, EE_TYPE_POS_IGAIN, Igain_pos[ch]);
        Push_EEPROM_Ring(ch, EE_TYPE_POS_DGAIN, Dgain_pos[ch]);

    } else if (data[1] == 1) {
        // request params
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_POS_GAIN;
        canTxData[1] = Pgain_pos[ch] & 0xFF;
        canTxData[2] = (Pgain_pos[ch] >> 8) & 0xFF;
        canTxData[3] = Igain_pos[ch] & 0xFF;
        canTxData[4] = (Igain_pos[ch] >> 8) & 0xFF;
        canTxData[5] = Dgain_pos[ch] & 0xFF;
        canTxData[6] = (Dgain_pos[ch] >> 8) & 0xFF;
        canTxData[7] = 1;
        if(ch == 0){
            Push_CAN_TX_Ring(CANID_DATA1, 8, canTxData);
        }else{
            Push_CAN_TX_Ring(CANID_DATA2, 8, canTxData);
        }
    }// Additional Gain 2
    else if (data[1] == 2) {
        // set params

    } else if (data[1] == 3) {
        // request params
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_POS_GAIN;
        canTxData[1] = 0;
        canTxData[2] = 0;
        canTxData[3] = 0;
        canTxData[4] = 0;
        canTxData[5] = 0;
        canTxData[6] = 0;
        canTxData[7] = 2;
        if(ch == 0){
            Push_CAN_TX_Ring(CANID_DATA1, 8, canTxData);
        }else{
            Push_CAN_TX_Ring(CANID_DATA2, 8, canTxData);
        }
    }// Additional Gain 3
    else if (data[1] == 4) {
        // set params
        
    } else if (data[1] == 5) {
        // request params
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_POS_GAIN;
        canTxData[1] = 0;
        canTxData[2] = 0;
        canTxData[3] = 0;
        canTxData[4] = 0;
        canTxData[5] = 0;
        canTxData[6] = 0;
        canTxData[7] = 3;
        if(ch == 0){
            Push_CAN_TX_Ring(CANID_DATA1, 8, canTxData);
        }else{
            Push_CAN_TX_Ring(CANID_DATA2, 8, canTxData);
        }
    }

}

void CMD_CurrentNulling(int ch, uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // set params
        CurrentNulling(ch);
        Push_EEPROM_Ring(ch, EE_TYPE_CURRENT_NULL_RZ, RZ[ch]);

    } else if (data[1] == 1) {
        // request params
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_CURRENT_NULLING;
        canTxData[1] = RZ[ch] & 0xFF;
        canTxData[2] = (RZ[ch] >> 8) & 0xFF;
        canTxData[3] = 0 & 0xFF;
        canTxData[4] = (0 >> 8) & 0xFF;
        canTxData[5] = 0 & 0xFF;
        canTxData[6] = (0 >> 8) & 0xFF;
        if(ch == 0){
            Push_CAN_TX_Ring(CANID_DATA1, 7, canTxData);
        }else{
            Push_CAN_TX_Ring(CANID_DATA2, 7, canTxData);
        }
    }
}


void CMD_SineWaveTest(int ch, uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // stop test -- goto
        goto_param[ch].move_flag = 0;
    } else if (data[1] == 1) {
        // start test -- goto
        if (STAT[ch].b.FET == 0 || STAT[ch].b.RUN == 0){
            return;
        }
         if(speed_test_start[ch] != 0){
            return;
        }
        if(goto_param[ch].move_flag == 1){
            goto_param[ch].stop_flag = 1;
        }
        if(GO_POS_CNT[ch] < 3){
            return;
        }
        
        STAT[ch].b.COLLISION_ERR = 0;
        STAT[ch].b.BIG = 0;
        STAT[ch].b.JAM = 0;
        STAT[ch].b.CUR = 0;
        STAT[ch].b.INP = 0;
        
//        int target_pos = (int) (data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24));
//        //int target_cnt = (int) ((unsigned short) (data[6] | (data[7] << 8)));
//        int target_vel = (int) ((unsigned short) (data[6] | (data[7] << 8))); // pulse/tick

        float target_pos = (float)((int) (data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24)));
        float target_vel = (float)((int) ((unsigned short) (data[6] | (data[7] << 8)))); // pulse/tick
        
        if(goto_param[ch].stop_flag == 1){
            override_target[ch] = target_pos;
            override_speed[ch] = target_vel;
            override_flag[ch] = 1;
        }else{
            if(fabs(goto_param[ch].cur_pos - (currentMotorPosition[ch])) > 1000){
                goto_param[ch].cur_pos = currentMotorPosition[ch];
                set_goto_start_pos(ch, currentMotorPosition[ch]);
            }else{
                set_goto_start_pos(ch, goto_param[ch].cur_pos);
            }
            set_goto_target(ch, target_pos*ENC_MUL, target_vel*ENC_MUL);
        }
        
        
        CONTROL_MODE[ch] = CONTROL_MODE_FOC_POS_CONTROL;
    } else if (data[1] == 2) {
        // stop test -- speed
        speed_test_start[ch] = 2;

    } else if (data[1] == 3) {
        // start test -- speed
        if (STAT[ch].b.FET == 0 || STAT[ch].b.RUN == 0) {
            return;
        }
        if(speed_test_start[ch] != 0){
            return;
        }
        if(goto_param[ch].move_flag == 1){
            return;
        }
        if(GO_POS_CNT[ch] < 3){
            return;
        }
        
        int target_rpm = (int) (data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24));
        speed_test_acc[ch] = (int) ((unsigned short) (data[6] | (data[7] << 8)));

        speed_test_target[ch] = target_rpm * ENC_RES[ch] * ENC_MUL / 500 / 60;
        speed_test_acc[ch] = fabs(speed_test_target[ch]) / 200;
        if (speed_test_acc[ch] < 1)
            speed_test_acc[ch] = 1;
        if(speed_test_start[ch] == 0){
            speed_test_cur_speed[ch] = 0;
        }
        speed_test_start[ch] = 1;
        CONTROL_MODE[ch] = CONTROL_MODE_FOC_POS_CONTROL;
    }
}


void CMD_HomeZeroOffset(int ch, uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // set zero offset
        if (data[6] == 1) {
            // set cur pos zero
            HOME_ZERO_OFFSET[ch] = NewPos[ch];
        } else {
            HOME_ZERO_OFFSET[ch] = (int) (data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24));
        }
        Push_EEPROM_Ring(ch, EE_TYPE_HOME_ZERO_OFFSET, HOME_ZERO_OFFSET[ch]);
    } else if (data[1] == 1) {
        // request zero offset
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_HOME_ZERO_OFFSET;
        canTxData[1] = HOME_ZERO_OFFSET[ch]&0xFF;
        canTxData[2] = (HOME_ZERO_OFFSET[ch]>>8)&0xFF;
        canTxData[3] = (HOME_ZERO_OFFSET[ch]>>16)&0xFF;
        canTxData[4] = (HOME_ZERO_OFFSET[ch]>>24)&0xFF;
        if(ch == 0){
            Push_CAN_TX_Ring(CANID_DATA1, 5, canTxData);
        }else{
            Push_CAN_TX_Ring(CANID_DATA2, 5, canTxData);
        }
    }
}

void CMD_EncResDir(int ch, uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        int enc_res = (data[2] | (data[3]<<8));
        ENC_RES[ch] = enc_res;
        Push_EEPROM_Ring(ch, EE_TYPE_ENC_RES, ENC_RES[ch]);
        
        HOME_DIR[ch] = data[4];
        Push_EEPROM_Ring(ch, EE_TYPE_HOME_DIR, HOME_DIR[ch]);
        
        if (MDR[ch] == data[5]) {
            MDR[ch] = data[5];
        } else {
            MDR[ch] = data[5];
            HOME_ZERO_OFFSET[ch] = -HOME_ZERO_OFFSET[ch];
            Push_EEPROM_Ring(ch, EE_TYPE_HOME_ZERO_OFFSET, HOME_ZERO_OFFSET[ch]);
        }
        Push_EEPROM_Ring(ch, EE_TYPE_MOTOR_DIR, MDR[ch]);
    } else if (data[1] == 1) {
        // request enc resolution & pole number & direction
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_ENC_RES_DIR;
        canTxData[1] = ENC_RES[ch] & 0xFF;
        canTxData[2] = (ENC_RES[ch] >> 8) & 0xFF;
        canTxData[3] = HOME_DIR[ch];
        canTxData[4] = MDR[ch];
        if(ch == 0){
            Push_CAN_TX_Ring(CANID_DATA1, 5, canTxData);
        }else{
            Push_CAN_TX_Ring(CANID_DATA2, 5, canTxData);
        }
    }
}
