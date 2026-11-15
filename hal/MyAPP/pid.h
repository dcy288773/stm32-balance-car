#ifndef __PID_H
#define __PID_H

#include "main.h"

void Control(void);

int Vertical(float Med,
             float Angle,
             float gyro_Y);

float Velocity(float Target,
               float encoder_left,
               float encoder_right);

int Turn(int gyro_Z,
         int RC);

extern float Pitch;
extern float Roll;
extern float Yaw;

extern short gyrox;
extern short gyroy;
extern short gyroz;

extern int Encoder_Left;
extern int Encoder_Right;
extern float Encoder_Left_10ms;
extern float Encoder_Right_10ms;

extern int Vertical_out;
extern float Velocity_out;
extern int Turn_out;

extern int MOTO1;
extern int MOTO2;

extern volatile uint32_t Control_Count;
extern volatile uint32_t Control_DMP_ErrorCount;
extern volatile uint32_t Control_DMP_CatchupPackets;
extern volatile uint32_t Control_DMP_BacklogResets;
extern volatile uint32_t Control_MaxCycles;
extern volatile uint32_t Control_OverrunCount;

#endif
