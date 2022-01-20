
#ifndef __HOME_H__
#define __HOME_H__

#include "board_definition.h"

typedef struct _GOTO_ST_{
    float start_pos;
    float target_pos;
    float delta_pos;
    float cur_pos;
    
    float fixed_a;
    
    int mode;
    float cur_cnt;
    float t1;
    float t2;
    float t3;
    
    float cur_vel;
    
    int move_flag;
    int stop_flag;
}GOTO_ST;


extern GOTO_ST goto_param[MAX_CH];


extern void set_goto_start_pos(int ch, float st_pos);
extern void set_goto_target(int ch, float target, float vel);
extern void move_goto_target(int ch);

#endif