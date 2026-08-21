# CampusCar

CampusCar 是“智行易——面向人机共生校园的路面环境评估机器人”项目的代码仓库，主要包含机器人底盘控制、相机与 RTK 接入、UE 数据联动、控制界面以及相关文档。

本仓库当前包含两部分核心内容：

- `CampusCar_STM32H723VGT6/`：STM32H723VGT6 底层固件工程，负责电调串口控制、舵机 PWM 控制、协议解析与安全保护。
- `campusCar-hardware-new-stm32-hikrobot/`：ROS2 上位机工程，负责底盘驱动、相机接入、视频流、UE 数据桥接、键盘/GUI 控制、RTK 工具与启动脚本。

---

## 项目简介

CampusCar 面向校园户外巡检场景，目标是为路面环境评估机器人提供稳定、可扩展的移动平台与感知控制基础。系统采用 STM32 作为底层实时控制核心，上位机采用 ROS2 负责任务编排、设备接入与数据联动，形成“底层执行 + 上层业务”的分层架构。

仓库中已实现的功能包括：底盘运动控制、舵机控制、串口协议通信、工业相机接入、视频流输出、键盘控制、GUI 控制、UE 数据桥接、RTK 工具及相关启动脚本。项目文档同时整理了协议说明、串口直控方案和 STM32 源码使用说明，便于后续开发与调试。

---

## 核心亮点

- **分层控制架构**：STM32 负责底层实时控制，ROS2 负责上层控制与业务调度。
- **串口直控底盘**：STM32 通过串口协议接收控制命令，驱动电调完成底盘运动。
- **舵机控制模块**：支持舵机 PWM 控制，可用于相机视野调整。
- **多种交互方式**：支持键盘控制、GUI 控制台和 UE 侧联动控制。
- **视频与感知接入**：提供 MJPEG、RTSP 视频流脚本，支持海康工业相机接入。
- **RTK 工具链**：包含路径录制、航点导航等 RTK 工具脚本。
- **配套文档完整**：协议、固件、启动方式、对接说明均已整理。

---

## 仓库结构

```text
CampusCar/
├── CampusCar_STM32H723VGT6/              # STM32 底层固件工程
├── campusCar-hardware-new-stm32-hikrobot/ # ROS2 上位机工程
├── 各项说明/                              # 详细开发文档
├── README.md
└── .gitignore
```

### 1) `CampusCar_STM32H723VGT6/`

STM32 固件工程，主要包含：

- `Core/Src/main.c`：主程序，负责协议解析、底盘控制和舵机控制
- `Core/Src/hoverboard_usart_controller.c`：电调串口控制
- `Core/Src/servo_controller.c`：舵机控制
- `Core/Src/tim.c`：PWM 定时器配置
- `Core/Src/usart.c`：串口配置
- `WASD_TEST_GUIDE.md`：底盘测试说明

### 2) `campusCar-hardware-new-stm32-hikrobot/`

ROS2 工程，主要包含：

- `hardware/hoverboard_driver/`：ROS2 Control 底盘驱动
- `src/keyboard_control.py`：键盘控制节点
- `src/car_gui.py`：控制界面
- `src/ue_bridge.py`：UE 数据桥接
- `src/mjpeg_server.py`：MJPEG 视频流
- `src/rtsp_server.py`：RTSP 视频流
- `src/servo_vision_controller.py`：舵机视觉控制节点
- `src/rtk_tools/`：RTK 工具集
- `scripts/`：启动、停止、探测、测试脚本
- `docs/`：快速启动和对接文档

---

## 运行环境

### 硬件

- **主控板**：STM32H723VGT6
- **上位机**：Intel NUC
- **底盘**：4WD 底盘
- **电调**：USART 模式电调
- **舵机**：MG996R
- **相机**：海康工业相机 MV-CS016-10GC（可选）
- **RTK**：RTK 模块（可选）

### 软件

#### NUC 端

- Ubuntu 22.04
- ROS2 Humble
- Python 3.10+
- 主要依赖：
  - `ros-humble-ros2-control`
  - `ros-humble-rosbridge-suite`
  - `python3-serial`
  - `python3-opencv`
  - `python3-pyqt5`

#### STM32 端

- STM32CubeIDE 1.13+
- ARM GCC
- ST-Link V2/V3
- SWD 烧录

---

## 安装方式

### 1. 克隆仓库

```bash
git clone https://github.com/qyubo/CampusCar.git
cd CampusCar
```

### 2. 安装 ROS2 环境

```bash
sudo apt update
sudo apt install -y curl gnupg lsb-release
sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
  -o /usr/share/keyrings/ros-archive-keyring.gpg

echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(lsb_release -cs) main" \
  | sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null

sudo apt update
sudo apt install -y ros-humble-desktop
sudo apt install -y ros-humble-ros2-control ros-humble-ros2-controllers
sudo apt install -y ros-humble-rosbridge-suite
```

### 3. 安装 Python 依赖

```bash
sudo apt install -y python3-pip python3-opencv
pip3 install pyserial PyQt5
```

### 4. 配置串口权限

```bash
sudo usermod -aG dialout $USER
```

登录后重新打开终端生效。

### 5. 配置运行参数

上位机默认配置在：

```bash
campusCar-hardware-new-stm32-hikrobot/config/robot.env
```

新底盘默认 profile 在：

```bash
campusCar-hardware-new-stm32-hikrobot/config/profiles/stm32_hoverboard_4wd.env
```

---

## 使用说明

### 1. 一键启动

```bash
cd campusCar-hardware-new-stm32-hikrobot
./scripts/launch_all.sh
```

`launch_all.sh` 会启动底盘驱动、相机、视频流、RTK 数据流、UE 数据桥和控制界面。

### 2. 键盘控制

```bash
./scripts/keyboard_control.sh
```

常用按键：

- `W`：前进
- `S`：后退
- `A`：左转
- `D`：右转
- `Space`：急停

也可以使用：

```bash
./scripts/open_car_gui.sh
```

打开控制界面进行操作。

### 3. STM32 串口测试

```bash
cd ../CampusCar_STM32H723VGT6
python3 ../campusCar-hardware-new-stm32-hikrobot/scripts/test_hoverboard_uart.py
```

### 4. 视觉舵机控制

```bash
cd ../campusCar-hardware-new-stm32-hikrobot
./scripts/servo_vision_start.sh
```

### 5. 查看服务

启动后会提供：

- rosbridge 端口：`9090`
- RTSP 视频流：`rtsp://<NUC_IP>:8554/robot_cam`
- HLS 视频流：`http://<NUC_IP>:8888/robot_cam/index.m3u8`
- 浏览器预览：`http://<NUC_IP>:8080/`

---

## 文档说明

详细资料见 `各项说明/`：

- `STM32H723_USART控制源码使用教程.md`
- `串口协议说明.md`
- `串口直控方案说明.md`
- `PWM切换组合参数使用说明.md`
- `PROJECT_SUMMARY.md`

以及：

- `CampusCar_STM32H723VGT6/WASD_TEST_GUIDE.md`
- `campusCar-hardware-new-stm32-hikrobot/docs/快速启动指南.md`
- `campusCar-hardware-new-stm32-hikrobot/docs/UE对接文档.md`

---

## 当前项目状态

- STM32 底层控制工程：已完成并可测试
- ROS2 底盘驱动：已整理并可启动
- 键盘控制与 GUI：已提供
- 视频流与 UE 数据桥：已提供
- 舵机控制：已提供
- RTK 工具：已提供

---

## 联系方式

- GitHub：https://github.com/qyubo/CampusCar

---

## 说明

本 README 仅根据当前仓库中已存在的代码、脚本和文档整理，不写未确认实现的功能。
