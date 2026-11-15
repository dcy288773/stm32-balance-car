#include "encoder.h"

/* 读一次计数并清零，返回"上次读到现在的增量"，带符号 */
int Read_Speed(TIM_HandleTypeDef *htim)
{
	  /*强转 short: 反转时计数会从 0 绕回 65535, 转成有符号才能正确变成负数 */
	  /*相当于量程从0 ~ 65535到-32768 ~ +32768*/
    int v = (short)__HAL_TIM_GET_COUNTER(htim); 
	  /* 清零, 下次读到的就是这一小段的增量 */
    __HAL_TIM_SET_COUNTER(htim, 0);
    return v;
}
