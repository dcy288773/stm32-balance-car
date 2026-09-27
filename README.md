# STM32 两轮自平衡小车（520 电机）

基于 **STM32F103C8T6** 的两轮自平衡小车。三闭环 PID 控制，可自主站立、前进后退、转向，OLED 实时显示角度、距离与轮速。

本仓库按实现方式分目录管理：

| 实现版本 | 目录 | 状态 |
|---|---|---|
| ST 标准外设库（StdPeriph + Keil MDK） | [`std/`](std/) | ✅ 完成，可直接编译烧录 |
| STM32CubeMX + HAL 库 | [`hal/`](hal/) | 开发中 |

## 文档

- 硬件清单与引脚接线：[`documents/hardware.md`](documents/hardware.md)
- PID 参数与调参顺序：[`documents/tuning.md`](documents/tuning.md)

## 许可

本人编写与整理的部分采用 MIT License，详见 [LICENSE](LICENSE)。

第三方代码（`std/STM32F10x_FWLib/`、`std/CORE/`、`std/HARDWARE/MPU6050/eMPL/`）保持各自原始版权声明，仅供学习研究。
