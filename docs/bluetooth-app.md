# 蓝牙遥控 App 使用教程 · Bluetooth Remote App Guide

[中文](#中文) · [English](#english)

> **APK 下载** · **APK download**: [`docs/app/lanyatiaoshiqi.apk`](app/lanyatiaoshiqi.apk) (2.3 MB, Android)
>
> 应用名 · App name：**蓝牙调试器**（Lazy Dog BT Controller）
> 包名 · Package：`xnj.lazydog.btcontroller`
> 同时支持经典蓝牙和 BLE · supports both Classic Bluetooth (SPP) and BLE

---

# 中文

## 0. 这个 App 干什么用

小车通过 USART3 接蓝牙模块（JDY-31）接收单字节指令。这个 App 就是一个**可自定义按键发送内容的蓝牙串口终端** —— 你在手机屏幕上按下按钮，它就把对应的一个字节发给小车。

## 1. 安装

1. 把 `docs/app/lanyatiaoshiqi.apk` 传到手机
2. 点击安装，系统提示"未知来源"时选择**允许**
3. 安装完打开，授予**蓝牙**和**位置信息**权限

## 2. ⚠️ 前提：蓝牙和定位都要打开

这是最容易卡住的一步，**两个开关缺一不可**：

| 开关 | 在哪 | 不开会怎样 |
|---|---|---|
| **蓝牙** | 下拉快捷面板 / 设置 | 完全搜不到设备 |
| **定位（位置信息）** | 下拉快捷面板 / 设置 → 位置信息 | **能搜到但列表是空的** |

> 原因：安卓从 6.0 开始，扫描 BLE 设备属于"可能推断地理位置"的行为，系统强制要求定位权限已授予**且定位开关处于打开状态**，否则扫描回调直接不返回结果。这不是 App 的 bug。

另外还要给 App 授予**定位权限**（设置 → 应用 → 蓝牙调试器 → 权限）。

## 3. 连接小车

1. 小车先上电，等待 2 秒
2. 打开 App → 搜索设备 → 在列表里找 **`JDY-31`**（或以 `JDY` 开头的名字）
3. 点击连接，状态变成"已连接"

> 如果列表里能看到设备但连不上，先关掉 App 再重开，或者把小车断电重启一次。

## 4. ⚠️ 关键设置：编辑模式 + HEX 模式

**这两步不做，车一定不动。** 出厂默认按键发的是普通文本，需要改成十六进制字节。

### 4.1 打开编辑模式

1. 切到 **「按钮控制」** 界面
2. 找到 **「编辑模式」** 开关（通常在界面右上角或长按按钮弹出菜单，不同版本位置略有差异）
3. **打开它**

### 4.2 逐个修改按键的发送数据

1. 点某个按键进入它的设置
2. 找到 **「按下时发送的数据」**
3. 输入对应的十六进制值（见下面的对照表）
4. **打开这个界面里的「HEX 模式」开关** ← 最关键的一步

> ⚠️ **不打 HEX 模式会怎样**：你输入 `01`，App 发出去的是两个 ASCII 字符 `'0'` 和 `'1'`（`0x30 0x31`），小车收到的是不认识的值，直接忽略。表现出来就是"连上了、按了没反应"。
>
> 打了 HEX 模式，`01` 才真的是一个字节 `0x01`。

### 4.3 五个按键的数据

| 按键 | 发送数据（HEX） | 含义 |
|:---:|:---:|:---|
| 前进 | `01` | 0x01 |
| 后退 | `05` | 0x05 |
| 左转 | `07` | 0x07 |
| 右转 | `03` | 0x03 |
| 停止 | `00` | 0x00 |

> 按键怎么排随你，**只要 HEX 值对上就行**。建议把「停止」放在最显眼、最好按的位置。
>
> 不要多发字节（比如 `0102`），这套固件只认单字节指令。

## 5. 怎么开

### 5.1 ⚠️ 指令是锁存的

按一次「前进」，小车会**一直加速到上限并保持**，不会自己停。要停必须按「停止」。

- 前进目标上限是 `RC_SPEED_MAX`（默认 30，约 0.4 m/s），从 0 加到上限约需 300 ms
- 转向同理，松手不会回正
- 这是 `hal/MyAPP/bluetooth.h` 里 `RC_STOP_ON_TIMEOUT = 0` 的设计

### 5.2 第一次测试一定要架空车轮

1. 把车架起来，轮子悬空
2. 按「前进」→ 两个轮子应该同向转，并且**转速稳定后不再变化**
3. 按「停止」→ 两个轮子应在约 300 ms 内停下
4. 按「左转 / 右转」→ 两个轮子应该**一快一慢**
5. 这些都对了，再落地试

### 5.3 转向方向反了怎么办

不要动 `Turn_Kp` 的正负号（会连带影响直立环）。改 `hal/MyAPP/bluetooth.c` 里按键的映射关系即可。

## 6. 常见问题

| 现象 | 原因 | 解决 |
|---|---|---|
| 搜不到任何设备 | 定位开关没开 | 打开定位 + 授予 App 定位权限 |
| 搜到列表是空的 | 同上 | 同上 |
| 能看到 `JDY-31` 但连不上 | 模块已被别的手机占用 / 需要重启 | 小车断电重开，关掉其它已连接 App |
| 连上了、按了没反应 | **HEX 模式没开** | 按 4.2 打开 HEX |
| 连上了、偶尔能动 | 波特率不对 | 模块应为 **9600**，用 AT 指令确认 |
| 前进变后退 | 按键数据填反了 | 按 4.3 对照表重填 |
| 车一直跑不停 | 没按「停止」 | 指令是锁存的，必须按停 |
| App 里有广告 | 免费版自带 | 正常，不影响功能 |

---

# English

## 0. What this app is for

The car receives **single-byte commands** over USART3 from a Bluetooth module (JDY-31). This app is a customizable Bluetooth serial terminal — each on-screen button sends one byte to the car.

## 1. Install

1. Copy `docs/app/lanyatiaoshiqi.apk` to your phone
2. Tap to install; allow "Install from unknown sources" when prompted
3. Open it and grant **Bluetooth** and **Location** permissions

## 2. ⚠️ Prerequisite: enable BOTH Bluetooth and Location

This is the step people get stuck on. **Both switches are required:**

| Switch | Where | If it is off |
|---|---|---|
| **Bluetooth** | Quick panel / Settings | No devices found at all |
| **Location** | Quick panel / Settings → Location | **Devices found, but the list stays empty** |

> Why: since Android 6.0, scanning for BLE devices is treated as something that could reveal location, so the system requires the location permission to be granted **and** the location switch to be on. Otherwise the scan callback simply never returns results. This is not an app bug.

Also grant the app itself the **Location** permission (Settings → Apps → 蓝牙调试器 → Permissions).

## 3. Connect to the car

1. Power the car on and wait 2 seconds
2. Open the app → scan → pick **`JDY-31`** (or any name starting with `JDY`)
3. Tap to connect; the status should turn to "connected"

> If the device shows up but the connection fails, restart the app, or power-cycle the car.

## 4. ⚠️ Critical: edit mode + HEX mode

**Skipping these two steps means the car will never move.** Out of the box, buttons send plain text; they must send raw hex bytes.

### 4.1 Turn on edit mode

1. Switch to the **Button Control** page
2. Find the **Edit Mode** switch (usually top-right, or long-press a button; the exact location varies by version)
3. **Turn it on**

### 4.2 Edit what each button sends

1. Tap a button to open its settings
2. Find the **"data to send on press"** field
3. Enter the hex value from the table below
4. **Turn on the "HEX mode" switch in that dialog** ← the single most important step

> ⚠️ **Without HEX mode**: typing `01` sends the two ASCII characters `'0'` and `'1'` (`0x30 0x31`). The car sees a value it does not recognise and ignores it — which looks exactly like "connected but pressing does nothing".
>
> With HEX mode enabled, `01` really is the single byte `0x01`.

### 4.3 The five buttons

| Button | Data (HEX) | Meaning |
|:---:|:---:|:---|
| Forward | `01` | 0x01 |
| Backward | `05` | 0x05 |
| Turn left | `07` | 0x07 |
| Turn right | `03` | 0x03 |
| Stop | `00` | 0x00 |

> The button layout is up to you — **only the hex values matter**. Put "Stop" somewhere easy to reach.
>
> Do not send multiple bytes at once (e.g. `0102`); this firmware only accepts single-byte commands.

## 5. Driving

### 5.1 ⚠️ Commands are latched

Press "Forward" once and the car **keeps accelerating to the limit and stays there**. It will not stop by itself — you must press "Stop".

- Forward target limit is `RC_SPEED_MAX` (30 by default, roughly 0.4 m/s); ramping from 0 to the limit takes about 300 ms
- Turning behaves the same: releasing does not recentre the car
- This is by design: `RC_STOP_ON_TIMEOUT = 0` in `hal/MyAPP/bluetooth.h`

### 5.2 Always test with the wheels off the ground first

1. Prop the car up so the wheels spin freely
2. Press "Forward" → both wheels spin the same direction and **settle at a constant speed**
3. Press "Stop" → both wheels stop within about 300 ms
4. Press "Turn left / Turn right" → one wheel should spin faster than the other
5. Only then put it on the floor

### 5.3 Turn direction is reversed

Do **not** flip the sign of `Turn_Kp` (it also affects the balance loop). Change the button mapping in `hal/MyAPP/bluetooth.c` instead.

## 6. Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| No devices found | Location switch is off | Enable location + grant the app location permission |
| Device list is empty | Same as above | Same as above |
| `JDY-31` visible but won't connect | Module already held by another phone / needs a restart | Power-cycle the car, close other apps |
| Connected, pressing does nothing | **HEX mode not enabled** | Follow section 4.2 |
| Connected, works only occasionally | Wrong baud rate | Must be **9600**; verify with AT commands |
| Forward drives backward | Wrong byte on the button | Re-check against section 4.3 |
| Car runs and never stops | "Stop" was never pressed | Commands are latched by design |
| Ads in the app | Free version | Harmless, does not affect functionality |
