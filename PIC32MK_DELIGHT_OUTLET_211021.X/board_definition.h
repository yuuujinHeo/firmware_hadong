#ifndef __BOARD_DEFINITION_H__
#define __BOARD_DEFINITION_H__




// Version Information---------------
//#define VERSION     1210705
//#define VERSION     1210802     // collision add
//#define VERSION     1210809     // BIG INP JAM CUR error --> non stop  ||  goto function override
//#define VERSION     1210812     // sonar sensor glitch protection & error code
#define VERSION     1210818     // light sensor disable
//-----------------------------------



#define DO_NOT_USE_LIGHT_SENSOR


// Setting Parameter -------------------------
// pulse/tick
#define SPIN_ACC        200.0   //20.0
#define DOOR_ACC        200.0   //20.0    //30.0

// rpm
#define SPIN_HOME_SPD   1200    //600 //2000
#define DOOR_HOME_SPD   1000    //500 //1000    

// encoder res
#define SPIN_ENC_RES 1024
#define DOOR_ENC_RES 1024//1311

// home back velocity
#define SPIN_HOME_VEL   20.0    //5.0
#define DOOR_HOME_VEL   10.0     //1.0//20.0


// collision
#define COLLISION_CURRENT           700    // 700mA
#define COLLISION_COUNT             150     // 0.3sec
#define COLLISION_MISMATCH_PULSE    1500    //250    // 250pulse/4800pulse*170mm = 9mm    10000     // 10000pulse   
#define COLLISION_BACK_PULSE        2500    //500    // 20000.0    // 20000pulse
#define COLLISION_BACK_SPD          15.0    //3.0       //45.0
// -------------------------------------------





#define ERS_max             2000
#define MAX_CUR_LIMIT       3000

#define ENC_MUL     10  //100


// I * R * G = V   
// I = V/R/G
// (3.3/4096) / 0.02 / 20 = 2.01416 mA/digit
#define CURRENT_CONSTANT    2.01416

#define HOME_SENSOR1    !(PORTBbits.RB4)  //!PORTGbits.RG8 //!PORTCbits.RC10 // Idle High
#define HOME_SENSOR2    !(PORTDbits.RD5)
#define ADDITIONAL_SENSOR   !(PORTDbits.RD6)


typedef enum{
    BNO_SPIN_1 = 0,
    BNO_DOOR_1,
    BNO_SPIN_2,
    BNO_DOOR_2
}BNO_LIST;



#define MAX_CH      2

#define PI          3.14159265359
#define R2D         57.29577951
#define D2R         0.017453292


#define ENABLE			1
#define DISABLE			0

#define TRUE    1
#define FALSE   0



#define	CTRL_MOD_POSITION		0
#define	CTRL_MOD_CURRENT		1


#define MOTOR_HIP_ON1    LATAbits.LATA11 = 1;
#define MOTOR_HIP_OFF1   LATAbits.LATA11 = 0;
#define MOTOR_HIP_STAT1  PORTAbits.RA11
#define MOTOR_HIP_ON2    LATGbits.LATG8 = 1;
#define MOTOR_HIP_OFF2   LATGbits.LATG8 = 0;
#define MOTOR_HIP_STAT2  PORTGbits.RG8
    

// CONTROL MODE--------------------
#define CONTROL_MODE_NO_ACT                 0
#define CONTROL_MODE_FOC_OPENLOOP_CONTROL   1
#define CONTROL_MODE_FOC_CUR_CONTROL        2
#define CONTROL_MODE_FOC_POS_CONTROL        3
#define CONTROL_MODE_FOC_HOME_CONTROL       4
// --------------------------------


typedef union{
	struct {
		unsigned    FET:1;	 	// FET ON
		unsigned    RUN:1;		// Control ON
        unsigned    INIT:1;     // Init Process Passed
		unsigned    MOD:1;		// Control Mode
        unsigned    NON_CTR:1;  // Nonius Error
		unsigned 	OP_STATUS:1; // Operating Status  - 1: moving  - 0: idle
        unsigned    CALIB:1;    // Calibration Mode
        unsigned    COLLISION_ERR:1;   // Collision Error
    	
    	unsigned 	JAM:1;		// JAM Error
      	unsigned 	CUR:1;		// Over Current Error
		unsigned    BIG:1;		// Big Position Error		
		unsigned    INP:1;	    // Big Input Error
		unsigned 	FLT:1;		// FET Driver Fault Error
		unsigned    TMP:1;	    // Temperature Error		
		unsigned	PS1:1;		// Position Limit Error (Lower)
		unsigned    PS2:1;		// Position Limit Error (Upper)
		
        unsigned    SPI_EXT:1;  // External SPI
		unsigned    CUR_BIG:1;  // Current Big
        unsigned    CAN_ERR:1;  // CAN_ERR in Current Mode
		unsigned	rsvd:13;
		
	} b;
	unsigned int B;
} mSTAT;



#endif
