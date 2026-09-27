# STM32 两轮自平衡小车（520 电机 / 标准库版）

基于 **STM32F103C8T6** 的两轮自平衡小车完整工程，使用 **ST 标准外设库（StdPeriph）** 开发，Keil MDK 工程可直接打开编译。

小车能**自主站立并保持平衡**，支持前进 / 后退 / 转向，通过 OLED 实时显示车身角度、超声波测距与车轮转速。

> 无需任何修改，下载 → 编译 → 烧录 → 上电即可站立。

---

## 一、硬件清单（BOM）

| 模块 | 规格 | 数量 | 备注 |
|---|---|---|---|
| 主控 | STM32F103C8T6 最小系统板 | 1 | 工程按 C8（64KB Flash）配置 |
| 姿态传感器 | MPU6050 模块（带 INT 引脚） | 1 | 用 DMP 硬件解算角度 |
| 电机 | 520 减速电机（带 AB 相霍尔编码器） | 2 | 左右各一 |
| 电机驱动 | TB6612FNG 或 L298N 双路驱动 | 1 | 本工程用 **TB6612** 逻辑 |
| 显示屏 | 0.96 寸 OLED（I2C，SSD1306） | 1 | 4 脚 |
| 测距 | HC-SR04 超声波模块 | 1 | 可选，用于避障/跟随 |
| 蓝牙 | HC-05 / HC-06 串口蓝牙 | 1 | 可选，接 USART3 |
| 电源 | 18650 电池 + 7.4V 输出 | 1 组 | 电机与主控建议分开供电、共地 |
| 车体 | 两轮平衡车底盘套件 | 1 | 电机需**左右对称安装** |

---

## 二、引脚接线对照表

### 电机与编码器

| 功能 | STM32 引脚 | 说明 |
|---|---|---|
| 左轮 PWM | **PA8** | TIM1_CH1 |
| 右轮 PWM | **PA11** | TIM1_CH4 |
| 左轮方向 AIN1 / AIN2 | **PB14 / PB15** | 推挽输出 |
| 右轮方向 BIN1 / BIN2 | **PB13 / PB12** | 推挽输出 |
| 左轮编码器 A/B | **PA0 / PA1** | TIM2 编码器模式 |
| 右轮编码器 A/B | **PB6 / PB7** | TIM4 编码器模式 |

### 传感器与外设

| 功能 | STM32 引脚 | 说明 |
|---|---|---|
| MPU6050 SCL | **PB3** | 软件 I2C |
| MPU6050 SDA | **PB4** | 软件 I2C |
| MPU6050 INT | **PB5** | 外部中断 EXTI_Line5，下降沿触发（5ms 控制周期） |
| 超声波 TRIG | **PA3** | 推挽输出 |
| 超声波 ECHO | **PA2** | 浮空输入 |
| OLED SCL | **PB8** | 软件 I2C |
| OLED SDA | **PB9** | 软件 I2C |
| 蓝牙 TX / RX | **PB10 / PB11** | USART3，波特率 9600 |
| 调试串口 | **PA9 / PA10** | USART1，波特率 115200 |

> ⚠️ **注意**：OLED 初始化时执行了 `GPIO_Remap_SWJ_JTAGDisable`，禁用了 JTAG 以释放 PB3/PB4。
> 如果你用 JTAG 下载器，请改用 **SWD 下载**（只需 SWDIO / SWCLK 两根线）。

---

## 三、两种验证方式（挑一个）

### 方式 A：直接烧录已编译固件（最快，1 分钟）

1. 打开 `firmware/UpStanding_Car.hex`
2. 用 ST-Link Utility / FlyMcu / STM32CubeProgrammer 烧录到 STM32F103C8
3. 上电，小车应自行站立

### 方式 B：自己编译（推荐，证明你真的跑通了）

1. 安装 **Keil MDK-ARM（uVision5）** 并确保已安装 **STM32F1 器件支持包**
2. 双击打开 `USER/UpStanding_Car.uvprojx`
3. 点击 **Build**（F7），应显示 `0 Error(s), 0 Warning(s)`，并生成 `OBJ/UpStanding_Car.hex`
4. 点击 **Download**（F8）烧录

> 若编译报 "Device not found"，说明没装 STM32F1 的 Device Family Pack，用 Pack Installer 装 `Keil::STM32F1xx_DFP`。

---

## 四、PID 参数与调参顺序

所有参数集中在 `HARDWARE/CONTROL/control.c` 顶部，**改这里就能调**。

```c
float Med_Angle   = 0;        // 机械中值：车能自己站住的那个角度
float Vertical_Kp = -400;     // 直立环 比例
float Vertical_Kd = -1.92;    // 直立环 微分
float Velocity_Kp = -0.44;    // 速度环 比例
float Velocity_Ki = -0.0022;  // 速度环 积分
float Turn_Kp     = 20;       // 转向环 比例
float Turn_Kd     = 0.6;      // 转向环 微分
```

**调参必须按这个顺序，跳步一定调不出来：**

1. **先定机械中值 `Med_Angle`** —— 让车静止，读 OLED 上显示的角度，把它填进去。这一步错了后面全白费。
2. **直立环 Kp** —— 从小到大加，直到车能"猛地一下"立住但会往前冲。
3. **直立环 Kd** —— 加上 Kd 抑制抖动，车会开始来回小幅摆动而不倒。
4. **速度环 Kp、Ki** —— 让车不再持续向一个方向跑偏，能原地站住。
5. **转向环 Kp、Kd** —— 最后调，让车走直线不偏航。

**关键常量**：

| 常量 | 值 | 含义 |
|---|---|---|
| `PWM_MAX / PWM_MIN` | ±7200 | PWM 限幅（对应 ARR=7199，即满占空比） |
| `SPEED_Y` | 40 | 前后运动最大设定速度 |
| `SPEED_Z` | 100 | 左右转向最大设定速度 |
| 控制周期 | 5 ms | 由 MPU6050 的 INT 引脚触发 `EXTI9_5_IRQHandler` |

---

## 五、目录结构

```
├── CORE/                 CMSIS 内核支持（core_cm3、启动文件）
├── STM32F10x_FWLib/      ST 标准外设库 V3.5
├── SYSTEM/               系统级驱动：延时、位带操作、串口
│   ├── delay/  sys/  usart/
├── HARDWARE/
│   ├── CONTROL/          三闭环 PID 控制（核心）
│   ├── ENCODER/          编码器测速（TIM2 / TIM4）
│   ├── MOTOR/            电机方向控制与 PWM 装载
│   ├── PWM/              TIM1 CH1/CH4 PWM 输出
│   ├── EXTI/             MPU6050 中断，5ms 控制周期
│   ├── MPU6050/          姿态传感器驱动
│   │   ├── mpu6050.c/h   寄存器读写
│   │   ├── mpuiic.c/h    软件 I2C
│   │   └── eMPL/         InvenSense DMP 运动驱动库
│   ├── OLED/             SSD1306 显示驱动
│   ├── SENSOR/           HC-SR04 超声波测距（TIM3）
│   └── USART3/           蓝牙串口
├── USER/
│   ├── main.c            主函数与初始化流程
│   ├── stm32f10x_it.c    中断服务函数
│   └── UpStanding_Car.uvprojx   Keil 工程文件
└── firmware/             已编译固件，可直接烧录
```

---

## 六、常见问题

| 现象 | 原因与解决 |
|---|---|
| 上电后车轮乱转、站不住 | 机械中值 `Med_Angle` 不对，先读 OLED 角度改对了再调 PID |
| 车往一个方向越跑越快 | 速度环没调好，或左右编码器有一路没接好 |
| 车一直朝一边倒 | 检查电机方向线是否接反；`Encoder_Left` 取了负号，若装反需改回来 |
| OLED 不亮 | 确认 PB8/PB9；注意工程已禁用 JTAG，必须用 SWD 下载 |
| MPU6050 初始化失败 | 检查 PB3/PB4 软件 I2C；模块需 3.3V 供电 |
| 编译报 `Device not found` | 未安装 STM32F1 器件支持包 |
| OLED 显示角度乱跳 | DMP 未成功加载，检查 MPU6050 的 AD0 与供电 |

---

## 七、代码来源与致谢

本项目为学习用途的复刻实现，其中：

- `STM32F10x_FWLib/`、`CORE/` —— 来自 STMicroelectronics 官方标准外设库与 ARM CMSIS
- `SYSTEM/`（delay / sys / usart）—— 沿用国内 STM32 教学资料的通用写法
- `HARDWARE/MPU6050/eMPL/` —— InvenSense Motion Driver，版权归 InvenSense Corporation 所有
- 控制算法与整体工程整合 —— 基于公开的两轮平衡车教学方案实现

在此对以上各方表示感谢。若您是某部分代码的权利人且不希望被引用，请提交 Issue，我会立即移除。

---

## 八、许可

仓库中**本人编写与整理的部分**（参数整定、接线整理、文档、注释）采用 **MIT License**，详见 [LICENSE](LICENSE)。

第三方代码（`STM32F10x_FWLib/`、`CORE/`、`HARDWARE/MPU6050/eMPL/`）保持各自的原始许可与版权声明，**仅供学习研究，请勿用于商业用途**。
