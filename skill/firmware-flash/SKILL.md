---
name: firmware-flash
description: Watchy S3 固件烧录流程。涵盖进入 Bootloader 模式、使用 Flash Download Tool 或 esptool 烧录 merged bin 固件、烧录后重启手表的完整操作。
---

# Watchy S3 固件烧录 / Firmware Flash

将编译好的固件（merged bin）烧录到 Watchy S3 手表的完整流程。

## 触发条件 / Trigger

- **手动**: 用户需要烧录新固件到 Watchy S3 手表。
- **关键词触发**:
  - "烧录固件" / "flash firmware"
  - "刷机" / "burn firmware"
  - "下载固件到手表" / "upload firmware"

## 前置条件 / Prerequisites

| 项目 | 要求 |
|---|---|
| 固件文件 | `*.ino.merged.bin` 格式（包含 bootloader + 分区表 + 应用），出厂默认固件位于 `Watchy-S3手表/固件烧录/7_SEG.ino.merged.bin` |
| 烧录地址 | `0x0` |
| 芯片类型 | ESP32-S3 |
| USB 连接 | Watchy S3 通过 USB Type-C 连接电脑，使用内置 USB JTAG/serial 接口 |
| 烧录工具（二选一） | **Flash Download Tool** (v3.9.7+) 或 **esptool** (pip install esptool) |

## 烧录流程 / Flash Procedure

### 第一步：进入 Bootloader 模式

Watchy S3 没有独立的 BOOT/RST 按钮，需通过手表按键进入：

1. **长按 Down 按钮 4 秒** — 手表关机
2. **再次按 Down 按钮** — 手表开机进入 Bootloader 模式
3. 此时电脑设备管理器中应出现 **USB JTAG/serial debug unit (COMx)** 端口
4. 如果未出现 COM 端口，重复步骤 1-2，或尝试拔插 USB 线

> **注意**：如果设备管理器中看不到 COM 端口，可能需要安装 ESP32-S3 USB 驱动。

### 第二步：烧录固件

#### 方式 A — Flash Download Tool（GUI 图形界面）

1. 下载并解压 Flash Download Tool（推荐 v3.9.7）
   - 下载地址：https://www.espressif.com.cn/zh-hans/support/download/other-tools
   - **不要放在中文目录下**，否则可能无法加载文件
2. 运行 `flash_download_tool_3.9.7.exe`
3. 启动设置：
   - **ChipType**: `ESP32-S3`
   - **WorkMode**: `Develop`
   - **LoadMode**: `UART`
   - 点击 `OK`
4. SPI Download 配置：
   - 第一行：点击 `...` 按钮，选择 bin 固件文件
   - **勾选**文件前面的复选框（必须勾选！）
   - 地址栏填写 `0x0`
   - **SPI SPEED**: `40MHz`（默认即可）
   - **SPI MODE**: `DIO`（默认即可，不要选 QOUT 或 DOUT）
   - **COM**: 选择设备管理器中对应的 COM 端口号
   - **BAUD**: `230400` 或更高
5. 点击 **START** 开始烧录
6. 等待进度条完成，状态显示 **FINISH**

#### 方式 B — esptool 命令行

```bash
esptool --chip esp32s3 --port COMx --baud 921600 write_flash 0x0 "path/to/firmware.merged.bin"
```

将 `COMx` 替换为实际端口号，`path/to/firmware.merged.bin` 替换为固件文件路径。

#### 方式 C — ESP Launchpad 浏览器在线烧录

1. 使用 Chrome 或 Edge 浏览器访问 https://espressif.github.io/esp-launchpad/
2. 切换到 **DIY** 页面
3. 点击 **Connect** 选择 COM 端口
4. Flash Address 填 `0x0`，选择 bin 文件
5. 点击 **Program** 开始烧录

### 第三步：烧录完成后重启手表

1. 在 Flash Download Tool 中点击 **STOP**
2. **长按 Down 按钮 4 秒** — 关机
3. **按 Down 按钮** — 开机
4. 手表正常启动，显示表盘即为烧录成功

## Watchy S3 按键定义

| 按键 | GPIO | 位置 | 功能 |
|---|---|---|---|
| Menu | GPIO 7 | 右上 | 菜单/确认 |
| Back | GPIO 6 | 右下 | 返回 |
| Up | GPIO 0 | 左上 | 向上/亮屏 |
| Down | GPIO 45 | 左下 | 向下/开关机/进入 Bootloader |

## 故障排除 / Troubleshooting

| 问题 | 解决方案 |
|---|---|
| 看不到 COM 端口 | 1) 确认 USB 线是数据线（非纯充电线）；2) 长按 Down 4s 关机后重新开机进入 Bootloader；3) 拔插 USB 线 |
| Flash Tool 报 "无法加载文件" | bin 文件路径不要包含中文字符 |
| 烧录后手表不断重启 | 固件版本不匹配，确认使用 Watchy S3 对应的固件 |
| esptool 连接超时 | 重新进入 Bootloader 模式后再试 |
| 烧录成功但屏幕不显示 | 按 Down 关机再开机；确认固件适配 200x200 电子墨水屏 |

## 出厂固件路径

```
Watchy-S3手表/固件烧录/7_SEG.ino.merged.bin    地址: 0x0
```

## 参考资料

- 烧录教程（飞书）: https://my.feishu.cn/wiki/Zpz4wXBtdimBrLk25WdcXzxcnNS
- Flash Download Tool 下载: https://www.espressif.com.cn/zh-hans/support/download/other-tools
- ESP Launchpad 在线烧录: https://espressif.github.io/esp-launchpad/
- Watchy 官方文档: https://watchy.sqfmi.com/docs/getting-started/
