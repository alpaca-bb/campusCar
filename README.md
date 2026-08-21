# CampusCar - 智能校园小车项目

> 基于 ROS2 + STM32 + UE5 的智能小车控制系统  
> 项目状态：**开发中** | 更新时间：2026-08-21

<div align="center">

[![ROS2](https://img.shields.io/badge/ROS2-Humble-blue)](https://docs.ros.org/en/humble/)
[![STM32](https://img.shields.io/badge/STM32-H723VGT6-green)](https://www.st.com/en/microcontrollers-microprocessors/stm32h723vg.html)


</div>

---

## 📋 目录

- [项目简介](#-项目简介)
- [系统架构](#-系统架构)
- [功能特性](#-功能特性)
- [运行环境](#-运行环境)
- [安装配置](#-安装配置)
- [使用说明](#-使用说明)
- [项目结构](#-项目结构)
- [开发文档](#-开发文档)
- [常见问题](#-常见问题)

---

## 🚀 项目简介

CampusCar 是一个基于 **ROS2 Humble** 和 **STM32H723VGT6** 的智能小车控制系统，支持：

- 🎮 **多种控制方式**：UE5远程控制、ROS2键盘控制、GUI控制台
- 📡 **模块化硬件接口**：电调(ESC)、舵机、海康相机、RTK GPS
- 🔄 **双向通信协议**：STM32 ↔ NUC (UART) + NUC ↔ UE5 (WebSocket)
- 🎥 **视觉反馈控制**：支持相机视野自动调整舵机云台角度
- 🗺️ **GPS航点导航**：基于Stanley控制器的路径跟踪

**典型应用场景：**
- 校园巡逻机器人
- 自主导航小车
- 远程遥控平台
- 多传感器融合实验平台

---

## 🏗️ 系统架构

### 完整控制链路

```
┌─────────────┐   WebSocket    ┌──────────────┐    ROS2 DDS    ┌─────────────┐
│  UE5 客户端  │ ═══════════════>│ rosbridge    │ ═══════════════>│ ROS2 节点   │
│  (WASD控制) │  (端口 9090)    │  (NUC上)     │                │ (/cmd_vel)  │
└─────────────┘                 └──────────────┘                └─────────────┘
                                                                       │
                                                                       ▼
┌─────────────┐   8字节协议     ┌──────────────┐   PWM 50Hz      ┌─────────────┐
│ Hoverboard  │ ════════════════>│  STM32H723   │ ═══════════════>│  电调+电机  │
│  Driver     │   UART 115200   │  (UART7控制) │  TIM1 CH1/CH3  │  (4WD底盘)  │
└─────────────┘                 └──────────────┘                 └─────────────┘
      ▲                                  │
      │                                  │ TIM2 PWM
      │ 16字节反馈                        ▼
      └──────────────────────────┌─────────────┐
                                 │ MG996R舵机  │ (云台俯仰角控制)
                                 └─────────────┘
```

### 数据流向

```
控制命令:  UE5 → rosbridge → /cmd_vel → Hoverboard Driver → STM32 → 电调 → 底盘
视觉反馈:  相机 → 目标检测 → 角度计算 → /servo_angle → STM32 → 舵机
状态反馈:  STM32 → NUC → /odom, /imu → ROS2生态系统
```

---

## ✨ 功能特性

### 已实现功能 ✅

| 模块 | 功能 | 状态 |
|------|------|------|
| **STM32固件** | UART7串口直控电调 | ✅ 已测试 |
| **STM32固件** | TIM2舵机PWM控制(50Hz) | ✅ 已测试 |
| **STM32固件** | 8字节hoverboard协议(XOR校验) | ✅ 已测试 |
| **ROS2驱动** | Hoverboard Driver (ros2_control) | ✅ 代码完成 |
| **ROS2驱动** | rosbridge_server (端口9090) | ✅ 已测试 |
| **控制界面** | 键盘控制 (WASD) | ✅ 代码完成 |
| **控制界面** | Car GUI (PyQt5) | ✅ 代码完成 |
| **视频流** | MJPEG Server (端口8080) | ✅ 代码完成 |
| **视频流** | RTSP Server (端口8554) | ✅ 代码完成 |
| **视觉控制** | 舵机视觉反馈控制节点 | ✅ 代码完成 |

### 开发中功能 🚧

- [ ] NUC ↔ STM32 全链路联调
- [ ] 海康相机 (MV-CS016-10GC) 接入
- [ ] RTK GPS 航点导航实测
- [ ] UE5 远程控制对接
- [ ] 多传感器融合

---

## 💻 运行环境

### 硬件要求

| 硬件 | 型号/规格 | 用途 |
|------|----------|------|
| **主控板** | STM32H723VGT6 (达妙 DM-MC02) | 底层控制 |
| **计算平台** | Intel NUC (Ubuntu 22.04) | ROS2运行环境 |
| **底盘** | 4WD底盘 | 移动平台 |
| **电调** | USART模式电调 | 电机驱动 |
| **舵机** | MG996R (数字舵机) | 云台俯仰控制 |
| **相机** | 海康 MV-CS016-10GC (可选) | 机器视觉 |
| **GPS** | RTK GPS模块 (可选) | 精确定位 |

### 软件环境

#### NUC端 (ROS2主机)

```bash
操作系统: Ubuntu 22.04 LTS
ROS版本:  ROS2 Humble
Python:   3.10+
依赖库:   
  - ros-humble-ros2-control
  - ros-humble-rosbridge-suite
  - python3-serial
  - python3-opencv
  - python3-pyqt5
```

#### STM32端 (嵌入式开发)

```bash
开发工具: STM32CubeIDE 1.13+
工具链:   ARM GCC
调试器:   ST-Link V2/V3
烧录方式: SWD
```

#### 客户端 (可选)

```bash
UE5:      Unreal Engine 5.x
Web浏览器: Chrome/Edge (WebSocket支持)
```

---

## 📦 安装配置

### 1. NUC端安装 (ROS2环境)

#### 1.1 安装ROS2 Humble

```bash
# 添加ROS2源
sudo apt update && sudo apt install -y curl gnupg lsb-release
sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
  -o /usr/share/keyrings/ros-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) \
  signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] \
  http://packages.ros.org/ros2/ubuntu $(lsb_release -cs) main" | \
  sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null

# 安装ROS2
sudo apt update
sudo apt install -y ros-humble-desktop
sudo apt install -y ros-humble-ros2-control ros-humble-ros2-controllers
sudo apt install -y ros-humble-rosbridge-suite
```

#### 1.2 克隆项目

```bash
cd ~
git clone https://github.com/qyubo/CampusCar.git
cd CampusCar/campusCar-hardware-new-stm32-hikrobot
```

#### 1.3 安装依赖

```bash
# Python依赖
sudo apt install -y python3-pip python3-opencv
pip3 install pyserial PyQt5

# 相机驱动 (可选)
sudo apt install -y ros-humble-camera-aravis2

# 串口权限
sudo usermod -aG dialout $USER
# 注销后重新登录生效
```

#### 1.4 配置环境

```bash
# 编辑配置文件
cd ~/CampusCar/campusCar-hardware-new-stm32-hikrobot
nano config/robot.env

# 修改串口设备 (根据实际情况调整)
export STM32_DEVICE="/dev/ttyUSB0"  # NUC连接STM32的串口

# 加载配置
source config/robot.env
source /opt/ros/humble/setup.bash
```

---

### 2. STM32端编译烧录

#### 2.1 打开项目

```bash
# Windows下打开STM32CubeIDE
文件 → 打开项目 → 选择 CampusCar_STM32H723VGT6/
```

#### 2.2 编译固件

```bash
# 方法1: IDE内编译
Project → Build Project (Ctrl+B)

# 方法2: 命令行编译 (需安装arm-none-eabi-gcc)
cd CampusCar_STM32H723VGT6
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```

#### 2.3 烧录程序

```bash
# 连接ST-Link到STM32开发板
# IDE内: Run → Debug (F11) 或 Run (Ctrl+F11)
```

#### 2.4 验证烧录

上电后应观察到：
- ✅ PC13 LED闪烁4次 (启动指示)
- ✅ PC14 LED持续闪烁 (主循环运行)
- ✅ 舵机执行测试序列后持续扫描 (20°~160°)

---

## 📖 使用说明

### 快速启动流程

#### 方式1: 一键启动全栈 (推荐)

```bash
cd ~/CampusCar/campusCar-hardware-new-stm32-hikrobot

# 1. 启动全部服务
./scripts/launch_all.sh

# 等待启动完成后，另开终端
# 2. 启动键盘控制
./scripts/keyboard_control.sh

# 或启动GUI控制
./scripts/open_car_gui.sh
```

**键盘控制按键：**
- `W` - 前进
- `S` - 后退
- `A` - 左转
- `D` - 右转
- `空格` - 停止
- `Q` - 退出

---

#### 方式2: 手动串口测试 (调试用)

```bash
# 测试STM32串口通信
cd ~/CampusCar/campusCar-hardware-new-stm32-hikrobot
python3 scripts/test_hoverboard_uart.py

# 按提示输入命令:
# f - 前进
# b - 后退
# l - 左转
# r - 右转
# s - 停止
```

---

#### 方式3: ROS2话题控制

```bash
# 发布速度命令
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5}, angular: {z: 0.0}}"

# 查看反馈数据
ros2 topic echo /odom
```

---

### 视觉舵机控制 (可选)

启用相机视野自动调整舵机角度：

```bash
# 1. 启动相机 (需先接线)
./scripts/hikrobot_camera_start.sh

# 2. 启动舵机视觉控制节点
./scripts/servo_vision_start.sh

# 舵机会根据画面中目标位置自动调整角度
```

---

### UE5远程控制 (可选)

```bash
# 1. 确保rosbridge已启动
ros2 run rosbridge_server rosbridge_websocket

# 2. UE5客户端连接
WebSocket地址: ws://<NUC_IP>:9090

# 3. 发布控制命令到话题
/U2RTopic_Command
```

---

## 📂 项目结构

```
CampusCar/
├── CampusCar_STM32H723VGT6/              # STM32固件工程
│   ├── Core/
│   │   ├── Inc/
│   │   │   ├── main.h
│   │   │   ├── hoverboard_usart_app_example.h
│   │   │   ├── hoverboard_usart_controller.h
│   │   │   ├── servo_controller.h
│   │   │   └── esc_pwm_controller.h
│   │   └── Src/
│   │       ├── main.c                    # 主程序(协议解析+控制逻辑)
│   │       ├── hoverboard_usart_controller.c  # UART7电调控制
│   │       ├── servo_controller.c         # 舵机控制模块
│   │       ├── tim.c                      # TIM1/TIM2 PWM配置
│   │       └── usart.c                    # UART1/UART7配置
│   ├── CMakeLists.txt
│   └── WASD_TEST_GUIDE.md                # 测试指南
│
├── campusCar-hardware-new-stm32-hikrobot/  # ROS2驱动工程
│   ├── config/
│   │   ├── robot.env                     # 主配置文件 ⭐
│   │   └── profiles/
│   │       └── stm32_hoverboard_4wd.env
│   │
│   ├── hardware/
│   │   └── hoverboard_driver/            # ROS2 Control驱动
│   │       ├── hardware/
│   │       │   ├── include/hoverboard_driver/
│   │       │   │   ├── protocol.hpp      # 通信协议定义 ⭐
│   │       │   │   └── hoverboard_driver.hpp
│   │       │   └── hoverboard_driver.cpp # 驱动实现
│   │       └── bringup/
│   │           ├── config/hoverboard_controllers.yaml
│   │           └── launch/diffbot.launch.py
│   │
│   ├── scripts/                          # 启动脚本 ⭐
│   │   ├── launch_all.sh                 # 一键启动
│   │   ├── stop_all.sh                   # 停止所有服务
│   │   ├── keyboard_control.sh           # 键盘控制
│   │   ├── open_car_gui.sh               # GUI控制台
│   │   ├── servo_vision_start.sh         # 视觉舵机控制
│   │   └── test_hoverboard_uart.py       # 串口测试工具
│   │
│   ├── src/                              # Python应用
│   │   ├── car_gui.py                    # PyQt5控制界面
│   │   ├── keyboard_control.py           # 键盘控制节点
│   │   ├── servo_vision_controller.py    # 舵机视觉控制
│   │   ├── ue_bridge.py                  # UE5数据桥接
│   │   ├── mjpeg_server.py               # MJPEG视频流
│   │   └── rtk_tools/                    # RTK GPS工具集
│   │       ├── gps_navigator.py          # GPS航点导航
│   │       └── path_recorder.py          # 路径录制
│   │
│   └── docs/                             # 项目文档
│       ├── 快速启动指南.md
│       └── UE对接文档.md
│
└── 各项说明/                              # 开发文档集合
    ├── STM32H723_USART控制源码使用教程.md
    ├── 串口协议说明.md
    ├── PWM切换组合参数使用说明.md
    └── PROJECT_SUMMARY.md                # 项目总结
```

---

## 📚 开发文档

详细文档请查看 `各项说明/` 目录：

| 文档 | 说明 |
|------|------|
| [STM32H723_USART控制源码使用教程.md](各项说明/STM32H723_USART控制源码使用教程.md) | STM32固件详细说明 |
| [串口协议说明.md](各项说明/串口协议说明.md) | UART通信协议定义 |
| [串口直控方案说明.md](各项说明/串口直控方案说明.md) | UART7电调控制方案 |
| [PWM切换组合参数使用说明.md](各项说明/PWM切换组合参数使用说明.md) | 电调初始化手势 |
| [PROJECT_SUMMARY.md](各项说明/PROJECT_SUMMARY.md) | 项目完整总结 |
| [WASD_TEST_GUIDE.md](CampusCar_STM32H723VGT6/WASD_TEST_GUIDE.md) | WASD控制测试指南 |

---

## ❓ 常见问题

### Q1: NUC无法识别USB转TTL设备

```bash
# 检查设备
ls /dev/ttyUSB*

# 如果没有输出，安装驱动
sudo modprobe ch341
sudo modprobe ftdi_sio

# 检查权限
sudo usermod -aG dialout $USER
# 注销后重新登录
```

### Q2: STM32烧录后LED不闪烁

- 检查供电是否正常 (5V/3.3V)
- 确认芯片型号为STM32H723VGT6
- 检查启动模式跳线 (BOOT0=0)
- 使用STM32CubeProgrammer重新烧录

### Q3: 电调初始化失败(无滴滴声)

- 确认PWM信号频率为50Hz
- 检查电调接线: PE9→CH2(速度), PE13→CH1(转向)
- 手动执行初始化手势 (上下左右快速扳动)
- 参考: [PWM切换组合参数使用说明.md](各项说明/PWM切换组合参数使用说明.md)

### Q4: rosbridge连接失败

```bash
# 检查rosbridge是否启动
ros2 node list | grep rosbridge

# 手动启动
ros2 run rosbridge_server rosbridge_websocket --ros-args -p port:=9090

# 测试连接
curl http://localhost:9090
```

### Q5: 舵机抖动或不动

- 检查TIM2 PWM输出 (PA0, 50Hz)
- 确认舵机供电独立 (5V ≥ 1A)
- 检查信号线连接和接地
- 用示波器测量脉宽: 0.5ms~2.5ms

---

## 🤝 贡献指南

欢迎提交Issue和Pull Request！

开发分支规范：
- `main` - 稳定版本
- `dev` - 开发版本
- `feature/*` - 新功能分支
- `fix/*` - Bug修复分支

---

---

## 📞 联系方式

- **项目维护**: qyubo
- **GitHub**: https://github.com/qyubo/CampusCar
- **问题反馈**: [Issues](https://github.com/qyubo/CampusCar/issues)

---

<div align="center">

**⭐ 如果这个项目对你有帮助，请给个Star支持一下！**

Made with ❤️ by CampusCar Team

</div>
