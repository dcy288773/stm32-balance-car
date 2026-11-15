# Third-Party Notices

This repository bundles third-party sources that are **NOT** covered by the MIT
License in [`LICENSE`](LICENSE). They keep their own copyright notices and
license terms and are included for study purposes only.

## `std/STM32F10x_FWLib/` and `std/CORE/`

STMicroelectronics Standard Peripheral Library and ARM CMSIS
Cortex-M3 device support files.

- Copyright (c) STMicroelectronics
- Copyright (c) ARM Limited (CMSIS)
- Distributed under the terms stated in the headers of the individual files.

## `std/HARDWARE/MPU6050/eMPL/` and `hal/MyAPP/inv_mpu*.c|h`

InvenSense Motion Driver (eMPL) — DMP firmware and driver.

- Copyright (C) 2011-2012 InvenSense Corporation
- Distributed under the terms stated in the headers of the individual files.

## `hal/MyAPP/ssd1306*.c|h`

SSD1306 OLED driver, based on
[afiskon/stm32-ssd1306](https://github.com/afiskon/stm32-ssd1306) (MIT).

## `std/SYSTEM/`

Common STM32 teaching-code style (`delay` / `sys` / `usart`), widely circulated
in Chinese STM32 tutorials. Original authorship is unclear; kept as-is for
compatibility with the rest of the project.

## Scope of the MIT License

Only the parts written and organized by the repository owner — control logic,
parameter tuning, wiring documentation, README, and comments — are released
under the MIT License in [`LICENSE`](LICENSE).

If you are a rights holder and would prefer your code not to be referenced
here, please open an Issue and it will be removed immediately.
