# UE5 虚实双向联动配置文档

## 概述

本文档说明如何配置 UE5 虚拟仿真平台与实车之间的双向数据通信。

## 系统架构

```
┌─────────────────────────────────────┐
│     UE5 虚拟仿真平台 (Windows)      │
│   CyberLubanTwin + ROSIntegration   │
└──────────────┬──────────────────────┘
               │ rosbridge_suite
               │ (WebSocket 9090)
               │
┌──────────────┴──────────────────────┐
│      UE5 Bridge Node (ROS2)         │
│   虚实双向桥接 + 话题映射转换        │
└──────────────┬──────────────────────┘
               │
┌──────────────┴──────────────────────┐
│   实车机器人 (Ubuntu 22.04)         │
│  底盘 + 雷达 + 相机 + 感知算法      │
└─────────────────────────────────────┘
```

## 数据流向

### 虚拟端 → 实车（控制指令）

- **UE5 话题**：`/ue5/command` (UE5Command)
- **实车话题**：`/cmd_vel` (geometry_msgs/Twist)
- **桥接节点**：`ue5_bridge_node` 订阅 `/ue5/command`，解析控制模式，转发速度指令到 `/cmd_vel`

### 实车 → 虚拟端（状态同步）

- **实车话题**：
  - `/odom` (nav_msgs/Odometry) - 机器人位姿
  - `/livox/pointcloud` (sensor_msgs/PointCloud2) - 激光雷达点云
  - `/perception/fused_defects` (cri_msgs/DefectArray) - 检测到的路面缺陷
  
- **UE5 话题**：`/ue5/robot_state` (UE5State)
- **桥接节点**：`ue5_bridge_node` 聚合实车状态，以 10Hz 频率发布到 `/ue5/robot_state`

## 网络配置

### 前提条件

1. **实车端 (Ubuntu)** 和 **UE5 虚拟端 (Windows)** 必须在同一局域网内
2. 确保防火墙允许 **9090 端口** (rosbridge WebSocket) 通信
3. 推荐使用有线网络以减少延迟

### 实车端配置 (Ubuntu)

1. 安装 rosbridge_suite：

```bash
sudo apt install ros-humble-rosbridge-suite
```

2. 启动 rosbridge_server：

```bash
ros2 launch rosbridge_server rosbridge_websocket_launch.xml
```

3. 获取实车端 IP 地址：

```bash
ip addr show | grep inet
# 例如: 192.168.1.100
```

### UE5 虚拟端配置 (Windows)

1. 打开 UE5 工程 `CyberLubanTwin.uproject`

2. 在 ROSIntegration 插件设置中：
   - **ROS Bridge Server Host**: 填写实车端 IP（如 `192.168.1.100`）
   - **ROS Bridge Server Port**: `9090`
   - **Protocol**: `ws://` (WebSocket)

3. 配置 UE5 订阅和发布话题：
   - 订阅 `/ue5/robot_state` 用于接收实车状态
   - 发布 `/ue5/command` 用于发送控制指令

## 启动步骤

### 1. 启动实车端 ROS2 系统

```bash
# Terminal 1: 启动 rosbridge
ros2 launch rosbridge_server rosbridge_websocket_launch.xml

# Terminal 2: 启动完整系统
cd ~/campus_road_inspection_ws
source install/setup.bash
ros2 launch cri_bringup full_system.launch.py
```

### 2. 启动 UE5 虚拟端

1. 双击打开 `CyberLubanTwin.uproject`
2. 确认 ROSIntegration 已连接到实车端 rosbridge
3. 点击 Play 开始运行虚拟场景

### 3. 验证连接

在实车端运行：

```bash
# 查看 UE5 发布的话题
ros2 topic echo /ue5/command

# 查看桥接节点发布给 UE5 的状态
ros2 topic echo /ue5/robot_state
```

## 话题映射表

| 虚拟端话题 | 实车端话题 | 消息类型 | 方向 | 频率 | 说明 |
|-----------|-----------|----------|------|------|------|
| `/ue5/command` | `/cmd_vel` | UE5Command → Twist | UE5→实车 | 按需 | 速度控制指令 |
| `/ue5/robot_state` | 聚合 | UE5State | 实车→UE5 | 10Hz | 机器人完整状态 |
| - | `/odom` | Odometry | 实车→UE5 | 20Hz | 里程计数据 |
| - | `/livox/pointcloud` | PointCloud2 | 实车→UE5 | 10Hz | 激光雷达点云 |
| - | `/perception/fused_defects` | DefectArray | 实车→UE5 | 1Hz | 缺陷检测结果 |

## 常见问题

### 1. UE5 无法连接到 rosbridge

**症状**：UE5 控制台显示 "ROSBridge connection failed"

**解决方案**：
- 检查实车端 rosbridge_server 是否正常运行
- 检查防火墙是否允许 9090 端口
- 确认 UE5 配置的 IP 地址正确

### 2. 数据延迟过高

**症状**：UE5 显示的机器人位置滞后明显

**解决方案**：
- 使用有线网络代替 WiFi
- 降低点云发布频率或分辨率
- 检查网络带宽占用情况

### 3. 控制指令无响应

**症状**：在 UE5 中发送控制指令，实车不动

**解决方案**：
- 检查 `ue5_bridge_node` 是否正常运行
- 检查底盘驱动节点是否正常接收 `/cmd_vel`
- 确认 UE5Command 的 `control_mode` 设置为 `manual`

## 开发调试

### 查看所有话题

```bash
ros2 topic list
```

### 监听特定话题

```bash
ros2 topic echo /ue5/command
ros2 topic echo /ue5/robot_state --once
```

### 手动发送测试指令

```bash
# 从命令行发送速度指令（模拟 UE5 控制）
ros2 topic pub /ue5/command cri_msgs/msg/UE5Command "{
  header: {stamp: {sec: 0, nanosec: 0}, frame_id: ''},
  velocity_command: {linear: {x: 0.5, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}},
  control_mode: 'manual'
}"
```

## 性能优化建议

1. **点云降采样**：在发送到 UE5 之前，对点云进行降采样，减少网络带宽占用
2. **按需发送**：只在 UE5 需要时发送大体积数据（如点云）
3. **消息压缩**：启用 rosbridge 的消息压缩功能
4. **QoS 配置**：根据实际需求配置话题的 QoS（Best Effort vs Reliable）

## 安全注意事项

1. **紧急停止**：UE5 应提供紧急停止按钮，发送 `control_mode: 'emergency_stop'` 指令
2. **超时保护**：如果一定时间内未收到 UE5 指令，底盘驱动应自动停止
3. **权限管理**：生产环境中应启用 ROS2 安全认证，防止未授权控制

## 扩展阅读

- [ROSIntegration Plugin 文档](https://github.com/code-iai/ROSIntegration)
- [rosbridge_suite 文档](https://github.com/RobotWebTools/rosbridge_suite)
- [ROS2 Humble 文档](https://docs.ros.org/en/humble/)
