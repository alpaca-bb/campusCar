# 🚀 5分钟快速开始

> 快速部署和运行 CyberLuban 校园智能巡检机器人系统

## 前置条件

### 硬件要求
- ✅ Ubuntu NUC（机器人端）
- ✅ Windows 电脑（校园大脑）
- ✅ 海康相机（已连接到 NUC）
- ✅ 两台设备在同一局域网

### 软件要求
- Ubuntu: ROS2 Jazzy + Python 3.10+
- Windows: UE5 + ROSIntegration 插件

---

## 第一步：启动机器人端（Ubuntu NUC）

### 1.1 打开终端 1 - 启动 ROSBridge

```bash
cd ~/CyberLuban/robot/ros2_workspace
source install/setup.bash
ros2 launch rosbridge_server rosbridge_websocket_launch.xml
```

**预期输出**：
```
[INFO] [rosbridge_websocket]: Rosbridge WebSocket server started on port 9090
```

### 1.2 打开终端 2 - 启动机器人系统

```bash
cd ~/CyberLuban/robot/ros2_workspace
source install/setup.bash
ros2 launch cri_bringup full_system.launch.py
```

**预期输出**：
```
[INFO] [hikrobot_camera]: Connected to camera 10.7.142.102
[INFO] [chassis_driver]: Chassis initialized
[INFO] [ue5_bridge]: Publishing robot state at 10Hz
```

### 1.3 获取 NUC 的 IP 地址

```bash
hostname -I
```

**记下输出的第一个 IP**，例如：`10.7.142.74`

---

## 第二步：启动校园大脑（Windows UE5）

### 2.1 打开 UE5 项目

双击打开：
```
CyberLuban\brain\ue5_project\CampusBrain.uproject
```

### 2.2 配置 ROS 连接

1. 在 UE5 编辑器中，找到 **Content Browser**
2. 打开 `Content/Blueprints/ROSConnection`
3. 设置参数：
   - **ROS Bridge Host**: `10.7.142.74`（你的 NUC IP）
   - **ROS Bridge Port**: `9090`
   - **Connection Type**: `WebSocket`

### 2.3 运行 UE5

点击工具栏的 **Play** 按钮（或按 `Alt+P`）

---

## 第三步：验证系统

### 3.1 在 NUC 上验证

```bash
# 查看运行的节点
ros2 node list

# 查看话题列表
ros2 topic list

# 查看机器人状态（应该有数据更新）
ros2 topic echo /ue5/robot_state --once

# 查看相机图像
ros2 topic echo /camera/image_raw --once
```

### 3.2 在 UE5 中验证

在 UE5 界面中，你应该看到：
- ✅ 机器人模型实时移动
- ✅ 相机画面实时显示
- ✅ 传感器数据可视化

### 3.3 测试控制

在 UE5 中操作控制面板，机器人应该响应指令移动

---

## 🎉 成功！

系统已经运行！你现在可以：

- 🎮 使用 UE5 界面控制真实机器人
- 📹 查看实时相机画面
- 🗺️ 进行路径规划和导航
- ⚠️ 实时检测路面缺陷

---

## ⚠️ 常见问题

### 问题 1：UE5 连接不上

**症状**：UE5 显示 "Disconnected"

**解决方案**：
```bash
# 检查 ROSBridge 是否运行
ros2 node list | grep rosbridge

# 检查端口
netstat -tuln | grep 9090

# 检查防火墙
sudo ufw allow 9090
```

### 问题 2：相机无图像

**症状**：`/camera/image_raw` 话题无数据

**解决方案**：
```bash
# 检查相机连接
ping 10.7.142.102

# 检查相机节点
ros2 node list | grep hikrobot

# 查看相机日志
ros2 node info /hikrobot_camera
```

### 问题 3：机器人不响应控制

**症状**：UE5 发送指令但机器人不动

**解决方案**：
```bash
# 检查底盘驱动
ros2 node list | grep chassis

# 手动测试
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.1}}"
```

---

## 📚 下一步

- 📖 [详细配置指南](robot_setup.md)
- 🔧 [相机配置](camera_setup.md)
- 🐛 [完整故障排查](troubleshooting.md)
- 🏗️ [系统架构](../architecture/system_overview.md)

---

**需要帮助？** 查看 [故障排查文档](troubleshooting.md)
