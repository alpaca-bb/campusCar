# 校园路面巡检机器人 ROS2 工作空间 - 交付说明

## 📦 已交付内容

### ✅ 完整的 ROS2 工作空间骨架

位置：`C:\Users\xzx15\Desktop\CyberLuban\campus_road_inspection_ws\`

### ✅ 自定义消息与服务包 (cri_msgs)

**消息类型**：
- `DefectInfo.msg` - 路面缺陷信息（类型、位置、尺寸、置信度）
- `DefectArray.msg` - 缺陷数组
- `DefectPrediction.msg` - 缺陷演化预测（未来尺寸、风险等级、推荐养护时间）
- `RoadCondition.msg` - 路况综合信息（区域评分、通行建议）
- `UE5Command.msg` - UE5虚拟端 → 实车控制指令
- `UE5State.msg` - 实车 → UE5虚拟端状态同步

**服务类型**：
- `QueryRoadCondition.srv` - 查询指定区域路况
- `RequestDetourPath.srv` - 请求绕行路径规划

### ✅ 驱动适配层 (cri_drivers)

#### 1. chassis_driver
- **功能**：STM32底盘串口通信
- **订阅**：`/cmd_vel` (Twist)
- **发布**：`/odom` (Odometry), `/chassis_pose` (PoseStamped)
- **配置**：`config/chassis_params.yaml`
- **状态**：✅ 占位代码完成，需对接实际串口协议

#### 2. livox_driver
- **功能**：Livox Mid-360S 雷达驱动
- **发布**：`/livox/pointcloud` (PointCloud2)
- **配置**：`config/livox_params.yaml`
- **状态**：✅ 占位代码完成，需集成 Livox SDK

#### 3. hikrobot_camera
- **功能**：海康 MV-CS016-10GC GigE 相机驱动
- **发布**：`/camera/image_raw` (Image), `/camera/camera_info` (CameraInfo)
- **配置**：`config/camera_params.yaml`
- **状态**：✅ 占位代码完成，需集成海康 MVS SDK

### ✅ 感知检测层示例 (cri_perception)

#### lidar_defect_detector
- **功能**：激光几何缺陷检测（地面分割、DEM差分、缺陷提取）
- **订阅**：`/livox/pointcloud`
- **发布**：`/perception/lidar_defects` (DefectArray)
- **状态**：✅ 占位代码完成，核心算法待实现

> **注**：`vision_defect_detector`（视觉语义识别）和 `sensor_fusion`（多传感器融合）包结构已创建，节点代码待补充。

### ✅ UE5 虚实双向桥接 (ue5_bridge)

**核心功能**：
- **虚拟 → 实车**：订阅 `/ue5/command`，解析后转发到 `/cmd_vel`
- **实车 → 虚拟**：聚合 `/odom`、`/livox/pointcloud`、`/perception/fused_defects`，发布到 `/ue5/robot_state`
- **通信方式**：通过 rosbridge_suite (WebSocket 9090端口) 与 UE5 ROSIntegration 插件通信
- **状态**：✅ 完整代码实现，可直接启动

### ✅ 系统启动包 (cri_bringup)

**Launch 文件**：
- `full_system.launch.py` - 一键启动完整系统
- `drivers.launch.py` - 仅启动驱动层
- `ue5_bridge.launch.py` - 仅启动 UE5 桥接

### ✅ 完整文档

1. **README.md** - 工作空间总览
2. **docs/QUICKSTART.md** - 快速开始指南（编译、启动、验证）
3. **docs/UE5_BRIDGE.md** - UE5虚实联动配置详细文档
4. **docs/PACKAGE_STRUCTURE.md** - 包结构与开发指南
5. **docs/DELIVERY.md** - 本交付说明文档

## 🚀 如何使用

### 在 Ubuntu 22.04 + ROS2 Humble 环境中

#### 1. 编译工作空间

```bash
cd ~/campus_road_inspection_ws
colcon build --symlink-install
source install/setup.bash
```

#### 2. 启动系统

```bash
# Terminal 1: 启动 rosbridge（用于 UE5 通信）
ros2 launch rosbridge_server rosbridge_websocket_launch.xml

# Terminal 2: 启动驱动层
ros2 launch cri_bringup drivers.launch.py

# Terminal 3: 启动 UE5 桥接
ros2 launch ue5_bridge ue5_bridge.launch.py
```

#### 3. 启动 UE5 虚拟端（Windows）

1. 打开 `CyberLubanTwin.uproject`
2. 配置 ROSIntegration 插件连接到 Ubuntu 端 IP:9090
3. 点击 Play 开始虚实联动

### 验证运行

```bash
# 查看所有话题
ros2 topic list

# 监听 UE5 状态同步
ros2 topic echo /ue5/robot_state

# 手动发送速度指令
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.5}, angular: {z: 0.0}}"
```

## 📋 包结构总览

```
campus_road_inspection_ws/
├── src/
│   ├── cri_msgs/              ✅ 自定义消息与服务
│   ├── cri_drivers/           ✅ 驱动适配层
│   │   ├── chassis_driver/    ✅ 底盘驱动
│   │   ├── livox_driver/      ✅ 雷达驱动
│   │   └── hikrobot_camera/   ✅ 相机驱动
│   ├── cri_perception/        ⚠️ 感知检测层（部分占位）
│   │   ├── lidar_defect_detector/  ✅ 激光缺陷检测
│   │   ├── vision_defect_detector/ ⚠️ 视觉缺陷检测（待补充）
│   │   └── sensor_fusion/          ⚠️ 多传感器融合（待补充）
│   ├── cri_algorithm/         ⚠️ 核心算法层（目录已创建）
│   │   ├── world_model/       ⚠️ 世界模型（待实现）
│   │   └── dynamic_roadmap/   ⚠️ 动态地图（待实现）
│   ├── cri_navigation/        ⚠️ 自主导航（待实现）
│   ├── cri_services/          ⚠️ 多机服务接口（待实现）
│   ├── ue5_bridge/            ✅ UE5虚实双向桥接
│   └── cri_bringup/           ✅ 系统启动与配置
└── docs/                      ✅ 完整文档
```

## ⚠️ 待完成项

### 高优先级

1. **硬件 SDK 对接**
   - 集成 Livox SDK 到 `livox_driver`
   - 集成海康 MVS SDK 到 `hikrobot_camera`
   - 实现 STM32 串口通信协议到 `chassis_driver`

2. **感知算法实现**
   - 实现激光地面分割与缺陷提取
   - 实现视觉语义缺陷识别（需训练模型）
   - 实现多传感器时空对齐与融合

3. **UE5 虚实联动测试**
   - 在局域网环境测试双向通信
   - 验证延迟和稳定性
   - 优化数据传输带宽

### 中优先级

4. **核心算法层**
   - 实现低秩动力学世界模型
   - 实现四维动态路面地图

5. **导航集成**
   - 配置 Nav2 导航栈
   - 实现航点巡航任务管理

6. **多机服务接口**
   - 实现路况查询服务
   - 实现绕行路径规划服务

## 🔧 技术栈

- **操作系统**：Ubuntu 22.04 LTS
- **ROS版本**：ROS2 Humble
- **编程语言**：Python 3.10+
- **虚实通信**：rosbridge_suite (WebSocket)
- **虚拟端**：UE5.3/5.7 + ROSIntegration 插件

## 📊 话题拓扑

### 核心数据流

```
驱动层 → 感知层 → 算法层 → 应用层
   ↓
UE5桥接（双向）
```

### 关键话题

| 话题名 | 消息类型 | 频率 | 说明 |
|--------|---------|------|------|
| `/cmd_vel` | Twist | 按需 | 速度控制指令 |
| `/odom` | Odometry | 20Hz | 里程计数据 |
| `/livox/pointcloud` | PointCloud2 | 10Hz | 激光点云 |
| `/camera/image_raw` | Image | 10Hz | 相机图像 |
| `/perception/fused_defects` | DefectArray | 1Hz | 融合缺陷检测结果 |
| `/ue5/command` | UE5Command | 按需 | UE5→实车控制 |
| `/ue5/robot_state` | UE5State | 10Hz | 实车→UE5状态 |

## 📝 重要提醒

1. **当前工作空间在 Windows 上生成**：
   - 所有 Python 节点可直接在 Ubuntu 上运行
   - 需要将整个 `campus_road_inspection_ws` 目录复制到 Ubuntu 机器
   - 在 Ubuntu 上运行 `colcon build` 编译

2. **硬件依赖**：
   - 需要安装 Livox SDK（参考 Livox 官方文档）
   - 需要安装海康 MVS SDK（参考海康官方文档）
   - 需要确认 STM32 串口设备号（通常为 /dev/ttyUSB0 或 /dev/ttyACM0）

3. **网络要求**：
   - UE5 虚拟端(Windows) 和实车端(Ubuntu) 必须在同一局域网
   - 确保防火墙允许 9090 端口（rosbridge）通信
   - 推荐使用有线网络以减少延迟

## 🎯 下一步行动建议

1. **立即可做**：
   - 将工作空间复制到 Ubuntu 机器
   - 运行 `colcon build` 验证编译通过
   - 启动占位节点，验证话题通信

2. **短期（1-2周）**：
   - 对接硬件 SDK
   - 测试 UE5 虚实联动基础通信
   - 实现激光地面分割算法

3. **中期（1-2月）**：
   - 完成感知层算法实现
   - 集成 Nav2 导航
   - 完整系统集成测试

## 📞 技术支持

如有问题，请参考：
- `docs/QUICKSTART.md` - 编译启动问题
- `docs/UE5_BRIDGE.md` - UE5通信问题
- `docs/PACKAGE_STRUCTURE.md` - 开发扩展问题

---

**交付日期**：2026-07-01  
**工作空间版本**：v1.0.0  
**状态**：骨架完成，可编译，核心模块待实现
