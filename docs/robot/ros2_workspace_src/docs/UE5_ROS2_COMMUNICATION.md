# UE5 与 ROS2 双向通信完整方案

## ✅ 可行性确认

**答案：完全可以！** UE5（Unreal Engine 5）通过 **rclUE** 插件或 **ROSBridge** 可以与ROS2实现双向通信。

---

## 🎯 应用场景

### 你的需求场景
```
【虚拟世界 UE5】          【真实世界 ROS2】
     ↓                          ↓
校园3D地图                  实体机器人
缺陷标记位置    ←ROS2→     RTK定位
虚拟车辆控制    ←ROS2→     真实车辆控制
     ↓                          ↓
前后移动同步                前后移动执行
转向角度同步                转向角度执行
```

### 双向数据流
```
UE5 → ROS2:
- 虚拟车辆控制指令（键盘/手柄输入）
- 目标航点设置
- 地图查询请求

ROS2 → UE5:
- 真实车辆位置（GPS坐标）
- 车辆姿态（roll, pitch, yaw）
- 缺陷检测结果
- 传感器数据（可选可视化）
```

---

## 🔧 技术方案

### 方案1: ROSBridge WebSocket（推荐）

#### 架构图
```
┌─────────────────┐         ┌──────────────┐         ┌─────────────────┐
│   UE5引擎       │         │  rosbridge   │         │   ROS2节点      │
│                 │         │  服务器       │         │                 │
│  WebSocket客户端│◄──────►│  WS:9090     │◄──────►│  发布/订阅话题  │
│  (蓝图/C++)     │   JSON  │              │  ROS2   │                 │
└─────────────────┘         └──────────────┘         └─────────────────┘
```

#### 优势
- ✅ 跨平台（Windows/Linux都支持）
- ✅ 实时性好（WebSocket持久连接）
- ✅ 易于调试（JSON格式）
- ✅ 官方支持

#### 安装步骤

**1. 安装rosbridge_server**
```bash
# ROS2端安装
sudo apt install ros-humble-rosbridge-server
# 或
sudo apt install ros-foxy-rosbridge-server
```

**2. 启动rosbridge**
```bash
ros2 launch rosbridge_server rosbridge_websocket_launch.xml
```

默认端口：9090

**3. UE5端安装ROSIntegration插件**

方法A：使用官方插件
```
1. 下载: https://github.com/code-iai/ROSIntegration
2. 解压到 YourProject/Plugins/ROSIntegration
3. 重新编译项目
4. 在UE5中启用插件
```

方法B：使用rclUE（推荐，原生ROS2支持）
```
1. 下载: https://github.com/RobotecAI/rclUE
2. 按文档编译安装
3. 支持原生ROS2通信，无需rosbridge
```

---

### 方案2: rclUE 原生ROS2（性能最佳）

#### 架构图
```
┌─────────────────┐         ┌─────────────────┐
│   UE5引擎       │         │   ROS2节点      │
│                 │  DDS    │                 │
│  rclUE插件      │◄──────►│  发布/订阅话题  │
│  (原生ROS2)     │         │                 │
└─────────────────┘         └─────────────────┘
```

#### 优势
- ✅ 性能最佳（原生DDS通信）
- ✅ 完整ROS2功能支持
- ✅ 类型安全

#### 劣势
- ⚠️ 配置复杂
- ⚠️ Windows支持有限

---

## 📡 通信协议定义

### 1. 车辆控制（UE5 → ROS2）

**话题**: `/ue5/vehicle/cmd`  
**消息类型**: `geometry_msgs/Twist`

```python
# ROS2消息定义
geometry_msgs/Twist:
    linear:
        x: 0.5  # 前进速度 m/s (正=前进，负=后退)
        y: 0.0  # 横向速度（通常为0）
        z: 0.0  # 垂直速度（通常为0）
    angular:
        x: 0.0  # roll角速度（通常为0）
        y: 0.0  # pitch角速度（通常为0）
        z: 0.3  # yaw角速度 rad/s (正=左转，负=右转)
```

**UE5蓝图示例**（WebSocket JSON格式）:
```json
{
    "op": "publish",
    "topic": "/ue5/vehicle/cmd",
    "msg": {
        "linear": {
            "x": 0.5,
            "y": 0.0,
            "z": 0.0
        },
        "angular": {
            "x": 0.0,
            "y": 0.0,
            "z": 0.3
        }
    }
}
```

### 2. 车辆位置（ROS2 → UE5）

**话题**: `/robot/localization/pose`  
**消息类型**: `geometry_msgs/PoseStamped`

```python
# ROS2消息定义
geometry_msgs/PoseStamped:
    header:
        stamp: {sec: 1234567890, nanosec: 0}
        frame_id: "map"
    pose:
        position:
            x: 123.45  # 东坐标 (米)
            y: 456.78  # 北坐标 (米)
            z: 5.23    # 高度 (米)
        orientation:
            x: 0.0
            y: 0.0
            z: 0.7071  # 四元数
            w: 0.7071
```

**UE5接收并转换为世界坐标**:
```cpp
// C++代码示例
FVector ROS2ToUE5Coordinates(double ros_x, double ros_y, double ros_z)
{
    // 校园原点GPS坐标
    const double OriginX = 123.0;
    const double OriginY = 456.0;
    const double OriginZ = 5.0;
    
    // 转换为UE5坐标（厘米，Z轴向上）
    FVector UE5Pos;
    UE5Pos.X = (ros_x - OriginX) * 100.0f;  // 米→厘米
    UE5Pos.Y = (ros_y - OriginY) * 100.0f;
    UE5Pos.Z = (ros_z - OriginZ) * 100.0f;
    
    return UE5Pos;
}
```

### 3. 缺陷标记（ROS2 → UE5）

**话题**: `/roadmap/defects`  
**消息类型**: `cri_msgs/DefectArray`（自定义）

```python
# 自定义消息（简化版）
DefectArray:
    header:
        stamp: ...
        frame_id: "map"
    defects:
        - id: "D001"
          type: "crack"
          severity: "high"
          position:
              x: 123.45
              y: 456.78
              z: 5.23
        - id: "D002"
          type: "pothole"
          ...
```

---

## 🎮 UE5蓝图实现

### 蓝图1：连接ROSBridge

```
【Event BeginPlay】
    ↓
【Create WebSocket Connection】
    URL: ws://localhost:9090
    ↓
【On Connected】
    Print String: "Connected to ROS2!"
    ↓
【Subscribe to Topics】
    - /robot/localization/pose
    - /roadmap/defects
```

**详细节点**:
```
节点: Connect to WebSocket
- Target: ROSIntegration Subsystem
- URL: ws://192.168.1.100:9090  (ROS2主机IP)
- Protocol: rosbridge_v2

节点: Subscribe to Topic
- Topic Name: /robot/localization/pose
- Message Type: geometry_msgs/PoseStamped
- Callback: OnPoseReceived
```

### 蓝图2：接收位置并更新车辆

```
【OnPoseReceived (Event)】
    ↓
【Parse JSON Message】
    Extract: position.x, position.y, position.z
    Extract: orientation (quaternion)
    ↓
【Convert Coordinates】
    ROS2 (米) → UE5 (厘米)
    ↓
【Set Actor Location】
    Target: VehicleActor
    New Location: Converted Position
    ↓
【Set Actor Rotation】
    Target: VehicleActor
    New Rotation: Converted Orientation
```

**蓝图节点示例**:
```
节点: Convert ROS to UE Coordinates
输入: 
- ROS X (float)
- ROS Y (float)  
- ROS Z (float)
- Origin X (float) = 123.0
- Origin Y (float) = 456.0
- Origin Z (float) = 5.0

输出:
- UE X = (ROS X - Origin X) * 100
- UE Y = (ROS Y - Origin Y) * 100
- UE Z = (ROS Z - Origin Z) * 100

节点: Set Actor Location
- Target: Get Actor (VehicleActor)
- New Location: Make Vector(UE X, UE Y, UE Z)
- Sweep: False
```

### 蓝图3：发送控制指令

```
【Input W Key Pressed】
    ↓
【Create Twist Message】
    linear.x = 0.5  (前进)
    angular.z = 0.0
    ↓
【Publish to Topic】
    Topic: /ue5/vehicle/cmd
    Message: Twist
    
【Input S Key Pressed】
    ↓
【Create Twist Message】
    linear.x = -0.5  (后退)
    angular.z = 0.0
    ↓
【Publish to Topic】

【Input A/D Keys】
    ↓
【Create Twist Message】
    linear.x = 0.5
    angular.z = ±0.5  (转向)
    ↓
【Publish to Topic】
```

**蓝图节点示例**:
```
节点: Input Action (Forward)
- Key: W
- Event: Pressed

节点: Make Twist Message
- Linear X: 0.5
- Linear Y: 0.0
- Linear Z: 0.0
- Angular X: 0.0
- Angular Y: 0.0
- Angular Z: 0.0

节点: Publish Message
- Target: ROSIntegration
- Topic: /ue5/vehicle/cmd
- Message Type: Twist
- Message: (from Make Twist)
```

### 蓝图4：显示缺陷标记

```
【On Defect Array Received】
    ↓
【For Each Defect】
    ↓
    【Convert Position】
    ROS → UE5
    ↓
    【Check if Marker Exists】
    If Not:
        【Spawn Defect Marker Actor】
        - Location: Converted Position
        - Type: Defect Type (crack/pothole)
        - Color: Severity Color
    Else:
        【Update Existing Marker】
        - Location: Converted Position
```

---

## 🚗 车辆同步实现

### ROS2端：接收UE5控制并执行

```python
#!/usr/bin/env python3
"""
ROS2节点：接收UE5控制指令，控制真实车辆
"""
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist

class UE5VehicleController(Node):
    def __init__(self):
        super().__init__('ue5_vehicle_controller')
        
        # 订阅UE5控制指令
        self.subscription = self.create_subscription(
            Twist,
            '/ue5/vehicle/cmd',
            self.cmd_callback,
            10
        )
        
        # 发布到车辆控制器
        self.publisher = self.create_publisher(
            Twist,
            '/cmd_vel',  # 真实车辆的控制话题
            10
        )
        
        self.get_logger().info('UE5 Vehicle Controller Started')
    
    def cmd_callback(self, msg):
        """接收UE5控制指令并转发到真实车辆"""
        # 可以在这里添加安全检查、限速等
        
        # 限速
        max_linear = 1.0  # m/s
        max_angular = 1.0  # rad/s
        
        controlled_msg = Twist()
        controlled_msg.linear.x = max(min(msg.linear.x, max_linear), -max_linear)
        controlled_msg.angular.z = max(min(msg.angular.z, max_angular), -max_angular)
        
        # 发布到真实车辆
        self.publisher.publish(controlled_msg)
        
        self.get_logger().info(
            f'UE5 Cmd: linear={controlled_msg.linear.x:.2f}, '
            f'angular={controlled_msg.angular.z:.2f}'
        )

def main():
    rclpy.init()
    controller = UE5VehicleController()
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
```

### ROS2端：发布车辆位置给UE5

```python
#!/usr/bin/env python3
"""
ROS2节点：发布车辆位置给UE5
"""
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import PoseStamped

class VehiclePosePublisher(Node):
    def __init__(self):
        super().__init__('vehicle_pose_publisher')
        
        # 订阅真实车辆位置
        self.subscription = self.create_subscription(
            PoseStamped,
            '/localization/pose',  # 来自EKF融合定位
            self.pose_callback,
            10
        )
        
        # 发布给UE5
        self.publisher = self.create_publisher(
            PoseStamped,
            '/robot/localization/pose',  # UE5订阅的话题
            10
        )
    
    def pose_callback(self, msg):
        """转发位置给UE5"""
        # 直接转发或进行坐标转换
        self.publisher.publish(msg)

def main():
    rclpy.init()
    publisher = VehiclePosePublisher()
    rclpy.spin(publisher)
    publisher.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
```

---

## 📐 坐标系统对齐

### 关键：统一坐标原点

```python
# 校园原点配置（需要实际测量）
CAMPUS_ORIGIN = {
    'gps': {
        'latitude': 31.026000,   # 校园某个参考点
        'longitude': 121.434000,
        'altitude': 5.0
    },
    'ue5': {
        'x': 0.0,  # UE5世界坐标原点
        'y': 0.0,
        'z': 0.0
    }
}

# ROS2局部坐标系（东北天ENU）
# - X轴：东
# - Y轴：北
# - Z轴：天

# UE5世界坐标系
# - X轴：前（对应东）
# - Y轴：右（对应北）
# - Z轴：上（对应天）
```

### 坐标转换函数

```cpp
// UE5 C++
FVector GPSToUE5(double lat, double lon, double alt)
{
    // 1. GPS → 局部米制坐标
    double dx = (lon - ORIGIN_LON) * 111320.0 * cos(ORIGIN_LAT * PI / 180.0);
    double dy = (lat - ORIGIN_LAT) * 111320.0;
    double dz = alt - ORIGIN_ALT;
    
    // 2. 米 → 厘米（UE5单位）
    FVector ue5_pos;
    ue5_pos.X = dx * 100.0f;
    ue5_pos.Y = dy * 100.0f;
    ue5_pos.Z = dz * 100.0f;
    
    return ue5_pos;
}
```

---

## 🔄 完整工作流程

```
1. 启动ROS2系统
   ros2 launch cri_bringup perception.launch.py
   
2. 启动rosbridge
   ros2 launch rosbridge_server rosbridge_websocket_launch.xml
   
3. 启动UE5车辆控制转发
   ros2 run cri_ue5_bridge ue5_vehicle_controller
   
4. 启动位置发布
   ros2 run cri_ue5_bridge vehicle_pose_publisher
   
5. 启动UE5地图
   打开UE5项目，进入Play模式
   
6. 测试双向通信
   - UE5中按WASD键
   - 观察ROS2车辆响应
   - 移动真实车辆
   - 观察UE5虚拟车辆同步
```

---

## 🎨 UE5地图数据传输

### 3D校园模型导入

**方法1: FBX/OBJ导入**
```
1. 从建模软件导出
   - 格式: FBX 2020或更高
   - 单位: 厘米
   - 坐标系: Z-up

2. 导入UE5
   - Content Browser → Import
   - 选择FBX文件
   - 设置导入选项

3. 对齐坐标
   - 在World Outliner中选择模型
   - 设置Transform与GPS原点对应
```

**方法2: Cesium插件（如果有地理数据）**
```
1. 安装Cesium for Unreal插件
2. 导入Cesium Ion的地理数据
3. 自动地理对齐
```

### 缺陷标记传输

**ROS2 → UE5实时传输**:
```python
# ROS2端发布
defect_msg = DefectArray()
defect_msg.defects.append(...)
publisher.publish(defect_msg)

# UE5蓝图接收
OnDefectArrayReceived:
    - 解析JSON
    - 转换坐标
    - 实例化Marker Actor
```

---

## 💡 性能优化

### 降低延迟
- WebSocket保持连接：避免频繁重连
- 消息频率：10-20Hz足够（不要100Hz）
- 批量处理：多个缺陷打包发送

### 带宽优化
- 压缩：rosbridge支持CBOR二进制
- 按需订阅：只订阅需要的话题
- ROI过滤：只传输视野内的缺陷

---

**总结**: UE5与ROS2完全可以双向通信！推荐使用ROSBridge WebSocket方案，简单可靠。车辆控制和地图数据都能实时同步！
