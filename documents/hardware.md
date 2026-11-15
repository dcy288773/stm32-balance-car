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
| 超声波 | HC-SR04 | 1（仅 `std/` 版驱动） |
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

| 功能 | 引脚 | 说明 | 版本差异 |
|---|---|---|---|
| MPU6050 SCL / SDA | PB3 / PB4 | 软件 I2C | 两版相同 |
| MPU6050 INT | PB5 | EXTI_Line5，下降沿触发 | `std/` 用它触发控制中断；**`hal/` 未使用**（改用 TIM3 200Hz 定时中断） |
| 超声波 TRIG / ECHO | PA3 / PA2 | TIM3 计时 | **仅 `std/` 驱动**；`hal/` 未实现 |
| OLED SCL / SDA | PB8 / PB9 | I2C | `std/` 软件 I2C；`hal/` 硬件 I2C1（I2C1 重映射到 PB8/PB9） |
| 蓝牙 TX / RX | PB10 / PB11 | USART3，9600 | 两版相同 |
| 调试串口 TX / RX | PA9 / PA10 | USART1，115200 | `std/` 未启用；`hal/` 已初始化但当前未使用 |

## 下载方式

两版初始化时都会禁用 JTAG 以释放 PB3/PB4 作 MPU6050 的软件 I2C：

- 标准库版：`GPIO_Remap_SWJ_JTAGDisable`
- HAL 版：`__HAL_AFIO_REMAP_SWJ_NOJTAG()`

必须使用 **SWD** 下载（SWDIO、SWCLK 两根线），不能用 JTAG。
