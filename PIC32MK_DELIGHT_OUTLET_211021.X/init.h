#ifndef __INIT_H__
#define __INIT_H__

#include "board_definition.h"

#define DEFAULT_TEMP_ERROR          60

#define DEFAULT_ENC_RES             1024
#define DEFAULT_POLE_NUM            5
#define DEFAULT_MOTOR_DIR           1

#define DEFAULT_POS_LOWER_LIMIT     -2114447    // 460degree
#define DEFAULT_POS_UPPER_LIMIT     2114447     // 460degree

#define DEFAULT_CURRENT_NULL_RZ     2048
#define DEFAULT_CURRENT_NULL_SZ     2048
#define DEFAULT_CURRENT_NULL_TZ     2048


#define DEFAULT_FOC_PGAIN           17000
#define DEFAULT_FOC_IGAIN           100

#define DEFAULT_SPIN_POS_PGAIN           200
#define DEFAULT_SPIN_POS_IGAIN           0
#define DEFAULT_SPIN_POS_DGAIN           2500

#define DEFAULT_DOOR_POS_PGAIN           200
#define DEFAULT_DOOR_POS_IGAIN           0
#define DEFAULT_DOOR_POS_DGAIN           2500

#define DEFAULT_JAM_LIMIT_CURRENT   3500    // 3.5A
#define DEFAULT_OVR_LIMIT_CURRENT   4000    // 4A
#define DEFAULT_JAM_LIMIT_TIME      10      // 1sec
#define DEFAULT_OVR_LIMIT_TIME      10      // 1sec
#define DEFAULT_INPUT_ERROR         3000
#define DEFAULT_BIG_ERROR           10000



extern void InitROM(int ch, int mode);  // 0:directly   1:push to buffer

extern void InitBoardIdentification();
extern void InitPeripheral();



#endif