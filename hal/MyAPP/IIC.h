#ifndef __IIC_H
#define __IIC_H

#include "main.h"

void IIC_GPIO_Init(void);

void IIC_Start(void);
void IIC_Stop(void);

void IIC_Send_Byte(uint8_t txd);
uint8_t IIC_Read_Byte(uint8_t ack);

uint8_t IIC_Wait_Ack(void);

void IIC_Ack(void);
void IIC_NAck(void);

/* 给MPU/DMP使用的阻塞延时 */
void IIC_DelayMs(uint32_t ms);

#endif

