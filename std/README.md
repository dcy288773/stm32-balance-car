# 🔧 标准外设库版（StdPeriph + Keil MDK）

芯片 **STM32F103C8T6**，ST 标准外设库 V3.5，Keil MDK-ARM 工程。

> 完整的硬件清单与引脚接线见 [`../documents/hardware.md`](../documents/hardware.md)，PID 调参见 [`../documents/tuning.md`](../documents/tuning.md)。

---

## 🚀 编译与烧录

1. 安装 **Keil MDK-ARM（uVision5）** 并装 **STM32F1 器件支持包**（`Keil::STM32F1xx_DFP`）
2. 打开 `USER/UpStanding_Car.uvprojx`
3. 按 **F7** 编译 → 应为 `0 Error(s)`
4. 按 **F8** 下载 → 上电即站立

不想编译，可直接烧录 [`firmware/UpStanding_Car.hex`](firmware/UpStanding_Car.hex)。

> ⚠️ 工程禁用了 JTAG（释放 PB3/PB4 给 MPU6050），必须用 **SWD** 下载。

---

## 📁 目录结构

```
CORE/                CMSIS 内核与启动文件
STM32F10x_FWLib/     ST 标准外设库 V3.5
SYSTEM/              delay / sys / usart
HARDWARE/
  CONTROL/           ⭐ 三闭环 PID（项目核心）
  ENCODER/           TIM2 / TIM4 编码器测速
  MOTOR/             电机方向与 PWM 装载
  PWM/               TIM1 CH1/CH4 PWM 输出
  EXTI/              MPU6050 中断，5ms 控制周期
  MPU6050/           驱动 + 软件 I2C + eMPL(DMP)
  OLED/              SSD1306 显示驱动
  SENSOR/            HC-SR04 超声波测距
  USART3/            蓝牙串口
USER/                main.c、中断、Keil 工程
firmware/            ✅ 已编译固件
```

工程文件内全部使用相对路径（如 `..\SYSTEM\delay`），整体移动到本目录不影响编译。

---

## 🐛 常见问题

| 现象 | 原因与解决 |
|:---|:---|
| 车轮乱转、站不住 | 机械中值 `Med_Angle` 不对 |
| 朝一个方向越跑越快 | 速度环未调好，或有一路编码器未接 |
| 一直朝一边倒 | 电机方向线接反 |
| OLED 不亮 | 检查 PB8/PB9；须用 SWD 下载 |
| MPU6050 初始化失败 | 检查 PB3/PB4 软件 I2C；模块需 3.3V |
| 编译报 Device not found | 未装 STM32F1 器件支持包 |
| 角度乱跳 | DMP 未加载，检查 AD0 与供电 |

---

## 🙏 第三方代码

| 目录 | 来源 |
|:---|:---|
| `STM32F10x_FWLib/`、`CORE/` | STMicroelectronics 标准外设库 / ARM CMSIS |
| `HARDWARE/MPU6050/eMPL/` | InvenSense Motion Driver，版权归 InvenSense Corporation |
| `SYSTEM/` | 国内 STM32 教学资料通用写法 |

以上仅供学习研究，勿用于商业用途。权利人不希望被引用时请提 Issue，立即移除。
