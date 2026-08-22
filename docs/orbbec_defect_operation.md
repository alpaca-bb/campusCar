# Orbbec RGBD 相机与缺陷识别操作说明

本文档记录 Orbbec Gemini 336L RGBD 相机接入 ROS2、调用缺陷识别模型、打开实时图像窗口的常用操作。

## 一、窗口地址

原始相机画面：

```text
http://localhost:8088/
```

缺陷识别结果：

```text
http://localhost:8089/
```

对应 ROS2 话题：

```text
/camera/color/image_raw
/perception/detection_image
/perception/vision_defects
```

## 二、一键控制脚本

控制脚本位置：

```bash
/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/scripts/orbbec_defect_control.sh
```

### 启动全部

启动 Orbbec 相机、缺陷识别模型、原始画面窗口和识别结果窗口：

```bash
/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/scripts/orbbec_defect_control.sh start
```

启动后打开：

```text
http://localhost:8088/
http://localhost:8089/
```

### 停止全部

```bash
/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/scripts/orbbec_defect_control.sh stop
```

### 重启全部

如果窗口显示异常、服务不可用、图像不刷新，优先执行：

```bash
/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/scripts/orbbec_defect_control.sh restart
```

### 查看状态

```bash
/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/scripts/orbbec_defect_control.sh status
```

正常时应看到类似：

```text
/camera/color/image_raw        Publisher count: 1
/perception/detection_image    Publisher count: 1
/perception/vision_defects     Publisher count: 1
```

### 查看窗口地址

```bash
/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/scripts/orbbec_defect_control.sh open
```

### 查看日志

```bash
/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/scripts/orbbec_defect_control.sh logs
```

日志目录：

```bash
/tmp/cyberluban_orbbec_defect_logs
```

## 三、手动启动命令

一般使用一键控制脚本即可。下面命令仅用于调试。

### 1. 启动 Orbbec 相机并回传 ROS2

```bash
source /opt/ros/humble/setup.bash
ros2 launch orbbec_camera orbbec_camera.launch.py camera_model:=gemini330_series usb_port:=3-1 enable_depth:=true enable_color:=true
```

相机成功连接时会看到类似：

```text
Device Orbbec Gemini 336L connected
color Frame - Width: 1280 Height: 720 fps: 10 Format: MJPG
depth Frame - Width: 1280 Height: 720 fps: 10 Format: Y16
```

### 2. 启动缺陷识别模型

```bash
source /opt/ros/humble/setup.bash
source /tmp/cyberluban_cri_install/setup.bash
export PYTHONPATH=/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/src/cri_perception/vision_defect_detector:$PYTHONPATH
python3 -m vision_defect_detector.vision_defect_detector_node --ros-args \
  -p image_topic:=/camera/color/image_raw \
  -p model_path:=/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/demo/models/road_damage_yolov8.pt \
  -p confidence_threshold:=0.25 \
  -p input_size:=640 \
  -p device:=cpu \
  -p enable_visualization:=true
```

模型成功启动时会看到类似：

```text
YOLO 模型加载成功
订阅图像: /camera/color/image_raw
发布: /perception/vision_defects, /perception/detection_image
```

### 3. 启动原始图像窗口

```bash
source /opt/ros/humble/setup.bash
python3 /tmp/orbbec_web_viewer.py
```

打开：

```text
http://localhost:8088/
```

### 4. 启动缺陷识别结果窗口

```bash
source /opt/ros/humble/setup.bash
source /tmp/cyberluban_cri_install/setup.bash
python3 /tmp/orbbec_detection_web_viewer.py
```

打开：

```text
http://localhost:8089/
```

## 四、手动停止命令

### 停止全部相关进程

```bash
pkill -f "orbbec_camera"
pkill -f "component_container"
pkill -f "vision_defect_detector"
pkill -f "orbbec_web_viewer"
pkill -f "orbbec_detection_web_viewer"
```

### 只停止显示窗口

```bash
pkill -f "orbbec_web_viewer"
pkill -f "orbbec_detection_web_viewer"
```

### 只停止识别模型

```bash
pkill -f "vision_defect_detector"
```

### 只停止相机驱动

```bash
pkill -f "orbbec_camera"
pkill -f "component_container"
```

## 五、常用检查命令

### 查看相机是否被 USB 识别

```bash
lsusb | grep -iE "orbbec|2bc5"
```

正常设备示例：

```text
Orbbec Gemini 336L
```

### 查看 ROS2 图像话题

```bash
source /opt/ros/humble/setup.bash
ros2 topic list | grep -E "camera|perception"
```

### 查看原始相机话题状态

```bash
source /opt/ros/humble/setup.bash
ros2 topic info /camera/color/image_raw
```

正常应包含：

```text
Publisher count: 1
```

### 查看识别结果图像话题状态

```bash
source /opt/ros/humble/setup.bash
source /tmp/cyberluban_cri_install/setup.bash
ros2 topic info /perception/detection_image
```

正常应包含：

```text
Publisher count: 1
```

### 查看缺陷识别消息

```bash
source /opt/ros/humble/setup.bash
source /tmp/cyberluban_cri_install/setup.bash
ros2 topic echo /perception/vision_defects --once
```

如果输出：

```yaml
defects: []
```

表示模型正在运行，但当前画面没有检测到缺陷目标。

## 六、故障处理

### 1. 8089 显示服务不可用

优先执行：

```bash
/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/scripts/orbbec_defect_control.sh restart
```

然后检查：

```bash
/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/scripts/orbbec_defect_control.sh status
```

### 2. 画面不刷新

检查图像帧接口：

```bash
curl -I http://localhost:8088/frame.jpg
curl -I http://localhost:8089/frame.jpg
```

正常应返回：

```text
HTTP/1.0 200 OK
Content-Type: image/jpeg
```

### 3. 相机打开失败或 UVC 报错

先停止旧进程：

```bash
/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/scripts/orbbec_defect_control.sh stop
```

再重新启动：

```bash
/home/qyb413/CyberLuban/nuc/robot/ros2_workspace_src/scripts/orbbec_defect_control.sh start
```

如果仍失败，检查是否有旧进程占用：

```bash
ps -ef | grep -iE "orbbec_camera|component_container|vision_defect_detector" | grep -v grep
```

### 4. USB 权限问题

如果日志中出现 USB 权限或 `uvc_open failed`，确认 udev 规则已生效：

```bash
sudo cp /opt/ros/humble/share/orbbec_camera/udev/99-obsensor-libusb.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

执行后拔插一次相机。

临时授权方式：

```bash
sudo chmod a+rw /dev/bus/usb/003/002
```

注意：设备路径可能随拔插变化，实际路径以 `lsusb` 和 `/dev/bus/usb/` 为准。

## 七、当前链路

```text
Orbbec Gemini 336L
  -> /camera/color/image_raw
  -> vision_defect_detector_node
  -> /perception/vision_defects
  -> /perception/detection_image
  -> Web 实时窗口 8088/8089
```
