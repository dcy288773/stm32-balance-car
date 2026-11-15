#include "motor.h"

#define PWM_MAX   7200
#define PWM_MIN  -7200

char stop = 0;

int GFP_abs(int p)
{
    return (p > 0) ? p : -p;
}

void Limit(int *a, int *b)
{
    if (*a > PWM_MAX)  *a = PWM_MAX;
    if (*a < PWM_MIN)  *a = PWM_MIN;
    if (*b > PWM_MAX)  *b = PWM_MAX;
    if (*b < PWM_MIN)  *b = PWM_MIN;
}

void Load(int moto1, int moto2)
{
    if (moto1 > 0)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
    }
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, GFP_abs(moto1));

    if (moto2 > 0)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    }
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, GFP_abs(moto2));
}

void Stop(float *Med, float *Angle)
{
    if (GFP_abs((int)(*Angle - *Med)) > 60)
    {
        Load(0, 0);
        stop = 1;
    }
}
