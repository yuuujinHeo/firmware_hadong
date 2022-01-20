//#include <proc/p32mk1024mcf064.h>
#include <proc/p32mk0512mcf064.h>
#include <../xc.h>

#include <sys/attribs.h>
#include <math.h>

#include "cpu_config.h"
#include "board_definition.h"
#include "init.h"
#include "timer.h"
#include "port.h"
#include "can.h"
#include "adc.h"
#include "qei.h"
#include "spi.h"
#include "status.h"
#include "control.h"
#include "execution.h"
#include "FOC.h"
#include "pwm.h"
#include "home.h"


static unsigned long timerCnt = 0;

static int init_start_pos = 0;

static void SPEED_CONTROL_LOOP();
static void GOTO_CONTROL_LOOP();
static void CheckError_TurnOff();
static void InitControlHandler();
static void InitHomeHandler();

unsigned int rising_time, falling_time;
int falling_edge_rcvd = 0;
int rising_edge = 1;

static int mv_avg_ultra_sonic = 0;
unsigned short ultra_sonic_avg;
unsigned int ultra_sonic_state = 1;
unsigned int ultra_sonic_fail_count = 0;

unsigned char LIMIT_STAT[MAX_CH] = {0,};
unsigned char LIMIT_ADDITIONAL_STAT = 0;
unsigned char ULTRA_SONIC_ERROR = 0;

int main(){
    // cpu acceleration setting -------------------
    asm volatile("di");
    CHECONbits.PFMWS = 0b011;
    CHECONbits.PREFEN = 0b01;
    register unsigned long tmp;
    asm("mfc0 %0,$16,0" :  "=r"(tmp));
    tmp = (tmp & ~7) | 3;
    asm("mtc0 %0,$16,0" :: "r" (tmp));
    // --------------------------------------------
    
    
    // apply initial delay for stable operation ---
    int initial_delay;
    for(initial_delay=0; initial_delay<100000; initial_delay++)
        ;
    for(initial_delay=0; initial_delay<100000; initial_delay++)
        ;
    volatile int initial_delay2 = 12000000;
    while(initial_delay2--)
        ;
    // --------------------------------------------
    
    // multi-vector enable
    INTCONbits.MVEC = 1;
       
    // Init Status
    STAT[0].B = 0;
    STAT[1].B = 0;
    
    
    // Initialize -----
    InitPort();
    InitBoardIdentification();
    InitPeripheral();
    
    
    //MOTOR_HIP_OFF
    GateDriver_OFF(0);
    GateDriver_OFF(1);

    asm volatile("ei");

    EEPROM_DESELECT
    
    int i, j;
    for(i=0; i<1000; ++i)
        for(j=0; j<800; ++j)
            ;   
            
    InitADC(1);
    InitTimer3();
    
            
    unsigned char canTxData[8] = {0,};
    Push_CAN_TX_Ring(900+JMC, 1, canTxData); 
        
    
    
    // BOBA BOBA
    goto_param[0].move_flag = 0;
    goto_param[0].stop_flag = 0;
    goto_param[1].move_flag = 0;
    goto_param[1].stop_flag = 0;
    
    
    // spin
    goto_param[0].fixed_a = SPIN_ACC;   // 1pulse/tick^2
    ENC_RES[0] = SPIN_ENC_RES;
    // door
    goto_param[1].fixed_a = DOOR_ACC;   // 1pulse/tick^2
    ENC_RES[1] = DOOR_ENC_RES;
    
    
//    LATBbits.LATB3 = 0;
//    LATBbits.LATB9 = 0;

    
    unsigned long while_cnt = 0;
    while(1){
        while_cnt++;
        
        // CAN Handler
        Handler_CAN_TX_Ring();
//        Handler_CAN_TX_Ring_TMR3();
                
        // Error Handler
        Handler_Error(timerCnt);
        
        // Status Handler
        Handler_Status(timerCnt);

        
        int ch;
        for(ch=0; ch<MAX_CH; ch++){
            if(InitControlFlag[ch] == 1){
                InitControlHandler(ch);            
            }else if(InitHomeFlag[ch] != 0){
                InitHomeHandler(ch);
            }
        }
        
        // EEPROM Handler
        if(while_cnt%20 == 0){
            Handler_EEPROM_Ring();
        }
        
        
        Handler_ADC_Trigger(timerCnt);
                
        
        
        if(timerCnt % 200 == 0){
            LATAbits.LATA12 ^= 1;
        }
        // --------------------------------------------
    }
}


static int InputCaptureCount = 0;
static int glitch_count = 0;

unsigned int t3_cnt = 0;
void __ISR(_TIMER_3_VECTOR, IPL6AUTO) Handler_TMR_3(){
    while(!ADCDSTAT1bits.ARDY0)
        ;
    while(!ADCDSTAT1bits.ARDY1)
        ;
    
    ++t3_cnt;
    UpdateEncoder();

    
    int ch = 0;
    for(ch=0; ch<MAX_CH; ch++){
        switch(CONTROL_MODE[ch]){

        case CONTROL_MODE_FOC_POS_CONTROL:
            if(t3_cnt%40 == 0){
                if(STAT[ch].b.RUN == 1){
                    GO_POS_CNT[ch]++;
                }
                if(GO_POS_CNT[ch] > 2){
                    SPEED_CONTROL_LOOP(ch);
                    GOTO_CONTROL_LOOP(ch);
                    Handler_Control(ch);
                }
            }
            FOC_CurrentControl_Double(ch);        
            break;

        case CONTROL_MODE_FOC_HOME_CONTROL:
            if((InitHomeFlag[ch] == 1) && (InitHomeState[ch] == 2)){
                // wait home sensor undetected
                if(ch == 0){
                    if(HOME_SENSOR1 == 1){
                        speed_test_start[ch] = 2;
                        InitHomeState[ch] = 3;
                    }
                }else{
                    if(HOME_SENSOR2 == 0){
                        speed_test_start[ch] = 2;
                        InitHomeState[ch] = 3;
                    }
                }
            }else if((InitHomeFlag[ch] == 1) && (InitHomeState[ch] == 4)){
                // wait home sensor detected
                if(ch == 0){
                    if(HOME_SENSOR1 == 0){
                        InitHomeOffset[ch] = currentMotorPosition[ch];
                        speed_test_start[ch] = 2;
                        InitHomeState[ch] = 5;
                    }
                }else{
                    if(HOME_SENSOR2 == 1){
                        InitHomeOffset[ch] = currentMotorPosition[ch];
                        speed_test_start[ch] = 2;
                        InitHomeState[ch] = 5;
                    }
                }
            }

            if(t3_cnt%40 == 0){
                SPEED_CONTROL_LOOP(ch);
                GOTO_CONTROL_LOOP(ch);
                Handler_Control(ch);
            }
            FOC_CurrentControl_Double(ch);
            break;

        case CONTROL_MODE_FOC_CUR_CONTROL:
            ref_current_q[ch] = NewInputCurrent[ch];

            FOC_CurrentControl_Double(ch);
            break;

        case CONTROL_MODE_FOC_OPENLOOP_CONTROL:
            FOC_MeasureCurrent(ch);
            FOC_OpenLoopControl(ch);            
            break;

        case CONTROL_MODE_NO_ACT:
        default:
            FOC_MeasureCurrent(ch);
            break;
        }
        
        MV_AVG_Q(ch);

        if(t3_cnt%20 == 0){ // 1kHz
            ReadEncoder(ch);
        }

        // encoder send itself (20k/2000 -> 10Hz)
        if(t3_cnt%2000 == 0){
            CMD_SendEncoder(ch);
        }
    }
    
    
    LIMIT_STAT[0] = HOME_SENSOR1;
    LIMIT_STAT[1] = HOME_SENSOR2;
    LIMIT_ADDITIONAL_STAT = ADDITIONAL_SENSOR;
        
    if(t3_cnt%20 == 0){ // 1kHz
        timerCnt++;
        if(timerCnt%1000 == 0){
            LATEbits.LATE15 ^= 1;
        }
    }
    
    
    // =========================================================================
    if(t3_cnt%2000 == 0){
        ultra_sonic_state = 1;
        ultra_sonic_fail_count++;
        if(ultra_sonic_fail_count > 30){
            ULTRA_SONIC_ERROR = 1;
        }else{
            ULTRA_SONIC_ERROR = 0;
        }
    }
    switch(ultra_sonic_state){
        case 1:
            rising_edge = 1;
            falling_edge_rcvd = 0;
            LATFbits.LATF0 = 1; // UltraSonic Trig high
            ultra_sonic_state = 2;
            break;
        case 2:
            LATFbits.LATF0 = 0; // UltraSonic Trig low
            ultra_sonic_state = 3;
            break;
        case 3:
            if(falling_edge_rcvd == 1){
                falling_edge_rcvd = 0;

                glitch_count = 0;
                ultra_sonic_state = 4;
            }
            break;
        case 4:
        {
            glitch_count++;
            if(glitch_count > 10){ // 0.5ms
                if(InputCaptureCount == 0){
                    // no glitch
                    int temptemp = 0;
                    if (falling_time > rising_time) {
                        temptemp = falling_time - rising_time;
                    }else {
                        temptemp = (PR2 - rising_time) + falling_time;
                    }

                    int ultra_sonic_data = (unsigned short)((float)(temptemp)*0.0184);
                    ultra_sonic_avg = ultra_sonic_data;
                    if(ultra_sonic_avg > 0xFF){
                        ultra_sonic_avg = 0xFF;
                    }
                    ultra_sonic_fail_count = 0;
                }else{
                    // there is glitch --> ignore data
                    ;
                }
                ultra_sonic_state = 0;
            }
            break;
        }
        default:
            break;
    }   
    // =========================================================================
   
    
    
//    CheckError_TurnOff();     // do not turn off when JAM CUR BIG INP error occur
        
    IFS0bits.T3IF=0;
}


void __ISR(_INPUT_CAPTURE_4_VECTOR, IPL5AUTO) Handler_INPUT_CAPTURE(){
    unsigned int t1;
    t1 = IC4BUF;
    
    InputCaptureCount++;
    if (rising_edge == 1) {
        rising_edge = 0;
        rising_time = t1;
    }
    else {
        rising_edge = 1;
        falling_edge_rcvd = 1;
        falling_time = t1;
        InputCaptureCount = 0;
    }
    //Clear IF
    IFS0bits.IC4IF = 0;
}


void InitHomeHandler(){
    int ch;
    for(ch=0; ch<MAX_CH; ch++){
        switch(InitHomeState[ch]){
        case 1:
            // first check the home sensor
            // if sensor on --> move back
            // if sensor off --> move forward
            if(speed_test_start[ch] != 0){
                // do nothing until motor finishes previous work
                break;
            }

            CONTROL_MODE[ch] = CONTROL_MODE_FOC_HOME_CONTROL;

            if(ch == 0){
                if(HOME_SENSOR1 == 0){
                    // move back
                    speed_test_target[ch] = -SPIN_HOME_SPD * ENC_RES[ch] * ENC_MUL / 500 / 60;   // -1000 RPM

                    if(speed_test_target[ch] > -1)
                        speed_test_target[ch] = -1;
                    speed_test_acc[ch] = fabs(speed_test_target[ch] / 500);//1000;


                    if(HOME_DIR[ch] == 0){
                        ;
                    }else{
                        speed_test_target[ch] = -speed_test_target[ch];
                    }

                    if (speed_test_acc[ch] < 1)
                        speed_test_acc[ch] = 1;
                    if(speed_test_start[ch] == 0){
                        speed_test_cur_speed[ch] = 0;
                    }
                    InitHomeState[ch] = 2;
                    speed_test_start[ch] = 1;

                }else{
                    // move forward
                    // go to case 3
                    InitHomeState[ch] = 3;
                }
            }else{
                if(HOME_SENSOR2 == 1){
                    // move back
                    speed_test_target[ch] = -DOOR_HOME_SPD * ENC_RES[ch] * ENC_MUL / 500 / 60;   // -1000 RPM

                    if(speed_test_target[ch] > -1)
                        speed_test_target[ch] = -1;
                    speed_test_acc[ch] = fabs(speed_test_target[ch] / 500);//1000;


                    if(HOME_DIR[ch] == 0){
                        ;
                    }else{
                        speed_test_target[ch] = -speed_test_target[ch];
                    }

                    if (speed_test_acc[ch] < 1)
                        speed_test_acc[ch] = 1;
                    if(speed_test_start[ch] == 0){
                        speed_test_cur_speed[ch] = 0;
                    }
                    InitHomeState[ch] = 2;
                    speed_test_start[ch] = 1;

                }else{
                    // move forward
                    // go to case 3
                    InitHomeState[ch] = 3;
                }
            }


            break;
        case 2:
            // wait until home sensor undetected
            // checked in the Timer3 and change state at there
            break;

        case 3:
            // move forward
            if(speed_test_start[ch] != 0){
                // do nothing until motor finishes previous work
                break;
            }

            if(ch == 0){
                speed_test_target[ch] = SPIN_HOME_SPD * ENC_RES[ch] * ENC_MUL / 500 / 60;   // -1000 RPM
            }else{
                speed_test_target[ch] = DOOR_HOME_SPD * ENC_RES[ch] * ENC_MUL / 500 / 60;   // -1000 RPM
            }
            if(speed_test_target[ch] < 1)
                speed_test_target[ch] = 1;
            speed_test_acc[ch] = fabs(speed_test_target[ch] / 500);//1000;

            if(HOME_DIR[ch] == 0){
                ;
            }else{
                speed_test_target[ch] = -speed_test_target[ch];
            }

            if (speed_test_acc[ch] < 1)
                speed_test_acc[ch] = 1;
            if(speed_test_start[ch] == 0){
                speed_test_cur_speed[ch] = 0;
            }
            InitHomeState[ch] = 4;
            speed_test_start[ch] = 1;
            break;

        case 4:
            // wait until home sensor detected
            // checked in the Timer3 and change state at there
            break;

        case 5:
            // move back for space
            if(speed_test_start[ch] != 0){
                // do nothing until motor finishes previous work
                break;
            }

            set_goto_start_pos(ch, currentMotorPosition[ch]);
            goto_param[ch].cur_pos = currentMotorPosition[ch];
            if(HOME_DIR[ch] == 0){
                if(ch == 0){
                    //set_goto_target(currentMotorPosition-HOME_ZERO_OFFSET*ENC_MUL, 5*ENC_MUL);     // 200 pulse/tick * 500 tick/sec = 100,000pulse/sec => 60*100,000/16384 = 366rpm
                    set_goto_target(ch, InitHomeOffset[ch]-HOME_ZERO_OFFSET[ch]*ENC_MUL, SPIN_HOME_VEL*ENC_MUL);     // 200 pulse/tick * 500 tick/sec = 100,000pulse/sec => 60*100,000/16384 = 366rpm
                }else{
                    set_goto_target(ch, InitHomeOffset[ch]-HOME_ZERO_OFFSET[ch]*ENC_MUL, DOOR_HOME_VEL*ENC_MUL);     // 200 pulse/tick * 500 tick/sec = 100,000pulse/sec => 60*100,000/16384 = 366rpm
                }
            }else{
                if(ch == 0){
                    set_goto_target(ch, InitHomeOffset[ch]+HOME_ZERO_OFFSET[ch]*ENC_MUL, SPIN_HOME_VEL*ENC_MUL);
                }else{
                    set_goto_target(ch, InitHomeOffset[ch]+HOME_ZERO_OFFSET[ch]*ENC_MUL, DOOR_HOME_VEL*ENC_MUL);
                }
            }
            InitHomeState[ch] = 6;
            break;

        case 6:
            if(goto_param[ch].move_flag == 0){
                InitHomeState[ch] = 0;
                InitHomeFlag[ch] = 0;

                currentMotorPosition[ch] = NewInputRef[ch] = REF[ch] = PREV_REF[ch] = NewPos[ch] = OldPos[ch] = 0;
                goto_param[ch].cur_pos = goto_param[ch].start_pos = currentMotorPosition[ch];

                STAT[ch].b.INIT = 1;
                CONTROL_MODE[ch] = CONTROL_MODE_FOC_POS_CONTROL;
            }
            break;
        }
    }
}



void SPEED_CONTROL_LOOP(){
    static int over_current_count[MAX_CH] = {0,};

    int ch;
    for(ch=0; ch<MAX_CH; ch++){    
        if(speed_test_start[ch] == 1){
            STAT[ch].b.OP_STATUS = 1;
            //speed_test_cur_speed += speed_test_acc;
            if(speed_test_cur_speed[ch] < speed_test_target[ch]){// >= 0){
                speed_test_cur_speed[ch] += speed_test_acc[ch];
                if(speed_test_cur_speed[ch] > speed_test_target[ch]){
                    speed_test_cur_speed[ch] = speed_test_target[ch];
                }
            }else if(speed_test_cur_speed[ch] > speed_test_target[ch]){
                speed_test_cur_speed[ch] -= speed_test_acc[ch];
                if(speed_test_cur_speed[ch] < speed_test_target[ch]){
                    speed_test_cur_speed[ch] = speed_test_target[ch];
                }
            }
            int prev_ref = REF[ch];
            int itemp = prev_ref + speed_test_cur_speed[ch];
            NewInputRef[ch] = itemp;
        }else if(speed_test_start[ch] == 2){
            STAT[ch].b.OP_STATUS = 1;
            if(speed_test_cur_speed[ch] >= 0){
                speed_test_cur_speed[ch] -= speed_test_acc[ch];
                if(speed_test_cur_speed[ch] <= 0){
                    speed_test_cur_speed[ch] = 0;
                    speed_test_start[ch] = 0;
                }
            }else{
                speed_test_cur_speed[ch] += speed_test_acc[ch];
                if(speed_test_cur_speed[ch] >= 0){
                    speed_test_cur_speed[ch] = 0;
                    speed_test_start[ch] = 0;
                }
            }
            int prev_ref;
            prev_ref = REF[ch];
            int itemp = prev_ref + speed_test_cur_speed[ch];
            NewInputRef[ch] = itemp;
        }


        if(speed_test_start[ch] != 0){
            // check current 
            if(ch == 1){
                if(abs(measured_current_q[ch]) > COLLISION_CURRENT){
                    over_current_count[ch]++;
                    if(over_current_count[ch] > COLLISION_COUNT){   // 0.3 sec
                        float new_target;
                        if(speed_test_cur_speed[ch] >= 0){
                            new_target = currentMotorPosition[ch] - COLLISION_BACK_PULSE*ENC_MUL;
                        }else{
                            new_target = currentMotorPosition[ch] + COLLISION_BACK_PULSE*ENC_MUL;
                        }
                        goto_param[ch].move_flag = 0;
                        set_goto_start_pos(ch, (float)(currentMotorPosition[ch]));
                        goto_param[ch].cur_pos = currentMotorPosition[ch];
                        NewInputRef[ch] = goto_param[ch].cur_pos;
                        set_goto_target(ch, new_target, COLLISION_BACK_SPD*ENC_MUL);

                        speed_test_start[ch] = 0;   // finish speed mode
                        if(InitHomeFlag[ch] == 1){  // finish home search
                            InitHomeFlag[ch] = 0;
                            InitHomeState[ch] = 0;
                            STAT[ch].b.INIT = 0;
                            STAT[ch].b.COLLISION_ERR = 1;
                        }
                    }
                }else{
                    over_current_count[ch] = 0;
                }
            }
        }
    }
}

void GOTO_CONTROL_LOOP(){
    static int over_current_count[MAX_CH] = {0,};
    static int do_brake[MAX_CH] = {0,};
    static float brake_spd[MAX_CH] = {0,};
    static int do_move_back[MAX_CH] = {0,};
    
    int ch = 0;
    for(ch=0; ch<MAX_CH; ch++){
        if(speed_test_start[ch] == 0){
            if(goto_param[ch].stop_flag == 1){
                goto_param[ch].stop_flag = 0;
                do_brake[ch] = 1;
                brake_spd[ch] = goto_param[ch].cur_vel;
                goto_param[ch].move_flag = 0;
            }
            if(do_brake[ch] == 1){
                STAT[ch].b.OP_STATUS = 1;
                float new_spd = 0.0;
                if(brake_spd[ch] > 0){
                    new_spd = brake_spd[ch]-goto_param[ch].fixed_a*2.0;
                    if(new_spd < 0.0){
                        new_spd = 0.0;
                        do_brake[ch] = 0;
                        STAT[ch].b.OP_STATUS = 0;
                    }
                }else{
                    new_spd = brake_spd[ch]+goto_param[ch].fixed_a*2.0;
                    if(new_spd > 0.0){
                        new_spd = 0.0;
                        do_brake[ch] = 0;
                        STAT[ch].b.OP_STATUS = 0;
                    }
                }
                brake_spd[ch] = new_spd;
                goto_param[ch].cur_pos = goto_param[ch].cur_pos + new_spd;

                NewInputRef[ch] = goto_param[ch].cur_pos;

            }else if(do_move_back[ch] == 1){
                STAT[ch].b.OP_STATUS = 1;
                do_move_back[ch] = 0;
                set_goto_start_pos(ch, (float)(currentMotorPosition[ch]));
                goto_param[ch].cur_pos = currentMotorPosition[ch];
                set_goto_target(ch, OpenPositionPulse[ch]*ENC_MUL, COLLISION_BACK_SPD*ENC_MUL);

            }else if(goto_param[ch].move_flag == 1){
                STAT[ch].b.OP_STATUS = 1;
                move_goto_target(ch);
                NewInputRef[ch] = goto_param[ch].cur_pos;

                if(ch == 1){
                    // collision 
                    if(STAT[ch].b.COLLISION_ERR == 0){

                        // check error flag (JAM BIG CUR INP)
                        if((STAT[ch].B & 0x00000F00) != 0){
                            STAT[ch].b.COLLISION_ERR = 1;
                            do_move_back[ch] = 1;
                            goto_param[ch].move_flag = 0;
                        }

                        // check mismatch pulse
                        if(goto_param[ch].move_flag == 0){
                            int prev_target = (int)(goto_param[ch].target_pos);
                            if(abs(prev_target - currentMotorPosition[ch]) > COLLISION_MISMATCH_PULSE*ENC_MUL){
                                STAT[ch].b.COLLISION_ERR = 1;
                                do_move_back[ch] = 1;
                                goto_param[ch].move_flag = 0;
                            }
                        }


                        // check current 
                        if(abs(measured_current_q[ch]) > COLLISION_CURRENT){
                            over_current_count[ch]++;
                            if(over_current_count[ch] > COLLISION_COUNT){   // 0.2 sec
                                STAT[ch].b.COLLISION_ERR = 1;
                                do_move_back[ch] = 1;
                                goto_param[ch].move_flag = 0;
                                over_current_count[ch] = 0;
                            }
                        }else{
                            over_current_count[ch] = 0;
                        }
                    }
                }
            }else if(override_flag[ch] == 1){
                override_flag[ch] = 0;
                set_goto_start_pos(ch, (float)(currentMotorPosition[ch]));
                goto_param[ch].cur_pos = currentMotorPosition[ch];
                set_goto_target(ch, override_target[ch]*ENC_MUL, override_speed[ch]*ENC_MUL);
            }else{
                STAT[ch].b.OP_STATUS = 0;
            }
        }else{
            goto_param[ch].cur_pos = NewInputRef[ch];
        }
    }
}

void CheckError_TurnOff(){
    int ch = 0;
    for(ch=0; ch<MAX_CH; ch++){
        if(((STAT[ch].B & 0x00000F00) != 0)){// || (STAT.b.CUR_BIG == 1)){// || (STAT.b.CAN_ERR == 1)){
            if(ERROR_FLAG[ch] == 0){
                ERROR_FLAG[ch] = 1;
                GateDriver_OFF(ch);
                CONTROL_MODE[ch] = CONTROL_MODE_NO_ACT;
            }
        }
    }
}

void InitControlHandler(){
    int ch = 0;
    for(ch=0; ch<MAX_CH; ch++){
        switch(InitControlState[ch]){
        case 1:
            STAT[ch].b.COLLISION_ERR = 0;
            STAT[ch].b.BIG = 0;
            STAT[ch].b.JAM = 0;
            STAT[ch].b.CUR = 0;
            STAT[ch].b.INP = 0;
            STAT[ch].b.INIT = 0;
            GateDriver_OFF(ch);
            CONTROL_MODE[ch] = CONTROL_MODE_NO_ACT;

            InitControlState[ch] = 2;
            break;

        case 2: 
            CMD_FetEnable(ch, 1);
            CMD_RunClosedLoop(ch, 1, 0);

            InitControlState[ch] = 3;
            break;

        case 3:
            InitHomeState[ch] = 1;
            InitHomeFlag[ch] = 1;

            InitControlState[ch] = 0;
            InitControlFlag[ch] = 0;
            break;   
        }
    }
}
