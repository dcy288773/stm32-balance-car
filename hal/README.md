# HAL 库版（STM32CubeMX + HAL）

基于 **STM32F103C8T6 + STM32CubeMX + HAL 库 + Keil MDK** 的两轮自平衡小车。
硬件接线与 [`std/`](../std/) 版完全相同，控制思路也一致（三闭环串级 PID）。

---

## 与标准库版的差异

| 项 | `std/` | `hal/` |
|:---|:---|:---|
| 控制节拍来源 | MPU6050 INT（PB5）EXTI 下降沿 | **TIM3 定时器中断**，PSC=71 / ARR=4999 → 200 Hz |
| PB5 | 使用 | **未使用** |
| OLED 总线 | 软件 I2C | **硬件 I2C1**（PB8/PB9，需 `__HAL_AFIO_REMAP_I2C1_ENABLE()`） |
| MPU6050 总线 | 软件 I2C（PB3/PB4） | 软件 I2C（PB3/PB4，DWT 微秒延时） |
| 超声波测距 | 有 | **未实现** |
| 蓝牙遥控 | 1 字节指令，指令锁存 | 1 字节指令 + 锁存 + 斜坡加减速 + 可选失联保护 |
| 参数位置 | `std/HARDWARE/CONTROL/control.c` | `hal/MyAPP/pid.c` |

---

## 目录结构

```
hal/
├── balance_car.ioc      CubeMX 工程文件（引脚与外设配置的唯一来源）
├── .mxproject           CubeMX 生成的工程信息
├── Core/
│   ├── Inc/             CubeMX 生成的头文件
│   └── Src/             main.c、各外设初始化、中断
├── Drivers/             HAL 库 + CMSIS
├── MDK-ARM/
│   ├── balance_car.uvprojx   Keil 工程
│   └── startup_stm32f103xb.s
└── MyAPP/               用户代码
    ├── pid.c / pid.h            三闭环控制（直立 200Hz / 速度 50Hz / 转向）
    ├── bluetooth.c / bluetooth.h 蓝牙遥控（USART3）
    ├── motor.c / motor.h        电机方向 + PWM 输出
    ├── encoder.c / encoder.h    编码器读数
    ├── mpu6050.c / .h            MPU6050 底层驱动
    ├── IIC.c / IIC.h             软件 I2C（DWT 延时）
    ├── inv_mpu*.c / .h           InvenSense DMP 驱动
    └── ssd1306*.c / .h           OLED 驱动
```

---

## 编译下载

1. 打开 `MDK-ARM/balance_car.uvprojx`
2. `Options for Target → Target` 确认勾选 **Use MicroLIB**（`sprintf` 依赖它）
3. **F7** 编译，应显示 `0 Error(s), 0 Warning(s)`
4. **F8** 下载

当前编译产物：

```
Code=23856  RO-data=5368  RW-data=276  ZI-data=2668
```

**必须用 SWD 下载。** `stm32f1xx_hal_msp.c` 里调了 `__HAL_AFIO_REMAP_SWJ_NOJTAG()` 释放 PB3/PB4 给 MPU6050 的软件 I2C，JTAG 已被关闭。

> ⚠️ CubeMX 重新生成代码会覆盖 `MDK-ARM/*.uvprojx`、清空 Keil 的 `MyAPP` 文件组和头文件路径、取消 MicroLIB 勾选。生成后必须手工恢复这三项。

---

## 控制结构

```
TIM3 溢出中断 @200Hz (5ms)
        │
        ├─ 读 DMP（一次 FIFO 读取同时给姿态角与角速度）
        │
        ├─ 每 4 次 → 速度环 Velocity() @50Hz（20ms）
        │      输出 Velocity_out（单位：度）叠加到直立环目标角
        │
        ├─ 直立环 Vertical() @200Hz
        │      输入：(Velocity_out + Med_Angle)、Pitch、gyroy
        │
        ├─ 转向环 Turn() @200Hz
        │      输出 Turn_out，作为左右轮差速
        │
        └─ MOTO1 = PWM - Turn_out
           MOTO2 = PWM + Turn_out → Limit() → Load()
```

**安全和异常处理**

- DMP 读失败：丢弃该次速度窗口；连续失败 2 次判定姿态源失效，立即停机
- FIFO 积压：清空并重开速度环测量窗口，不用旧姿态控车
- 倾角超过 60°：切断电机输出
- 电机输出饱和时速度环做抗积分饱和

---

## 可调参数

**`MyAPP/pid.c` 顶部**

| 行 | 参数 | 当前值 | 说明 |
|:---|---:|---:|:---|
| 70 | `Med_Angle` | `0.0f` | 机械中值，**标定值，不是调出来的** |
| 74 | `Vertical_Kp` | `-400.0f` | 直立环 P |
| 75 | `Vertical_Kd` | `-1.92f` | 直立环 D |
| 77 | `Velocity_Kp` | `-0.44f` | 速度环 P |
| 78 | `Velocity_Ki` | `-0.0022f` | 速度环 I |
| 80 | `Turn_Kd` | `0.6f` | 转向环 D，兼作直线约束 |
| 81 | `Turn_Kp` | `20.0f` | 转向环 P |

**符号不能改**：这些正负号是实测得到的极性约定，改了车会直接倒。

**`MyAPP/bluetooth.h`**

| 行 | 宏 | 当前值 | 说明 |
|:---|---:|---:|:---|
| 38 | `RC_STOP_ON_TIMEOUT` | `0` | 0 = 指令一直保持；1 = 300ms 无数据自动停车 |
| 43 | `RC_TIMEOUT_MS` | `300U` | 失联判定时间 |
| 47 | `RC_SPEED_MAX` | `30.0f` | 前进目标上限，单位是"编码器计数/10ms"，约 0.4 m/s |
| 50 | `RC_TURN_MAX` | `150.0f` | 转向目标上限（`Turn_Kp` 的输入单位） |
| 56 | `RC_SPEED_STEP` | `0.5f` | 每 5ms 变化量，等效每 10ms 变化 1 |
| 57 | `RC_TURN_STEP` | `15.0f` | 每 5ms 变化量，等效每 10ms 变化 30 |

---

## 蓝牙遥控

USART3，PB10=TX / PB11=RX，9600 8N1。指令为单字节，沿用厂商手机 App 的编码：

| 指令 | 含义 |
|:---:|:---|
| `0x00` | 停 |
| `0x01` | 前进 |
| `0x05` | 后退 |
| `0x07` | 左转 |
| `0x03` | 右转 |

**指令锁存**：收到新指令前，上一个方向一直有效，靠 `0x00` 或反方向指令改变状态。

**斜坡**：目标值每 5ms 变化一步，从 0 到满速约 300ms，从满速到 0 也走斜坡（防止急刹引起点头）。

**失联保护**默认关闭——因为手机 App 是按键单次发送，开启后车只会动 300ms 就停。需要断连保护时把 `RC_STOP_ON_TIMEOUT` 改成 1，并适当加大 `RC_TIMEOUT_MS`。

**调试量**（显示或观察窗口用）：`Bluetooth_RxCount`、`Bluetooth_LastByte`、`Bluetooth_InitResult`、`Bluetooth_ErrorCount`、`Bluetooth_OreCount`、`Bluetooth_FeCount`、`Bluetooth_NeCount`、`Bluetooth_PeCount`。

> `ORE` 持续增加 = 消费太慢；`FE`/`NE` 持续增加 = 波特率不对或线材干扰。

---

## 已知未实现

- **HC-SR04 超声波测距**：硬件接口（PA3=TRIG / PA2=ECHO）在板上存在，但本版本没有驱动，也没有接进控制环
- OLED 未显示蓝牙状态，只显示 `M/A/G/V/O` 五行（初始化状态、角度、角速度、转速、直立环输出）

---

## 进度记录

| 日期 | 内容 |
|---|---|
| 2026-11-15 | CubeMX 工程 + 三闭环控制 + 蓝牙遥控完成，编译 0 错误 0 警告 |
| — | 超声波测距待实现 |
