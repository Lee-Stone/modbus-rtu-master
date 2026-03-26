# modbus-host-computer

基于 Qt 6 开发的 Modbus RTU 上位机，支持串口连接配置、保持寄存器与线圈的读写操作、通信日志、串口热插拔检测及中英文界面切换。

****

**⭐ 欢迎提出 Issues 和 PR，如果这个项目对你有帮助，请给个 Star！**

## 📑 目录

- [📖 项目简介](#-项目简介)
- [📁 项目结构](#-项目结构)
- [🔧 开发环境](#-开发环境)
- [▶️ 编译与运行](#️-编译与运行)
- [🎮 使用说明](#-使用说明)
- [📧 联系方式](#-联系方式)

## 📖 项目简介

这是一个基于 **Qt 6 / C++** 开发的 **Modbus RTU 上位机（Master）** 工具，通过 RS-485 / USB 转串口适配器与 Modbus 从站设备通信，适用于工业控制、传感器调试等场景。

**主要功能：**

- **串口设置**

  - ✅ 自动枚举并刷新可用串口（1 秒定时扫描）
  - ✅ 支持波特率 / 数据位 / 校验位 / 停止位 / 从站地址配置
  - ✅ 串口热插拔检测：设备移除后自动断开连接

- **保持寄存器（Holding Register）**

  - ✅ FC03 批量读取，支持自定义起始地址和数量
  - ✅ FC06 / FC16 写入，支持单值、逗号分隔多值、0x 十六进制格式
  - ✅ 结果实时显示在表格中（地址 / 十进制 / 十六进制）

- **线圈 / 离散输入（Coil）**

  - ✅ FC01 批量读取线圈状态
  - ✅ FC05 写入单线圈（0 / 1）
  - ✅ 结果实时显示在表格中

- **通信日志**

  - ✅ 带时间戳的操作记录（成功 / 失败 / 断开均有提示）
  - ✅ 一键清空日志

- **界面**

  - ✅ 中文 / 英文界面一键切换
  - ✅ 状态指示灯（绿色已连接 / 红色未连接 / 橙色连接中）
  - ✅ 自定义应用图标

## 📁 项目结构

```
modbus-host-computer/
├── icons/
│   └── app.svg                 # 应用图标（SVG）
├── build/                      # 构建输出目录（Qt Creator 影子构建）
├── main.cpp                    # 程序入口，设置应用图标
├── widget.h                    # 主窗口头文件
├── widget.cpp                  # 主窗口逻辑实现
├── widget.ui                   # Qt Designer UI 布局文件
├── modbus-host-computer.pro    # qmake 项目配置文件
├── resources.qrc               # Qt 资源文件（图标）
├── README.md
└── .gitignore
```

## 🔧 开发环境

| 组件 | 版本 |
|------|------|
| Qt | 6.11.0 |
| 编译器 | MinGW 13.1.0 64-bit |
| 构建工具 | qmake + mingw32-make |
| IDE | Qt Creator |
| 操作系统 | Windows 10 / 11 |

**Qt 模块依赖：**

```
QT += widgets serialbus serialport
```

> 需要在 Qt 维护工具（Qt Maintenance Tool）中安装 **Qt Serial Bus** 组件。

## ▶️ 编译与运行

### 使用 Qt Creator

1. 克隆仓库：

   ```bash
   git clone https://github.com/YourUsername/modbus-host-computer.git
   ```

2. 用 Qt Creator 打开 `modbus-host-computer.pro`

3. 选择套件 **Desktop Qt 6.x.x MinGW 64-bit**

4. 点击 **Build → Build Project** 或按 `Ctrl+B`

5. 运行：`Ctrl+R`

### 使用命令行

```bash
cd modbus-host-computer
mkdir build && cd build
qmake ../modbus-host-computer.pro -spec win32-g++ "CONFIG+=debug"
mingw32-make -j4
```

### 配置 exe 图标（可选）

若需要在 Windows 资源管理器中显示自定义图标，将 `icons/app.svg` 导出为 `icons/app.ico`，然后在 `modbus-host-computer.pro` 中添加：

```
RC_ICONS = icons/app.ico
```

## 🎮 使用说明

### 连接从站

1. 将 USB 转 RS-485（或 RS-232）适配器插入 PC
2. 在 **端口** 下拉框中选择对应 COM 口（如 COM3）
3. 根据从站配置设置波特率、数据位、校验位、停止位和从站地址
4. 点击 **连接**，状态指示灯变绿表示成功

### 读取保持寄存器

1. 填写 **起始地址** 和 **数量**
2. 点击 **读取**，结果显示在下方表格

### 写入保持寄存器

1. 填写起始地址
2. 在 **写入值** 框中输入值（支持以下格式）：

   | 格式 | 示例 |
   |------|------|
   | 十进制单值 | `100` |
   | 十六进制单值 | `0x0064` |
   | 逗号分隔多值 | `10,20,30` |

3. 点击 **写入**

### 读/写线圈

- **读取线圈**：填写起始地址和数量，点击 **读取线圈**
- **写入线圈**：填写地址，设置写入值（0 或 1），点击 **写入线圈**

### 中英文切换

点击界面右上角的 **EN / 中** 按钮切换界面语言。

## 📧 联系方式

- **GitHub**：[https://github.com/CaddonThaw](https://github.com/CaddonThaw)
- **Bilibili**：[https://space.bilibili.com/CaddonThaw](https://space.bilibili.com)
