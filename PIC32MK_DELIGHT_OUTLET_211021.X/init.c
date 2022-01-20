//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>

#include "init.h"
#include "board_definition.h"
#include "timer.h"
#include "port.h"
#include "can.h"
#include "adc.h"
#include "qei.h"
#include "spi.h"
#include "status.h"
#include "control.h"
#include "execution.h"
#include "dac.h"
#include "FOC.h"


static void InitParameters();
static void GetROM(int ch);
static void ROMData(int ch);
extern unsigned int _SPI_WRITE(unsigned int data);



void InitBoardIdentification(){
    int i, j, try_cnt;
    
    STAT[0].B = 0;
    STAT[1].B = 0;
        
    EEPROM_DESELECT
   
    //InitSPI(8, 2);  // 8bit, high idle - falling edge
    InitSPI(8, 1);
    
    
    for(i=0;i<20000;i++)
        ;
        
    
    int temp_bno = 0;
    if(PORTGbits.RG9 == 1){
        temp_bno += 1;
    }
    if(PORTGbits.RG7 == 1){
        temp_bno += 2;
    }
    if(PORTGbits.RG6 == 1){
        temp_bno += 4;
    }
    JMC = temp_bno;
    
    

    // == Initialize Peripherals & Parameters =====   
	InitParameters();
 
	GetROM(0); 
    GetROM(1); 
	ROMData(0);
    ROMData(1);
  
    for(i=0; i<1000; ++i)
        for(j=0; j<500; ++j)
            ;
        
        
    // Force to Write EEPROM
//    Push_EEPROM_Ring(MAX_CH, EE_COMMON_JMC, 1);
//    InitROM(1);
}

void InitPeripheral(){
    InitCANID(JMC);
    InitCAN();
    InitADC(0);
    InitQEI();
    
    InitPWM();
    
    InitTimer2();
    InitIC();
    
    //InitDAC();
}

static void InitParameters(){
    int ch;
    for(ch=0; ch<MAX_CH; ch++){
        ERROR_sum[ch] = 0.0;

        // Initialize Status and Control Bytes
        STAT[ch].B = 0;		

        JAM_cnt[ch] = 0;
        OVR_cnt[ch] = 0;
        Initialize_MotorParameter(ch);
    }
    
}


void InitROM(int ch, int mode){   
    if(mode == 0){
        // Write Directly
//        EEPROM_WRITE(MAX_CH, EE_COMMON_JMC, JMC);
//        EEPROM_WRITE(MAX_CH, EE_COMMON_BOARDTYPE, 4);
//        EEPROM_WRITE(MAX_CH, EE_COMMON_JMC_NOT, ~(JMC&0xFF));
        EEPROM_WRITE(MAX_CH, EE_COMMON_TEMP_ERROR, DEFAULT_TEMP_ERROR);
//        EEPROM_WRITE(MAX_CH, EE_COMMON_LAST_ENCODER, 0);

        EEPROM_WRITE(ch, EE_TYPE_MOTOR_DIR, 0);
        EEPROM_WRITE(ch, EE_TYPE_HOME_DIR, 0);
        
        EEPROM_WRITE(ch, EE_TYPE_JAM_LIMIT_CURRENT, DEFAULT_JAM_LIMIT_CURRENT);
        EEPROM_WRITE(ch, EE_TYPE_OVR_LIMIT_CURRENT, DEFAULT_OVR_LIMIT_CURRENT);
        EEPROM_WRITE(ch, EE_TYPE_JAM_LIMIT_TIME, DEFAULT_JAM_LIMIT_TIME);
        EEPROM_WRITE(ch, EE_TYPE_OVR_LIMIT_TIME, DEFAULT_OVR_LIMIT_TIME);
        EEPROM_WRITE(ch, EE_TYPE_INPUT_ERROR, DEFAULT_INPUT_ERROR);
        EEPROM_WRITE(ch, EE_TYPE_BIG_ERROR, DEFAULT_BIG_ERROR);
        EEPROM_WRITE(ch, EE_TYPE_POS_LOWER_LIMIT, DEFAULT_POS_LOWER_LIMIT);
        EEPROM_WRITE(ch, EE_TYPE_POS_UPPER_LIMIT, DEFAULT_POS_UPPER_LIMIT);


        EEPROM_WRITE(ch, EE_TYPE_FOC_PGAIN, DEFAULT_FOC_PGAIN);
        EEPROM_WRITE(ch, EE_TYPE_FOC_IGAIN, DEFAULT_FOC_IGAIN);
        
        if(ch == 0){
            EEPROM_WRITE(ch, EE_TYPE_POS_IGAIN, DEFAULT_SPIN_POS_IGAIN);
            EEPROM_WRITE(ch, EE_TYPE_POS_DGAIN, DEFAULT_SPIN_POS_DGAIN);
            EEPROM_WRITE(ch, EE_TYPE_POS_PGAIN, DEFAULT_SPIN_POS_PGAIN);
        }else{
            EEPROM_WRITE(ch, EE_TYPE_POS_IGAIN, DEFAULT_DOOR_POS_IGAIN);
            EEPROM_WRITE(ch, EE_TYPE_POS_DGAIN, DEFAULT_DOOR_POS_DGAIN);
            EEPROM_WRITE(ch, EE_TYPE_POS_PGAIN, DEFAULT_DOOR_POS_PGAIN);
        }


        EEPROM_WRITE(ch, EE_TYPE_CURRENT_NULL_RZ, DEFAULT_CURRENT_NULL_RZ);
        
        EEPROM_WRITE(ch, EE_TYPE_ENC_RES, DEFAULT_ENC_RES);

        
    }else{
        // Write to Buffer
//        Push_EEPROM_Ring(MAX_CH, EE_COMMON_JMC, JMC);
//        Push_EEPROM_Ring(MAX_CH, EE_COMMON_BOARDTYPE, 4);
//        Push_EEPROM_Ring(MAX_CH, EE_COMMON_JMC_NOT, ~(JMC&0xFF));
        Push_EEPROM_Ring(MAX_CH, EE_COMMON_TEMP_ERROR, DEFAULT_TEMP_ERROR);
//        Push_EEPROM_Ring(MAX_CH, EE_COMMON_LAST_ENCODER, 0);

        Push_EEPROM_Ring(ch, EE_TYPE_MOTOR_DIR, 0);
        Push_EEPROM_Ring(ch, EE_TYPE_HOME_DIR, 0);
        
        Push_EEPROM_Ring(ch, EE_TYPE_JAM_LIMIT_CURRENT, DEFAULT_JAM_LIMIT_CURRENT);
        Push_EEPROM_Ring(ch, EE_TYPE_OVR_LIMIT_CURRENT, DEFAULT_OVR_LIMIT_CURRENT);
        Push_EEPROM_Ring(ch, EE_TYPE_JAM_LIMIT_TIME, DEFAULT_JAM_LIMIT_TIME);
        Push_EEPROM_Ring(ch, EE_TYPE_OVR_LIMIT_TIME, DEFAULT_OVR_LIMIT_TIME);
        Push_EEPROM_Ring(ch, EE_TYPE_INPUT_ERROR, DEFAULT_INPUT_ERROR);
        Push_EEPROM_Ring(ch, EE_TYPE_BIG_ERROR, DEFAULT_BIG_ERROR);
        Push_EEPROM_Ring(ch, EE_TYPE_POS_LOWER_LIMIT, DEFAULT_POS_LOWER_LIMIT);
        Push_EEPROM_Ring(ch, EE_TYPE_POS_UPPER_LIMIT, DEFAULT_POS_UPPER_LIMIT);


        Push_EEPROM_Ring(ch, EE_TYPE_FOC_PGAIN, DEFAULT_FOC_PGAIN);
        Push_EEPROM_Ring(ch, EE_TYPE_FOC_IGAIN, DEFAULT_FOC_IGAIN);
        
        
        if(ch == 0){
            Push_EEPROM_Ring(ch, EE_TYPE_POS_PGAIN, DEFAULT_SPIN_POS_PGAIN);
            Push_EEPROM_Ring(ch, EE_TYPE_POS_IGAIN, DEFAULT_SPIN_POS_IGAIN);
            Push_EEPROM_Ring(ch, EE_TYPE_POS_DGAIN, DEFAULT_SPIN_POS_DGAIN);
        }else{
            Push_EEPROM_Ring(ch, EE_TYPE_POS_PGAIN, DEFAULT_DOOR_POS_PGAIN);
            Push_EEPROM_Ring(ch, EE_TYPE_POS_IGAIN, DEFAULT_DOOR_POS_IGAIN);
            Push_EEPROM_Ring(ch, EE_TYPE_POS_DGAIN, DEFAULT_DOOR_POS_DGAIN);
        }
        
        Push_EEPROM_Ring(ch, EE_TYPE_CURRENT_NULL_RZ, DEFAULT_CURRENT_NULL_RZ);

        Push_EEPROM_Ring(ch, EE_TYPE_ENC_RES, DEFAULT_ENC_RES);
    }
    
}



static void GetROM(int ch){
    T_ERR = DEFAULT_TEMP_ERROR;//EEPROM_READ(MAX_CH, EE_COMMON_TEMP_ERROR);
    
    MDR[ch] = 0;//EEPROM_READ(0, EE_TYPE_MOTOR_DIR);
    HOME_DIR[ch] = 0;//EEPROM_READ(0, EE_TYPE_HOME_DIR);
    ENC_RES[ch] = DEFAULT_ENC_RES;//EEPROM_READ(0, EE_TYPE_ENC_RES);
    
    JAM_LIM_Current[ch] = DEFAULT_JAM_LIMIT_CURRENT;//EEPROM_READ(0, EE_TYPE_JAM_LIMIT_CURRENT);
    OVR_LIM_Current[ch] = DEFAULT_OVR_LIMIT_CURRENT;//EEPROM_READ(0, EE_TYPE_OVR_LIMIT_CURRENT);
    JAM_LIM_Time[ch] = DEFAULT_JAM_LIMIT_TIME;//EEPROM_READ(0, EE_TYPE_JAM_LIMIT_TIME);
    OVR_LIM_Time[ch] = DEFAULT_OVR_LIMIT_TIME;//EEPROM_READ(0, EE_TYPE_OVR_LIMIT_TIME);
    I_ERR[ch] = DEFAULT_INPUT_ERROR;//EEPROM_READ(0, EE_TYPE_INPUT_ERROR);
    B_ERR[ch] = DEFAULT_BIG_ERROR;//EEPROM_READ(0, EE_TYPE_BIG_ERROR);
    POS_LOWER_LIM[ch] = DEFAULT_POS_LOWER_LIMIT;//EEPROM_READ(0, EE_TYPE_POS_LOWER_LIMIT);
    POS_UPPER_LIM[ch] = DEFAULT_POS_UPPER_LIMIT;//EEPROM_READ(0, EE_TYPE_POS_UPPER_LIMIT);
    
    HOME_ZERO_OFFSET[ch] = EEPROM_READ(ch, EE_TYPE_HOME_ZERO_OFFSET);
    
    CUR_BIG_Time[ch] = 10;  // 1sec
    CUR_BIG_LIM[ch] = 5000; // 5A
    
    OVER_CURRENT[ch] = OVR_LIM_Current[ch] * 8 / 10;
    OVER_CURRENT_TIME[ch] = 20; // 20ms
    

    if(POS_LOWER_LIM[ch] & 0x01){
        MAX_POS1_flag[ch] = 1;
    }else{
        MAX_POS1_flag[ch] = 0;
    }
    
    if(POS_UPPER_LIM[ch] & 0x01){
        MAX_POS2_flag[ch] = 1;
    }else{
        MAX_POS2_flag[ch] = 0;
    }
    
    if(I_ERR[ch] > 0xFFFF)
        I_ERR[ch]=0xFFFF;
    if(B_ERR[ch] > 0xFFFF)
        B_ERR[ch]=0xFFFF;

    Pgain_q1000[ch] = DEFAULT_FOC_PGAIN;//EEPROM_READ(0, EE_TYPE_FOC_PGAIN);
    Igain_q1000[ch] = DEFAULT_FOC_IGAIN;//EEPROM_READ(0, EE_TYPE_FOC_IGAIN);
    
    if(ch == 0){
        Pgain_pos[ch] = DEFAULT_SPIN_POS_PGAIN;//EEPROM_READ(0, EE_TYPE_POS_PGAIN);
        Igain_pos[ch] = DEFAULT_SPIN_POS_IGAIN;//EEPROM_READ(0, EE_TYPE_POS_IGAIN);
        Dgain_pos[ch] = DEFAULT_SPIN_POS_DGAIN;//EEPROM_READ(0, EE_TYPE_POS_DGAIN);
    }else{
        Pgain_pos[ch] = DEFAULT_DOOR_POS_PGAIN;//EEPROM_READ(0, EE_TYPE_POS_PGAIN);
        Igain_pos[ch] = DEFAULT_DOOR_POS_IGAIN;//EEPROM_READ(0, EE_TYPE_POS_IGAIN);
        Dgain_pos[ch] = DEFAULT_DOOR_POS_DGAIN;//EEPROM_READ(0, EE_TYPE_POS_DGAIN);
    }
    
    RZ[ch] = DEFAULT_CURRENT_NULL_RZ;// EEPROM_READ(0, EE_TYPE_CURRENT_NULL_RZ);
    
}


static void ROMData(int ch){
    RomData[MAX_CH][EE_COMMON_JMC] = JMC;
    RomData[MAX_CH][EE_COMMON_BOARDTYPE] = 4;
    RomData[MAX_CH][EE_COMMON_TEMP_ERROR] = T_ERR;
      
    RomData[ch][EE_TYPE_MOTOR_DIR] = MDR[ch];

    RomData[ch][EE_TYPE_JAM_LIMIT_CURRENT] = JAM_LIM_Current[ch];
    RomData[ch][EE_TYPE_OVR_LIMIT_CURRENT] = OVR_LIM_Current[ch];
    RomData[ch][EE_TYPE_JAM_LIMIT_TIME] = JAM_LIM_Time[ch];
    RomData[ch][EE_TYPE_OVR_LIMIT_TIME] = OVR_LIM_Time[ch];
    RomData[ch][EE_TYPE_INPUT_ERROR] = I_ERR[ch];
    RomData[ch][EE_TYPE_BIG_ERROR] = B_ERR[ch];
    RomData[ch][EE_TYPE_POS_LOWER_LIMIT] = POS_LOWER_LIM[ch];
    RomData[ch][EE_TYPE_POS_UPPER_LIMIT] = POS_UPPER_LIM[ch];
}
