# PID 参数与调参

两版实现的控制结构与参数一致，参数都集中在控制文件顶部。

- 标准库版：`std/HARDWARE/CONTROL/control.c`

## 参数

```c
float Med_Angle   = 0;        /* 机械中值 */
float Vertical_Kp = -400;     /* 直立环 比例 */
float Vertical_Kd = -1.92;    /* 直立环 微分 */
float Velocity_Kp = -0.44;    /* 速度环 比例 */
float Velocity_Ki = -0.0022;  /* 速度环 积分 */
float Turn_Kp     = 20;       /* 转向环 比例 */
float Turn_Kd     = 0.6;      /* 转向环 微分 */
```

## 调参顺序

必须按 1→5 依次调，跳步调不出来。

1. **机械中值 `Med_Angle`**：让车静止，读 OLED 上显示的角度，填进去。填错后面全部白费。
2. **直立环 Kp**：从小到大加，到车能猛地立住但往前冲。
3. **直立环 Kd**：加上后抑制抖动，车开始小幅摆动而不倒。
4. **速度环 Kp、Ki**：让车不再持续跑偏，能原地站住。
5. **转向环 Kp、Kd**：最后调，让车走直线。

## 关键常量

| 常量 | 值 | 含义 |
|---|---|---|
| `PWM_MAX` / `PWM_MIN` | ±7200 | PWM 限幅，对应 ARR=7199 |
| `SPEED_Y` | 40 | 前后最大设定速度 |
| `SPEED_Z` | 100 | 转向最大设定速度 |
| 控制周期 | 5 ms | MPU6050 INT 触发 `EXTI9_5_IRQHandler` |
