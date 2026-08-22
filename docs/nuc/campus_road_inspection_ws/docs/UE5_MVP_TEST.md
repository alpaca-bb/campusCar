# UE5虚实联动测试验证指南

## 🎯 测试目标

在 CyberLubanTwin UE平台中验证：
1. ROS2感知链路与UE5的双向通信
2. 实时缺陷数据同步到数字孪生场景
3. 虚拟操控指令下发到ROS2系统
4. 整体系统延迟与稳定性

## 🖥️ 测试环境准备

### Ubuntu端（ROS2系统）

#### 1. 确认IP地址
```bash
# 获取Ubuntu机器的局域网IP
ip addr show | grep "inet " | grep -v 127.0.0.1

# 示例输出: inet 192.168.1.100/24
# 记下这个IP，后续UE5要连接这个地址
```

#### 2. 安装rosbridge
```bash
sudo apt install ros-humble-rosbridge-suite
```

#### 3. 配置防火墙
```bash
# 允许9090端口（rosbridge WebSocket）
sudo ufw allow 9090

# 如果防火墙未启用，可以跳过
sudo ufw status
```

### Windows端（UE5平台）

#### 1. 确认CyberLubanTwin工程路径
- 工程位置: `C:\Users\xzx15\Desktop\CyberLuban\CyberLubanTwin\`
- 确认存在: `CyberLubanTwin.uproject`

#### 2. 检查ROSIntegration插件
在UE5中打开工程后：
- 编辑 → 插件 → 搜索 "ROSIntegration"
- 确认已启用

#### 3. 网络连通性测试
```powershell
# 在PowerShell中测试与Ubuntu的连通性
ping 192.168.1.100  # 替换为实际Ubuntu IP

# 测试9090端口（需要Ubuntu端先启动rosbridge）
Test-NetConnection -ComputerName 192.168.1.100 -Port 9090
```

## 🚀 完整测试流程

### 阶段1：基础连通性测试

#### Ubuntu端操作

**Terminal 1: 启动rosbridge**
```bash
cd ~/campus_road_inspection_ws
source install/setup.bash

# 启动rosbridge服务器
ros2 launch rosbridge_server rosbridge_websocket_launch.xml
```

**预期输出**:
```
[INFO] [rosbridge_websocket]: Rosbridge WebSocket server started on port 9090
```

**Terminal 2: 启动UE5桥接节点**
```bash
source install/setup.bash

# 启动虚实桥接
ros2 run ue5_bridge ue5_bridge_node
```

**预期输出**:
```
[INFO] [ue5_bridge_node]: UE5 虚实双向桥接节点已启动
```

#### Windows端操作

**步骤1**: 打开UE5工程
```
双击: C:\Users\xzx15\Desktop\CyberLuban\CyberLubanTwin\CyberLubanTwin.uproject
```

**步骤2**: 配置ROSIntegration连接

在UE5编辑器中：
1. 打开 `编辑 → 项目设置`
2. 搜索 "ROS Integration"
3. 配置以下参数：
   - **ROSBridge Server Host**: `192.168.1.100` （Ubuntu IP）
   - **ROSBridge Server Port**: `9090`
   - **Protocol**: `ws://`
   - **Reconnect Interval**: `1.0`

4. 点击 `Apply` 保存

**步骤3**: 验证连接

在UE5输出日志中查找：
```
LogROSIntegration: Connected to ROS bridge at ws://192.168.1.100:9090
```

✅ **如果看到此消息，基础连通性测试通过！**

---

### 阶段2：数据流测试（实车→UE5）

#### Ubuntu端操作

**Terminal 3: 启动模拟数据发布器**

创建测试脚本 `~/campus_road_inspection_ws/test/mock_data_publisher.py`:

```python
#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import PoseStamped, Twist
from nav_msgs.msg import Odometry
from cri_msgs.msg import DefectArray, DefectInfo
from geometry_msgs.msg import Point, Vector3
import math

class MockDataPublisher(Node):
    def __init__(self):
        super().__init__('mock_data_publisher')
        
        # 发布里程计（模拟实车位姿）
        self.odom_pub = self.create_publisher(Odometry, '/odom', 10)
        
        # 发布模拟缺陷数据
        self.defect_pub = self.create_publisher(DefectArray, '/perception/fused_defects', 10)
        
        # 定时器
        self.timer = self.create_timer(0.1, self.publish_data)  # 10Hz
        
        self.t = 0.0
        self.get_logger().info('模拟数据发布器已启动')
    
    def publish_data(self):
        self.t += 0.1
        
        # 发布模拟里程计（圆周运动）
        odom = Odometry()
        odom.header.stamp = self.get_clock().now().to_msg()
        odom.header.frame_id = 'odom'
        odom.child_frame_id = 'base_link'
        
        # 圆周运动轨迹
        radius = 5.0
        odom.pose.pose.position.x = radius * math.cos(self.t * 0.5)
        odom.pose.pose.position.y = radius * math.sin(self.t * 0.5)
        odom.pose.pose.position.z = 0.0
        
        # 朝向切线方向
        yaw = self.t * 0.5 + math.pi / 2
        odom.pose.pose.orientation.z = math.sin(yaw / 2.0)
        odom.pose.pose.orientation.w = math.cos(yaw / 2.0)
        
        odom.twist.twist.linear.x = 0.5
        odom.twist.twist.angular.z = 0.1
        
        self.odom_pub.publish(odom)
        
        # 每5秒发布一次缺陷数据
        if int(self.t) % 5 == 0 and (self.t - int(self.t)) < 0.15:
            self.publish_defect()
    
    def publish_defect(self):
        defect_array = DefectArray()
        defect_array.header.stamp = self.get_clock().now().to_msg()
        defect_array.header.frame_id = 'map'
        
        # 创建模拟缺陷
        defect = DefectInfo()
        defect.header = defect_array.header
        defect.defect_id = f'test_{int(self.t)}'
        defect.defect_type = 'pothole'
        defect.position = Point(x=3.0, y=2.0, z=0.0)
        defect.dimensions = Vector3(x=0.5, y=0.4, z=0.08)
        defect.confidence = 0.85
        defect.detection_source = 'fusion'
        defect.severity_level = 'high'
        
        defect_array.defects.append(defect)
        self.defect_pub.publish(defect_array)
        
        self.get_logger().info(f'发布缺陷: {defect.defect_id} at ({defect.position.x:.2f}, {defect.position.y:.2f})')

def main():
    rclpy.init()
    node = MockDataPublisher()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()
```

启动模拟器：
```bash
chmod +x test/mock_data_publisher.py
python3 test/mock_data_publisher.py
```

**预期输出**:
```
[INFO] [mock_data_publisher]: 模拟数据发布器已启动
[INFO] [mock_data_publisher]: 发布缺陷: test_5 at (3.00, 2.00)
```

**Terminal 4: 监控数据流**
```bash
# 检查话题是否正常发布
ros2 topic hz /odom
ros2 topic hz /perception/fused_defects

# 查看UE5桥接是否聚合数据
ros2 topic echo /ue5/robot_state --once
```

#### Windows端验证

在UE5中：

**步骤1**: 创建或打开测试关卡
- 打开 `Content/Maps/TestLevel.umap`（如果没有则创建新关卡）

**步骤2**: 添加ROS订阅Actor

在关卡蓝图或专用Actor中订阅话题：

```cpp
// 伪代码示例（实际在UE5蓝图中配置）
订阅话题: "/ue5/robot_state"
消息类型: cri_msgs/UE5State

OnMessageReceived:
    更新机器人位姿
    更新缺陷标注
```

**步骤3**: 运行并验证

点击 `Play` 后，应该看到：
- ✅ 场景中有一个代表机器人的物体在圆周运动
- ✅ 每5秒在(3.0, 2.0)位置生成一个缺陷标记
- ✅ UE5输出日志显示接收到消息

---

### 阶段3：控制测试（UE5→实车）

#### Windows端操作

**步骤1**: 在UE5中创建控制界面

在UE5 Widget蓝图中添加：
- 前进按钮
- 后退按钮
- 左转按钮
- 右转按钮
- 紧急停止按钮

**步骤2**: 配置发布话题

为每个按钮配置发布消息：

```
话题: /ue5/command
消息类型: cri_msgs/UE5Command

前进按钮:
  velocity_command.linear.x = 0.5
  control_mode = "manual"

后退按钮:
  velocity_command.linear.x = -0.3
  control_mode = "manual"

左转按钮:
  velocity_command.angular.z = 0.5
  control_mode = "manual"
```

**步骤3**: 测试发布

点击按钮后，检查消息是否发出（UE5日志）

#### Ubuntu端验证

**Terminal 5: 监听控制指令**
```bash
# 查看UE5发来的控制指令
ros2 topic echo /ue5/command

# 查看桥接节点转发的速度指令
ros2 topic echo /cmd_vel
```

**预期行为**:
- ✅ 点击UE5按钮后，Ubuntu端立即收到消息
- ✅ `/ue5/command` 正确转发到 `/cmd_vel`
- ✅ 如果有实车底盘，应该响应运动

---

### 阶段4：完整感知链路+UE5联合测试

#### Ubuntu端完整启动序列

```bash
# Terminal 1: rosbridge
ros2 launch rosbridge_server rosbridge_websocket_launch.xml

# Terminal 2: 模拟传感器数据
python3 test/mock_lidar_pub.py & python3 test/mock_camera_pub.py

# Terminal 3: 感知链路
ros2 launch cri_bringup perception.launch.py

# Terminal 4: UE5桥接
ros2 run ue5_bridge ue5_bridge_node

# Terminal 5: 监控
ros2 topic hz /perception/fused_defects
ros2 topic hz /ue5/robot_state
```

#### Windows端验证

在UE5中：
1. 点击 `Play`
2. 观察场景中：
   - ✅ 实时缺陷标注出现
   - ✅ 点云数据可视化（如已配置）
   - ✅ 机器人位姿同步
3. 点击控制按钮：
   - ✅ 虚拟操控响应

---

## 📊 测试检查表

### 基础连通性
- [ ] Ubuntu rosbridge正常启动
- [ ] UE5成功连接到rosbridge
- [ ] UE5日志显示"Connected"
- [ ] 无连接超时或断开

### 数据流：实车→UE5
- [ ] `/odom` 数据正常发布（20Hz）
- [ ] `/perception/fused_defects` 有数据输出
- [ ] `/ue5/robot_state` 正确聚合
- [ ] UE5场景实时更新位姿
- [ ] UE5场景显示缺陷标注

### 数据流：UE5→实车
- [ ] UE5能发布 `/ue5/command`
- [ ] 桥接节点收到指令
- [ ] 正确转发到 `/cmd_vel`
- [ ] 底盘响应（如有硬件）

### 性能指标
- [ ] 端到端延迟<500ms
- [ ] 无明显丢包
- [ ] UE5帧率稳定>30fps
- [ ] ROS节点CPU<80%

---

## 🐛 常见问题排查

### 问题1：UE5无法连接rosbridge

**症状**: 
```
LogROSIntegration: Failed to connect to ws://192.168.1.100:9090
```

**排查步骤**:
```bash
# 1. 确认rosbridge运行
ros2 node list | grep rosbridge

# 2. 确认端口监听
netstat -tulpn | grep 9090

# 3. 测试WebSocket
# 在Windows PowerShell:
Test-NetConnection -ComputerName 192.168.1.100 -Port 9090

# 4. 检查防火墙
sudo ufw status
sudo ufw allow 9090

# 5. 重启rosbridge
ros2 launch rosbridge_server rosbridge_websocket_launch.xml
```

### 问题2：UE5收不到消息

**排查步骤**:
```bash
# 1. 确认话题存在
ros2 topic list | grep ue5

# 2. 确认有数据发布
ros2 topic hz /ue5/robot_state

# 3. 手动发布测试消息
ros2 topic pub /ue5/robot_state cri_msgs/msg/UE5State "{...}"

# 4. 检查UE5订阅配置
# 在UE5中检查话题名称拼写
```

### 问题3：延迟过高

**优化措施**:
```bash
# 1. 降低点云发布频率
# 在lidar_params.yaml中设置 publish_rate: 5.0

# 2. 降低图像分辨率
# 在camera_params.yaml中设置较低分辨率

# 3. 使用有线网络代替WiFi

# 4. 启用消息压缩（rosbridge配置）
```

---

## 🎬 演示视频录制建议

### 准备
1. 清理桌面，关闭无关程序
2. 准备PPT或标题卡
3. 测试屏幕录制软件

### 分镜建议

**镜头1: 系统架构图**（15秒）
- 展示ROS2-UE5整体架构图

**镜头2: Ubuntu端启动**（30秒）
- 分屏显示4个Terminal
- 依次启动rosbridge、感知链路、桥接
- 强调"一键启动"

**镜头3: UE5连接**（20秒）
- 打开UE5工程
- 展示ROSIntegration配置
- 展示连接成功日志

**镜头4: 实时数据同步**（60秒）
- 分屏：左侧Ubuntu RViz，右侧UE5
- 展示点云、缺陷检测、位姿同步
- 突出"毫秒级实时性"

**镜头5: 虚拟操控**（30秒）
- 在UE5中点击控制按钮
- 展示指令下发与响应
- 如有实车，展示实际运动

**镜头6: 技术亮点**（30秒）
- 快速切回架构图
- 列出关键技术点

---

## ✅ 测试成功标准

### MVP通过标准
1. ✅ ROS2与UE5稳定连接（无断线）
2. ✅ 缺陷检测数据实时显示在UE5
3. ✅ 虚拟操控指令成功下发
4. ✅ 端到端延迟<500ms
5. ✅ 系统持续运行>5分钟无崩溃

### 演示就绪标准
1. ✅ 能够流畅演示完整流程
2. ✅ 有录制好的备用视频
3. ✅ 准备好技术讲解PPT
4. ✅ 能够回答评委技术问题

---

**测试版本**: MVP v1.0  
**建议测试时间**: 2-3小时  
**建议测试人员**: 2人（1人操作Ubuntu，1人操作UE5）
