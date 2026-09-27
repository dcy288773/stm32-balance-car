# 🤖 STM32 两轮自平衡小车

> 一辆会自己站起来的小车 —— 基于 **STM32F103C8T6**，三闭环串级 PID 控制，上电即站立。

![MCU](https://img.shields.io/badge/MCU-STM32F103C8T6-0769AD?logo=stmicroelectronics)
![Library](https://img.shields.io/badge/Library-StdPeriph%20%2B%20HAL-00979D)
![IDE](https://img.shields.io/badge/IDE-Keil%20MDK-orange)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## ✨ 它能做什么

| 功能 | 状态 | 说明 |
|:---|:---:|:---|
| 🧍 **自主站立** | ✅ | 上电后自行找平衡，无需外力扶持 |
| ⚖️ **抗扰动** | ✅ | 被轻推后能自行恢复直立 |
| 🔄 **原地转向** | ✅ | 转向环控制偏航角速度 |
| 📺 **实时数据显示** | ✅ | OLED 显示车身角度、距离、轮速 |
| 📏 **超声波测距** | ✅ | HC-SR04，实时测距并上屏 |
| 📱 **蓝牙串口** | ✅ | USART3 透传，可接手机调试 |
| 🎮 **蓝牙遥控** | ⏳ | 开发中 |
| 🚧 **自动避障** | ⏳ | 开发中 |

---

## 🧠 控制思路

这是整个项目最核心的部分，5ms 一个周期，全部在中断里完成：

```
MPU6050 INT 引脚每 5ms 产生一次下降沿
        ↓
EXTI9_5_IRQHandler 中断服务函数
        ↓
  ① 读 DMP 解算的 Pitch 角 + 陀螺仪角速度
  ② 读 TIM2/TIM4 编码器计数值（左右轮速度）
        ↓
  ┌─────────────────────────────────┐
  │  直立环 PD  → Kp=-400, Kd=-1.92 │  ← 让车不倒
  │  速度环 PI  → Kp=-0.44, Ki=-0.0022 │ ← 让车不跑偏
  │  转向环 PD  → Kp=20,  Kd=0.6    │  ← 让车走直线
  └─────────────────────────────────┘
        ↓ 三环输出相加
  限幅 ±7200（对应 PWM 满占空比）
        ↓
  写入 TIM1_CH1 / TIM1_CH4 占空比 → 电机动作
```

### 🔑 三个关键设计

**① 用 DMP 硬件解算姿态，不自己写滤波**
MPU6050 内部的 DMP（数字运动处理器）直接输出四元数并解算成欧拉角，省掉了卡尔曼/互补滤波的代码量和调参工作，角度稳定且几乎不占 MCU 资源。

**② 编码器用定时器的编码器接口模式，硬件自动计数**
TIM2 和 TIM4 配置成 `TIM_EncoderMode_TI12`，AB 相脉冲由硬件自动加减计数，CPU 只在中断里读一次计数值再清零，不占用任何轮询时间。

**③ 控制放在中断里，主循环只管显示**
5ms 的定时由 MPU6050 的 INT 引脚硬件触发，周期精确不受主循环影响；`main` 的 `while(1)` 只负责刷新 OLED，互不干扰。

---

## 🚀 快速开始

### 方式一：直接烧录固件（1 分钟）

1. 下载 `std/firmware/UpStanding_Car.hex`
2. 用 ST-Link Utility / FlyMcu / STM32CubeProgrammer 烧录
3. 上电，小车自己站起来

### 方式二：自己编译（推荐）

1. 安装 **Keil MDK-ARM** 并确保已装 **STM32F1 器件支持包**（`Keil::STM32F1xx_DFP`）
2. 打开 `std/USER/UpStanding_Car.uvprojx`
3. 按 **F7** 编译 → 应显示 `0 Error(s)`
4. 按 **F8** 下载 → 上电即可

> ⚠️ 必须用 **SWD** 下载。工程为释放 PB3/PB4 禁用了 JTAG。

---

## 🧰 硬件与接线

| 模块 | 规格 |
|:---|:---|
| 主控 | STM32F103C8T6 最小系统板 |
| 姿态传感器 | MPU6050（带 INT 引脚） |
| 电机 | 520 减速电机 ×2（带 AB 相霍尔编码器） |
| 电机驱动 | TB6612FNG 双路 |
| 显示 | 0.96 寸 OLED（I2C / SSD1306） |
| 测距 | HC-SR04 超声波 |
| 蓝牙 | HC-05 / HC-06 |
| 电源 | 18650 电池组 7.4V（电机与主控分开供电，**必须共地**） |

**关键引脚**

| 功能 | 引脚 |
|:---|:---|
| 左轮 PWM / 右轮 PWM | `PA8` / `PA11`（TIM1_CH1 / CH4） |
| 左轮方向 / 右轮方向 | `PB14+PB15` / `PB13+PB12` |
| 左轮编码器 / 右轮编码器 | `PA0+PA1`（TIM2）/ `PB6+PB7`（TIM4） |
| MPU6050 SCL / SDA / INT | `PB3` / `PB4` / `PB5` |
| 超声波 TRIG / ECHO | `PA3` / `PA2` |
| OLED SCL / SDA | `PB8` / `PB9` |
| 蓝牙 TX / RX | `PB10` / `PB11`（9600） |
| 调试串口 TX / RX | `PA9` / `PA10`（115200） |

完整物料清单与接线说明见 [`documents/hardware.md`](documents/hardware.md)。

---

## 🎛️ PID 调参

参数全部集中在 `std/HARDWARE/CONTROL/control.c` 顶部。

| 参数 | 值 | 作用 |
|:---|---:|:---|
| `Med_Angle` | `0` | **机械中值**，先调这个 |
| `Vertical_Kp` | `-400` | 直立环比例，太小站不起来 |
| `Vertical_Kd` | `-1.92` | 直立环微分，抑制抖动 |
| `Velocity_Kp` | `-0.44` | 速度环比例，防止持续跑偏 |
| `Velocity_Ki` | `-0.0022` | 速度环积分，消除稳态误差 |
| `Turn_Kp` | `20` | 转向环比例 |
| `Turn_Kd` | `0.6` | 转向环微分 |

**调参顺序不能跳步**：机械中值 → 直立环 Kp → 直立环 Kd → 速度环 Kp/Ki → 转向环 Kp/Kd。

机械中值填错，后面全部白调。详细步骤见 [`documents/tuning.md`](documents/tuning.md)。

---

## 📁 项目结构

```
stm32-balance-car/
├── 📄 README.md          你正在看的这个文件
├── 📂 documents/         共用文档
│   ├── hardware.md       物料清单 + 完整引脚接线表
│   └── tuning.md         PID 参数说明与调参步骤
├── 📂 std/               ✅ 标准外设库版（已完成）
│   ├── CORE/             CMSIS 内核与启动文件
│   ├── STM32F10x_FWLib/  ST 标准外设库 V3.5
│   ├── SYSTEM/           delay / sys / usart
│   ├── HARDWARE/         CONTROL ENCODER MOTOR PWM EXTI
│   │                     MPU6050 OLED SENSOR USART3
│   ├── USER/             main.c + Keil 工程
│   └── firmware/         已编译固件，可直接烧录
└── 📂 hal/               ⏳ HAL 库版（开发中）
```

两种实现共用同一套硬件与调参方法，所以文档放在 `documents/` 只维护一份。

---

## 🗺️ 路线图

- [x] 标准外设库版本，三闭环调通
- [x] OLED 显示角度 / 距离 / 速度
- [x] 超声波测距
- [ ] HAL 库版本（STM32CubeMX 重实现）
- [ ] 手机 APP 蓝牙遥控
- [ ] 超声波自动避障

---

## 🐛 常见问题

| 现象 | 原因与解决 |
|:---|:---|
| 车轮乱转、站不住 | 机械中值 `Med_Angle` 不对，先读 OLED 角度填对 |
| 朝一个方向越跑越快 | 速度环没调好，或有一路编码器没接上 |
| 一直朝一边倒 | 电机方向线接反 |
| OLED 不亮 | 检查 `PB8`/`PB9`；工程已禁用 JTAG，须用 SWD |
| MPU6050 初始化失败 | 检查 `PB3`/`PB4` 软件 I2C；模块需 3.3V |
| 编译报 Device not found | 未装 STM32F1 器件支持包 |
| 角度乱跳 | DMP 未加载成功，检查 AD0 引脚与供电 |

---

## 🙏 致谢与声明

本项目为学习用途的复刻实现，其中第三方代码保持各自的原始版权声明：

- `std/STM32F10x_FWLib/`、`std/CORE/` —— STMicroelectronics 标准外设库与 ARM CMSIS
- `std/HARDWARE/MPU6050/eMPL/` —— InvenSense Motion Driver，版权归 InvenSense Corporation
- `std/SYSTEM/` —— 国内 STM32 教学资料的通用写法

感谢以上各方。若您是某部分代码的权利人且不希望被引用，请提交 Issue，我立即移除。

---

## 📄 许可

本人编写与整理的部分（参数整定、接线整理、文档与注释）采用 **MIT License**，详见 [LICENSE](LICENSE)。

第三方代码仅供学习研究，请勿用于商业用途。
