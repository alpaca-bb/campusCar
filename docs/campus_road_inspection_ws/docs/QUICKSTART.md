# 系统启动指南

## 前置依赖

### Ubuntu 22.04 端

```bash
# 安装 ROS2 Humble
# 参考: https://docs.ros.org/en/humble/Installation/Ubuntu-Install-Debians.html

# 安装 rosbridge_suite（用于 UE5 通信）
sudo apt install ros-humble-rosbridge-suite

# 安装 Python 依赖
sudo apt install python3-serial python3-pip

# 安装 colcon 编译工具
sudo apt install python3-colcon-common-extensions
```

## 编译工作空间

```bash
cd ~/campus_road_inspection_ws
colcon build --symlink-install
source install/setup.bash
```

## 启动系统

### 方式一：一键启动完整系统

```bash
# Terminal 1: 启动 rosbridge（用于 UE5 通信）
ros2 launch rosbridge_server rosbridge_websocket_launch.xml

# Terminal 2: 启动完整系统
ros2 launch cri_bringup full_system.launch.py
```

### 方式二：分模块启动

```bash
# 仅启动驱动层
ros2 launch cri_bringup drivers.launch.py

# 仅启动 UE5 桥接
ros2 launch ue5_bridge ue5_bridge.launch.py
```

## 验证系统

```bash
# 查看所有运行中的节点
ros2 node list

# 查看所有话题
ros2 topic list

# 查看底盘里程计
ros2 topic echo /odom

# 查看雷达点云
ros2 topic echo /livox/pointcloud --once

# 查看 UE5 状态同步
ros2 topic echo /ue5/robot_state
```

## UE5 虚拟端配置

详见 [UE5_BRIDGE.md](./UE5_BRIDGE.md)

## 当前状态说明

### 已实现（占位骨架）

- ✅ 自定义消息与服务定义 (`cri_msgs`)
- ✅ 底盘驱动节点 (`chassis_driver`)
- ✅ Livox 雷达驱动节点 (`livox_driver`)
- ✅ 海康相机驱动节点 (`hikrobot_camera`)
- ✅ UE5 双向桥接节点 (`ue5_bridge`)
- ✅ 系统启动 Launch 文件

### 待完整实现

- ⚠️ 感知检测层节点（激光缺陷检测、视觉缺陷检测、多传感器融合）
- ⚠️ 核心算法层节点（世界模型、动态地图）
- ⚠️ 应用层节点（自主导航、多机服务接口）
- ⚠️ 硬件 SDK 对接（Livox SDK、海康 SDK、STM32 串口通信协议）

## 下一步开发建议

1. **对接硬件 SDK**：在各驱动节点中集成实际的硬件 SDK
2. **实现感知算法**：补充激光和视觉缺陷检测的具体算法逻辑
3. **集成 Nav2**：在 `cri_navigation` 包中配置 Nav2 导航栈
4. **测试 UE5 通信**：在局域网内测试虚实双向数据流
5. **性能优化**：根据实际运行情况优化话题频率和数据带宽

## 技术支持

如有问题，请检查：
1. ROS2 环境是否正确 source
2. 所有依赖包是否已安装
3. 硬件设备是否正常连接
4. 网络配置是否正确（UE5 通信）
