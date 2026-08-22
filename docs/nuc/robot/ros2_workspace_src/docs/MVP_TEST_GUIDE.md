# MVP测试指南 - 校园路面巡检系统

## 🎯 MVP测试目标

验证完整的感知链路能够：
1. 接收传感器数据（激光+相机）
2. 执行地面分割与缺陷检测
3. 完成多传感器融合
4. 输出结构化缺陷信息
5. 与UE5虚拟端建立双向通信
6. 在数字孪生界面实时展示

## 📋 前置准备

### 1. 确认Ubuntu环境
```bash
# 检查ROS2版本
ros2 --version  # 应显示 Humble

# 检查Python版本
python3 --version  # 应>=3.10

# 确认工作空间已编译
cd ~/campus_road_inspection_ws
source install/setup.bash
```

### 2. 安装额外依赖
```bash
# OpenVINO（视觉检测必需）
pip3 install openvino openvino-dev

# 图像处理
pip3 install opencv-python

# 数值计算
pip3 install numpy scipy

# rosbridge（UE5通信必需）
sudo apt install ros-humble-rosbridge-suite

# 可视化工具
sudo apt install ros-humble-rqt ros-humble-rqt-image-view
```

### 3. 准备测试数据

由于暂时没有真实传感器，我们创建模拟数据发布器：

#### 创建激光点云模拟器
```bash
# 文件位置: ~/campus_road_inspection_ws/test/mock_lidar_pub.py
```

```python
#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2, PointField
import numpy as np
import struct

class MockLidarPublisher(Node):
    def __init__(self):
        super().__init__('mock_lidar_publisher')
        self.pub = self.create_publisher(PointCloud2, '/livox/pointcloud', 10)
        self.timer = self.create_timer(0.1, self.publish_pointcloud)  # 10Hz
        
    def publish_pointcloud(self):
        # 生成模拟地面点云（10m x 10m区域）
        x = np.random.uniform(-5, 5, 5000)
        y = np.random.uniform(-5, 5, 5000)
        z = np.random.normal(0.0, 0.02, 5000)  # 地面基准高度+噪声
        
        # 添加一些模拟缺陷（凹陷区域）
        defect_mask = (x > 1) & (x < 2) & (y > 1) & (y < 2)
        z[defect_mask] -= 0.03  # 3cm深的坑
        
        intensity = np.ones(5000) * 100.0
        points = np.column_stack([x, y, z, intensity])
        
        msg = PointCloud2()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = 'livox_frame'
        msg.height = 1
        msg.width = len(points)
        msg.fields = [
            PointField(name='x', offset=0, datatype=PointField.FLOAT32, count=1),
            PointField(name='y', offset=4, datatype=PointField.FLOAT32, count=1),
            PointField(name='z', offset=8, datatype=PointField.FLOAT32, count=1),
            PointField(name='intensity', offset=12, datatype=PointField.FLOAT32, count=1),
        ]
        msg.is_bigendian = False
        msg.point_step = 16
        msg.row_step = msg.point_step * msg.width
        msg.is_dense = True
        
        data = []
        for point in points:
            data.extend(struct.pack('ffff', *point))
        msg.data = bytes(data)
        
        self.pub.publish(msg)

def main():
    rclpy.init()
    node = MockLidarPublisher()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()
```

#### 创建相机图像模拟器
```bash
# 文件位置: ~/campus_road_inspection_ws/test/mock_camera_pub.py
```

```python
#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image, CameraInfo
import numpy as np
import cv2
from cv_bridge import CvBridge

class MockCameraPublisher(Node):
    def __init__(self):
        super().__init__('mock_camera_publisher')
        self.image_pub = self.create_publisher(Image, '/camera/image_raw', 10)
        self.info_pub = self.create_publisher(CameraInfo, '/camera/camera_info', 10)
        self.timer = self.create_timer(0.1, self.publish_image)  # 10Hz
        self.bridge = CvBridge()
        
    def publish_image(self):
        # 生成模拟路面图像（灰色路面+模拟裂缝）
        img = np.ones((1080, 1920, 3), dtype=np.uint8) * 100
        
        # 绘制模拟裂缝
        cv2.line(img, (500, 400), (700, 600), (50, 50, 50), 3)
        cv2.circle(img, (1000, 500), 50, (80, 80, 80), -1)  # 模拟坑槽
        
        # 发布图像
        img_msg = self.bridge.cv2_to_imgmsg(img, encoding='bgr8')
        img_msg.header.stamp = self.get_clock().now().to_msg()
        img_msg.header.frame_id = 'camera_frame'
        self.image_pub.publish(img_msg)
        
        # 发布相机内参
        info_msg = CameraInfo()
        info_msg.header = img_msg.header
        info_msg.width = 1920
        info_msg.height = 1080
        info_msg.k = [1500.0, 0.0, 960.0, 0.0, 1500.0, 540.0, 0.0, 0.0, 1.0]
        info_msg.d = [0.0, 0.0, 0.0, 0.0, 0.0]
        self.info_pub.publish(info_msg)

def main():
    rclpy.init()
    node = MockCameraPublisher()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()
```

## 🚀 MVP测试执行

### 测试1：感知链路独立测试

#### Terminal 1: 启动模拟激光
```bash
cd ~/campus_road_inspection_ws
source install/setup.bash
python3 test/mock_lidar_pub.py
```

#### Terminal 2: 启动模拟相机
```bash
source install/setup.bash
python3 test/mock_camera_pub.py
```

#### Terminal 3: 启动完整感知链路
```bash
source install/setup.bash
ros2 launch cri_bringup perception.launch.py
```

#### Terminal 4: 监控输出
```bash
# 查看融合后的缺陷检测结果
ros2 topic echo /perception/fused_defects

# 查看话题列表
ros2 topic list

# 查看节点状态
ros2 node list

# 查看话题频率
ros2 topic hz /perception/fused_defects
```

#### 预期结果
- ✅ 所有4个节点正常启动
- ✅ `/perception/fused_defects` 有数据输出
- ✅ 检测到模拟缺陷（1-2个）
- ✅ 融合置信度>0.4

### 测试2：UE5虚实联动测试

#### Terminal 1-3: 保持上述节点运行

#### Terminal 4: 启动rosbridge
```bash
source install/setup.bash
ros2 launch rosbridge_server rosbridge_websocket_launch.xml
```

#### Terminal 5: 启动UE5桥接
```bash
source install/setup.bash
ros2 launch ue5_bridge ue5_bridge.launch.py
```

#### Windows端: 启动UE5虚拟平台
1. 打开 `CyberLubanTwin.uproject`
2. 在 ROSIntegration 设置中配置：
   - Host: Ubuntu机器IP（如192.168.1.100）
   - Port: 9090
   - Protocol: ws://
3. 配置订阅话题：`/ue5/robot_state`
4. 配置发布话题：`/ue5/command`
5. 点击 Play

#### 预期结果
- ✅ UE5显示"Connected to ROS"
- ✅ 虚拟场景中实时显示机器人位姿
- ✅ 数字孪生界面显示检测到的缺陷标注
- ✅ 在UE5中发送控制指令，实车响应（如有底盘）

### 测试3：数据流完整性验证

```bash
# 检查所有关键话题的数据流
ros2 topic hz /livox/pointcloud  # 应~10Hz
ros2 topic hz /camera/image_raw  # 应~10Hz
ros2 topic hz /perception/ground_cloud  # 应~10Hz
ros2 topic hz /perception/lidar_defects  # 应~1-10Hz
ros2 topic hz /perception/vision_defects  # 应~1-10Hz
ros2 topic hz /perception/fused_defects  # 应~1-10Hz
ros2 topic hz /ue5/robot_state  # 应~10Hz

# 检查消息内容
ros2 topic echo /perception/fused_defects --once
```

### 测试4：可视化验证

```bash
# 启动RViz2可视化
ros2 run rviz2 rviz2

# 在RViz中添加：
# - PointCloud2: /livox/pointcloud
# - PointCloud2: /perception/ground_cloud
# - Image: /perception/detection_image
# - Marker: /perception/defect_markers（如有）
```

## 📊 MVP成功标准

| 测试项 | 标准 | 状态 |
|--------|------|------|
| 感知链路启动 | 4个节点全部正常运行 | ⬜ |
| 地面分割 | 点云正确分为地面/非地面 | ⬜ |
| 激光检测 | 检测到模拟缺陷 | ⬜ |
| 视觉检测 | 运行无报错（模型可选） | ⬜ |
| 融合输出 | `/fused_defects`有数据 | ⬜ |
| rosbridge通信 | UE5连接成功 | ⬜ |
| 双向数据流 | 实车→UE5数据同步 | ⬜ |
| 数字孪生展示 | UE5显示缺陷标注 | ⬜ |

## 🐛 常见问题排查

### 1. 节点启动失败
```bash
# 检查Python路径
which python3

# 检查包是否编译
ls install/lidar_defect_detector/lib/lidar_defect_detector/

# 重新编译
colcon build --packages-select lidar_defect_detector --symlink-install
```

### 2. 找不到消息类型
```bash
# 确认消息包已编译
colcon build --packages-select cri_msgs

# 重新source
source install/setup.bash
```

### 3. UE5无法连接
```bash
# 检查rosbridge是否运行
ros2 node list | grep rosbridge

# 检查端口
netstat -tulpn | grep 9090

# 检查防火墙
sudo ufw allow 9090
```

### 4. 没有数据输出
```bash
# 检查话题是否存在
ros2 topic list

# 检查发布频率
ros2 topic hz /livox/pointcloud

# 检查节点日志
ros2 node info /ground_segmentation
```

## 📈 性能基准

在Intel NUC + ROS2 Humble环境下的预期性能：

- 地面分割：~20Hz（5000点/帧）
- 激光检测：~10Hz
- 视觉检测：~5-10Hz（CPU OpenVINO）
- 融合输出：~5-10Hz
- 端到端延迟：<200ms
- 内存占用：<2GB

## 🎬 演示流程（答辩用）

1. **启动展示**（30秒）
   - 一键启动所有节点
   - 展示RViz2实时可视化

2. **功能演示**（2分钟）
   - 播放测试数据或实时采集
   - 展示地面分割效果
   - 展示缺陷检测结果
   - 展示融合后的置信度提升

3. **数字孪生展示**（2分钟）
   - 切换到UE5界面
   - 展示实时位姿同步
   - 展示缺陷标注
   - 演示虚拟操控实车（如有）

4. **技术亮点讲解**（1分钟）
   - 多传感器融合算法
   - 置信度加权策略
   - 虚实双向通信架构

---

**MVP版本**: v1.0  
**测试日期**: 待执行  
**测试状态**: 准备就绪
