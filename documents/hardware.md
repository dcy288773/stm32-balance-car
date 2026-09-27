# 硬件清单与引脚接线

两版实现共用同一套硬件与接线。

## 物料清单

| 模块 | 规格 | 数量 |
|---|---|---|
| 主控 | STM32F103C8T6 最小系统板 | 1 |
| 姿态传感器 | MPU6050 模块（带 INT 引脚） | 1 |
| 电机 | 520 减速电机（带 AB 相霍尔编码器） | 2 |
| 电机驱动 | TB6612FNG 双路驱动 | 1 |
| 显示屏 | 0.96 寸 OLED（I2C / SSD1306） | 1 |
| 超声波 | HC-SR04 | 1 |
| 蓝牙 | HC-05 / HC-06 | 1 |
| 电源 | 18650 电池组 7.4V | 1 |

电机与主控分开供电，两者必须共地。两个电机左右对称安装。

## 电机与编码器

| 功能 | 引脚 | 说明 |
|---|---|---|
| 左轮 PWM | PA8 | TIM1_CH1 |
| 右轮 PWM | PA11 | TIM1_CH4 |
| 左轮方向 AIN1 / AIN2 | PB14 / PB15 | 推挽输出 |
| 右轮方向 BIN1 / BIN2 | PB13 / PB12 | 推挽输出 |
| 左轮编码器 A/B | PA0 / PA1 | TIM2 编码器模式 |
| 右轮编码器 A/B | PB6 / PB7 | TIM4 编码器模式 |

## 传感器与外设

| 功能 | 引脚 | 说明 |
|---|---|---|
| MPU6050 SCL / SDA | PB3 / PB4 | 软件 I2C |
| MPU6050 INT | PB5 | EXTI_Line5，下降沿触发 |
| 超声波 TRIG / ECHO | PA3 / PA2 | TIM3 计时 |
| OLED SCL / SDA | PB8 / PB9 | 软件 I2C |
| 蓝牙 TX / RX | PB10 / PB11 | USART3，9600 |
| 调试串口 TX / RX | PA9 / PA10 | USART1，115200 |

## 下载方式

OLED 初始化执行了 `GPIO_Remap_SWJ_JTAGDisable`，禁用 JTAG 释放 PB3/PB4。
必须使用 **SWD** 下载（SWDIO、SWCLK 两根线），不能用 JTAG。
