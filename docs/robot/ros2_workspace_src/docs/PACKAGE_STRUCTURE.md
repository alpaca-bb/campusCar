# 包结构说明文档

## 包划分与职责

### 1. cri_msgs
自定义消息和服务定义包，所有包的公共接口层。

**消息类型**：
- `DefectInfo.msg` - 单个路面缺陷信息
- `DefectArray.msg` - 缺陷数组
- `DefectPrediction.msg` - 缺陷演化预测
- `RoadCondition.msg` - 路况综合信息
- `UE5Command.msg` - UE5 → 实车控制指令
- `UE5State.msg` - 实车 → UE5 状态同步

**服务类型**：
- `QueryRoadCondition.srv` - 查询指定区域路况
- `RequestDetourPath.srv` - 请求绕行路径规划

### 2. 驱动适配层 (cri_drivers)

#### chassis_driver
- 与 STM32 底盘通过串口通信
- 订阅 `/cmd_vel`，发布 `/odom` 和 `/chassis_pose`
- 差速运动学模型实现

#### livox_driver
- 对接 Livox Mid-360S SDK
- 发布标准化 `/livox/pointcloud` (PointCloud2)
- 需要集成 Livox 官方 SDK

#### hikrobot_camera
- 对接海康 GigE 相机 SDK
- 发布 `/camera/image_raw` 和 `/camera/camera_info`
- 需要集成海康 MVS SDK

### 3. 感知检测层 (cri_perception)

#### lidar_defect_detector（待实现）
- 订阅 `/livox/pointcloud`
- 地面点分割与 DEM 基准高程差分
- 发布 `/perception/lidar_defects` (DefectArray)

#### vision_defect_detector（待实现）
- 订阅 `/camera/image_raw`
- 基于深度学习的路面缺陷语义识别
- 发布 `/perception/vision_defects` (DefectArray)

#### sensor_fusion（待实现）
- 订阅激光和视觉的缺陷检测结果
- 时空对齐与融合校验
- 发布 `/perception/fused_defects` (DefectArray)

### 4. 核心算法层 (cri_algorithm)

#### world_model（待实现）
- 低秩动力学世界模型
- 订阅融合缺陷数据
- 发布 `/prediction/defect_evolution` (DefectPrediction)

#### dynamic_roadmap（待实现）
- 四维路面地图（几何+语义+时间）
- 提供 `QueryRoadCondition` 服务
- 支持增量更新

### 5. 应用层

#### cri_navigation（待实现）
- 基于 Nav2 的自主巡检导航
- 航点巡航任务管理

#### cri_services（待实现）
- 对外提供标准化服务接口
- `RequestDetourPath` 服务实现
- 多机协同调度

### 6. UE5 虚实桥接 (ue5_bridge)

**核心功能**：
- 虚拟端 → 实车：接收 `/ue5/command`，转发到 `/cmd_vel`
- 实车 → 虚拟端：聚合状态发布到 `/ue5/robot_state`
- 通过 rosbridge_suite 实现 WebSocket 通信

### 7. 系统启动 (cri_bringup)

**Launch 文件**：
- `full_system.launch.py` - 一键启动完整系统
- `drivers.launch.py` - 仅启动驱动层
- `perception.launch.py` - 仅启动感知层（待实现）
- `navigation.launch.py` - 仅启动导航层（待实现）

## 话题拓扑图

```
驱动层：
  chassis_driver -> /odom, /chassis_pose
  livox_driver -> /livox/pointcloud
  hikrobot_camera -> /camera/image_raw

感知层：
  lidar_defect_detector <- /livox/pointcloud -> /perception/lidar_defects
  vision_defect_detector <- /camera/image_raw -> /perception/vision_defects
  sensor_fusion <- lidar & vision defects -> /perception/fused_defects

算法层：
  world_model <- /perception/fused_defects -> /prediction/defect_evolution
  dynamic_roadmap <- fused_defects & prediction

UE5 桥接：
  ue5_bridge <- /ue5/command -> /cmd_vel
  ue5_bridge <- /odom, /livox/pointcloud, /perception/fused_defects -> /ue5/robot_state
```

## 依赖关系

```
cri_msgs (无依赖)
   ↓
驱动层 (依赖 cri_msgs)
   ↓
感知层 (依赖 cri_msgs + 驱动层话题)
   ↓
算法层 (依赖 cri_msgs + 感知层话题)
   ↓
应用层 (依赖 cri_msgs + 算法层服务)

ue5_bridge (依赖 cri_msgs + 跨层话题聚合)
cri_bringup (依赖所有包)
```

## 编译顺序

colcon 会自动根据 `package.xml` 中的依赖关系确定编译顺序：

1. `cri_msgs` (最底层，无依赖)
2. 驱动层三个包（依赖 cri_msgs）
3. 其余包（依赖 cri_msgs + 驱动层）

## 开发指南

### 添加新节点

1. 在对应包的 Python 模块目录创建节点文件
2. 在 `setup.py` 的 `entry_points` 中注册节点
3. 更新 `package.xml` 添加依赖
4. 在 `cri_bringup/launch/` 中添加对应 launch 文件

### 添加新消息

1. 在 `cri_msgs/msg/` 或 `cri_msgs/srv/` 创建消息定义
2. 在 `cri_msgs/CMakeLists.txt` 的 `rosidl_generate_interfaces` 中添加
3. 重新编译 `cri_msgs` 包
4. 其他包即可使用新消息

## 测试策略

### 单元测试
每个包应包含独立的单元测试，验证核心算法逻辑。

### 集成测试
使用 rosbag 录制真实数据，回放测试各层级联动。

### 硬件在环测试
在实际硬件平台上验证驱动层和完整系统。

### 虚实联动测试
启动 UE5 和实车端，验证双向通信延迟和稳定性。
