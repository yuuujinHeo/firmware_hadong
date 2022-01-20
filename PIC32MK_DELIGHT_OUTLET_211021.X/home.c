#include <math.h>
#include "home.h"
#include "qei.h"


GOTO_ST goto_param[MAX_CH];



void set_goto_start_pos(int ch, float st_pos){
    goto_param[ch].start_pos = st_pos;
}

void set_goto_target(int ch, float target, float vel){  // pulse, pulse/tick  a = pulse/tick^2
    if(goto_param[ch].move_flag == 1)
        return;
    
    if(vel < 5.0){
        vel = 5.0;
    }
    goto_param[ch].target_pos = target;
    goto_param[ch].delta_pos = goto_param[ch].target_pos - goto_param[ch].start_pos;
    
    if(vel*vel/goto_param[ch].fixed_a < fabs(goto_param[ch].delta_pos)){
        // normal
        goto_param[ch].mode = 0;
        goto_param[ch].t1 = vel/goto_param[ch].fixed_a;
        goto_param[ch].t2 = fabs(goto_param[ch].delta_pos)/vel;
        goto_param[ch].t3 = goto_param[ch].t1+goto_param[ch].t2;
    }else{
        // triangle
        goto_param[ch].mode = 1;
        goto_param[ch].t1 = sqrt(fabs(goto_param[ch].delta_pos)/goto_param[ch].fixed_a);
        goto_param[ch].t2 = goto_param[ch].t1*2;
    }
    goto_param[ch].cur_cnt = 0;
    goto_param[ch].move_flag = 1;
    goto_param[ch].cur_vel = 0.0;
}
void move_goto_target(int ch){
    if(goto_param[ch].move_flag == 1){
        goto_param[ch].cur_cnt = goto_param[ch].cur_cnt + 1.0;
    
        float dir = 1.0;
        if(goto_param[ch].delta_pos > 0){
            dir = 1.0;
        }else{
            dir = -1.0;
        }    
        float prev_pos = goto_param[ch].cur_pos;
        
        if(goto_param[ch].mode == 0){
            // normal
            float vel = goto_param[ch].fixed_a * goto_param[ch].t1;
            if(goto_param[ch].cur_cnt < goto_param[ch].t1){
                goto_param[ch].cur_pos = goto_param[ch].start_pos + dir*(0.5*goto_param[ch].fixed_a*goto_param[ch].cur_cnt*goto_param[ch].cur_cnt);
            }else if(goto_param[ch].cur_cnt < goto_param[ch].t2){
                float t = goto_param[ch].cur_cnt - goto_param[ch].t1;
                goto_param[ch].cur_pos = goto_param[ch].start_pos + dir*(0.5*goto_param[ch].fixed_a*goto_param[ch].t1*goto_param[ch].t1 + vel*t);
            }else if(goto_param[ch].cur_cnt < goto_param[ch].t3){
                float t = goto_param[ch].cur_cnt - goto_param[ch].t2;
                goto_param[ch].cur_pos = goto_param[ch].start_pos + dir*(0.5*goto_param[ch].fixed_a*goto_param[ch].t1*goto_param[ch].t1 + vel*(goto_param[ch].t2-goto_param[ch].t1) + vel*t - 0.5*goto_param[ch].fixed_a*t*t);
            }else{
                goto_param[ch].cur_pos = goto_param[ch].target_pos;
                goto_param[ch].cur_cnt = 0;
                goto_param[ch].move_flag = 0;
            }
        }else{
            // triangle
            float vel = goto_param[ch].fixed_a * goto_param[ch].t1;
            if(goto_param[ch].cur_cnt < goto_param[ch].t1){
                goto_param[ch].cur_pos = goto_param[ch].start_pos + dir*(0.5*goto_param[ch].fixed_a*goto_param[ch].cur_cnt*goto_param[ch].cur_cnt);
            }else if(goto_param[ch].cur_cnt < goto_param[ch].t2){
                float t = goto_param[ch].cur_cnt - goto_param[ch].t1;
                goto_param[ch].cur_pos = goto_param[ch].start_pos + dir*(0.5*goto_param[ch].fixed_a*goto_param[ch].t1*goto_param[ch].t1 + vel*t - 0.5*goto_param[ch].fixed_a*t*t);
            }else{
                goto_param[ch].cur_pos = goto_param[ch].target_pos;
                goto_param[ch].cur_cnt = 0;
                goto_param[ch].move_flag = 0;
            }
        }
        
        goto_param[ch].cur_vel = goto_param[ch].cur_pos - prev_pos;
    }
}