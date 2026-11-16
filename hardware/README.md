# 硬件文件 · Hardware Files

[中文](#中文) · [English](#english)

---

# 中文

## 目录

```
hardware/
├── pcb/
│   └── driver_board_gerber_jlcpcb.zip   嘉立创下单打包文件
└── 3d-print/
    ├── bottom_plate_150x80x4.stl        底板 150 × 80 × 4 mm
    └── top_plate_150x80x3.stl           顶板 150 × 80 × 3 mm
```

## PCB 驱动板

`pcb/driver_board_gerber_jlcpcb.zip` 是**嘉立创（JLCPCB）下单用的打包文件**，里面已经包含：

| 内容 | 文件 |
|---|---|
| 顶层 / 底层铜箔 | `Gerber_TopLayer.GTL` / `Gerber_BottomLayer.GBL` |
| 顶层 / 底层丝印 | `Gerber_TopSilkscreenLayer.GTO` / `Gerber_BottomSilkscreenLayer.GBO` |
| 顶层 / 底层阻焊 | `Gerber_TopSolderMaskLayer.GTS` / `Gerber_BottomSolderMaskLayer.GBS` |
| 底层钢网层 | `Gerber_BottomPasteMaskLayer.GBP` |
| 板框 | `Gerber_BoardOutlineLayer.GKO` |
| 文档层 / 钻孔图 | `Gerber_DocumentLayer.GDL` / `Gerber_DrillDrawingLayer.GDD` |
| 钻孔（沉铜 / 非沉铜 / 过孔） | `Drill_PTH_Through.DRL` / `Drill_NPTH_Through.DRL` / `Drill_PTH_Through_Via.DRL` |
| 飞针测试 | `FlyingProbeTesting.json` |

- **层数**：2 层
- **外形尺寸**：约 **90.1 × 70.1 mm**（从板框 Gerber 读出）

### 下单步骤

1. 打开 [嘉立创下单页面](https://www.jlc.com/)，选择 **PCB 下单**
2. 直接把 `driver_board_gerber_jlcpcb.zip` **拖进去上传**（不要解压）
3. 层数会自动识别成 **2 层**；板厚建议 **1.6 mm**，其余按默认
4. 下单前可以用网页自带的 Gerber 预览确认一次外形和丝印

官方教程：<https://prodocs.lceda.cn/cn/pcb/order-order-pcb/index.html>
（压缩包内附的 `PCB下单必读.txt` 就是这个链接）

> 不同批次打样可能有改版，**下单前请用预览核对一次孔位与外形**。

## 3D 打印件

| 文件 | 零件 | 尺寸 |
|---|---|---|
| `bottom_plate_150x80x4.stl` | 底板 | 150 × 80 × 4 mm |
| `top_plate_150x80x3.stl` | 顶板 | 150 × 80 × 3 mm |

- 单位 **mm**，二进制 STL，可直接拖进切片软件（Cura / OrcaSlicer / PrusaSlicer）
- 参考参数：**PLA，0.2 mm 层高，30–40 % 填充，无需支撑**
- 底板是承重件，可以加厚或把填充提到 60 %

> ⚠️ **PCB 和 3D 打印件不是同一个零件**：驱动板约 90 × 70 mm，底板是 150 × 80 mm，板子装在底板上。两者的固定孔位请打印前自行核对。

---

# English

## Layout

```
hardware/
├── pcb/
│   └── driver_board_gerber_jlcpcb.zip   JLCPCB order package
└── 3d-print/
    ├── bottom_plate_150x80x4.stl        bottom plate 150 × 80 × 4 mm
    └── top_plate_150x80x3.stl           top plate 150 × 80 × 3 mm
```

## PCB — driver board

`pcb/driver_board_gerber_jlcpcb.zip` is a ready-to-upload **JLCPCB order package** containing:

| Content | File |
|---|---|
| Top / bottom copper | `Gerber_TopLayer.GTL` / `Gerber_BottomLayer.GBL` |
| Top / bottom silkscreen | `Gerber_TopSilkscreenLayer.GTO` / `Gerber_BottomSilkscreenLayer.GBO` |
| Top / bottom solder mask | `Gerber_TopSolderMaskLayer.GTS` / `Gerber_BottomSolderMaskLayer.GBS` |
| Bottom paste mask | `Gerber_BottomPasteMaskLayer.GBP` |
| Board outline | `Gerber_BoardOutlineLayer.GKO` |
| Document / drill drawing | `Gerber_DocumentLayer.GDL` / `Gerber_DrillDrawingLayer.GDD` |
| Drill files (PTH / NPTH / vias) | `Drill_PTH_Through.DRL` / `Drill_NPTH_Through.DRL` / `Drill_PTH_Through_Via.DRL` |
| Flying probe test | `FlyingProbeTesting.json` |

- **Layers**: 2
- **Board size**: about **90.1 × 70.1 mm** (measured from the outline gerber)

### How to order

1. Open the [JLCPCB order page](https://www.jlc.com/) and choose **PCB**
2. **Drag the whole zip in** — do not unzip it first
3. Layer count is detected as **2**; 1.6 mm thickness is recommended, everything else can stay default
4. Use the built-in gerber viewer to double-check outline and silkscreen before paying

Official guide: <https://prodocs.lceda.cn/cn/pcb/order-order-pcb/index.html>
(the `PCB下单必读.txt` inside the zip just points to this link)

> Revisions may change between batches — **verify hole positions and outline in the gerber preview before ordering**.

## 3D printed parts

| File | Part | Size |
|---|---|---|
| `bottom_plate_150x80x4.stl` | bottom plate | 150 × 80 × 4 mm |
| `top_plate_150x80x3.stl` | top plate | 150 × 80 × 3 mm |

- Units are **mm**, binary STL — drop straight into Cura / OrcaSlicer / PrusaSlicer
- Suggested settings: **PLA, 0.2 mm layer height, 30–40 % infill, no supports**
- The bottom plate carries the load; thicken it or raise infill to 60 % if it flexes

> ⚠️ **The PCB and the 3D parts are different components**: the driver board is about 90 × 70 mm and mounts on the 150 × 80 mm bottom plate. Check the mounting holes yourself before printing.
