#ifndef __MOTOR_H
#define __MOTOR_H

#include "main.h"
#include "tim.h"

extern char stop;

void Load(int moto1, int moto2);
void Limit(int *motoA, int *motoB);
int  GFP_abs(int p);
void Stop(float *Med, float *Angle);

#endif
