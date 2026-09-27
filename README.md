# 🤖 两轮自平衡小车

基于 **STM32F103C8T6** 的两轮自平衡小车。MPU6050 姿态解算 + 三闭环 PID 控制，上电即站立。

![MCU](https://img.shields.io/badge/MCU-STM32F103C8T6-0769AD?logo=stmicroelectronics)
![IDE](https://img.shields.io/badge/IDE-Keil%20MDK-orange)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## ✨ 功能特性

- 上电自主站立，被推后自行恢复
- 三闭环串级 PID：直立环 PD + 速度环 PI + 转向环 PD，5ms 控制周期
- MPU6050 内置 DMP 硬件解算姿态，INT 引脚触发中断，周期精确
- 编码器使用定时器编码器接口模式，硬件自动计数，不占用 CPU
- OLED 实时显示车身角度、超声波距离、车轮转速
- 倾角超过 60° 自动切断电机输出，保护硬件

---

## 🛠 硬件要求

| 模块 | 型号 | 说明 |
|:---|:---|:---|
| 主控 | STM32F103C8T6 | 最小系统板 |
| 姿态传感器 | MPU6050 | 需带 INT 引脚，3.3V 供电 |
| 电机 | 520 减速电机 ×2 | 带 AB 相霍尔编码器，左右对称安装 |
| 电机驱动 | TB6612FNG | 双路 H 桥 |
| 显示 | 0.96 寸 OLED | SSD1306，I2C 接口 |
| 测距 | HC-SR04 | 超声波模块 |
| 蓝牙 | HC-05 / HC-06 | 串口透传 |
| 电源 | 18650 电池组 7.4V | 电机与主控分开供电，**必须共地** |

---

## 🔌 硬件连接

| STM32 引脚 | 外设 | 功能 |
|:---|:---|:---|
| `PA8` / `PA11` | 左轮 / 右轮 | PWM（TIM1_CH1 / CH4） |
| `PB14` `PB15` | 左轮 | 方向控制 AIN1 / AIN2 |
| `PB13` `PB12` | 右轮 | 方向控制 BIN1 / BIN2 |
| `PA0` `PA1` | 左轮编码器 | TIM2 编码器接口 |
| `PB6` `PB7` | 右轮编码器 | TIM4 编码器接口 |
| `PB3` / `PB4` | MPU6050 | 软件 I2C SCL / SDA |
| `PB5` | MPU6050 INT | EXTI_Line5，5ms 控制中断 |
| `PA3` / `PA2` | HC-SR04 | TRIG / ECHO |
| `PB8` / `PB9` | OLED | 软件 I2C SCL / SDA |
| `PB10` / `PB11` | 蓝牙 | USART3，9600 |
| `PA9` / `PA10` | 调试串口 | USART1，115200 |

完整物料清单见 [`documents/hardware.md`](documents/hardware.md)。

> ⚠️ OLED 初始化禁用了 JTAG 以释放 PB3/PB4，下载必须使用 **SWD**。

---

## 🚀 快速开始

**软件环境**

- Keil MDK-ARM（uVision5）
- STM32F1 器件支持包（`Keil::STM32F1xx_DFP`）
- ST-Link 下载器

**获取代码**

```bash
git clone https://github.com/dcy288773/stm32-balance-car.git
```

**编译下载**

1. 打开 `std/USER/UpStanding_Car.uvprojx`
2. 按 **F7** 编译，应显示 `0 Error(s)`
3. 按 **F8** 下载到开发板
4. 上电，小车自行站立

不想编译可直接烧录 `std/firmware/UpStanding_Car.hex`。

---

## 📊 PID 参数

参数集中在 `std/HARDWARE/CONTROL/control.c` 顶部。

| 参数 | 值 | 环 | 作用 |
|:---|---:|:---|:---|
| `Med_Angle` | `0` | — | 机械中值，**最先调** |
| `Vertical_Kp` | `-400` | 直立环 | 太小站不起来 |
| `Vertical_Kd` | `-1.92` | 直立环 | 抑制抖动 |
| `Velocity_Kp` | `-0.44` | 速度环 | 防止持续跑偏 |
| `Velocity_Ki` | `-0.0022` | 速度环 | 消除稳态误差 |
| `Turn_Kp` | `20` | 转向环 | 走直线 |
| `Turn_Kd` | `0.6` | 转向环 | 抑制转向抖动 |

**调参顺序**：机械中值 → 直立环 Kp → 直立环 Kd → 速度环 Kp/Ki → 转向环 Kp/Kd。顺序不能跳。

详细步骤见 [`documents/tuning.md`](documents/tuning.md)。

---

## 📁 项目结构

```
stm32-balance-car/
├── documents/      硬件清单与调参说明
├── std/            ✅ 标准外设库版（已完成）
│   ├── CORE/       CMSIS 内核与启动文件
│   ├── STM32F10x_FWLib/   ST 标准外设库
│   ├── SYSTEM/     delay / sys / usart
│   ├── HARDWARE/   CONTROL ENCODER MOTOR PWM EXTI
│   │               MPU6050 OLED SENSOR USART3
│   ├── USER/       main.c 与 Keil 工程
│   └── firmware/   已编译固件
└── hal/            ⏳ HAL 库版（未开始）
```

两种实现共用同一套硬件与调参方法，文档放在 `documents/` 只维护一份。

---

## 🙏 致谢

第三方代码保持各自原始版权声明：`std/STM32F10x_FWLib/` 与 `std/CORE/`（STMicroelectronics / ARM CMSIS）、`std/HARDWARE/MPU6050/eMPL/`（InvenSense Motion Driver）、`std/SYSTEM/`（国内 STM32 教学资料通用写法）。仅供学习研究。权利人不希望被引用时请提 Issue，立即移除。

---

## 📄 许可

本人编写与整理的部分采用 **MIT License**，详见 [LICENSE](LICENSE)。
