# 海康相机真实画面验证

## 数据流

```text
MV-CS016-10GC (10.7.142.102)
        │ GigE / MVS SDK
        ▼
hikrobot_camera_node
        │ sensor_msgs/msg/Image
        ├── /camera/image_raw ──▶ vision_defect_detector_node
        │                              └── /perception/detection_image
        └── /camera/camera_info
```

UE5 蓝图订阅文档中的 `/R2UTopic_Pos` 是位姿通道，不是图像通道；本次相机验证先在 ROS2 内确认图像，再决定是否通过独立视频链路提供给 UE5。不要把原始 `sensor_msgs/Image` 直接塞进现有位姿 rosbridge 通道。

## MVS SDK

仓库没有携带海康 SDK 二进制。安装 Linux MVS 后，确保 Python wrapper 和运行库可见：

官方 Linux 包（当前下载中心版本）：

```text
MVS V5.0.2 Linux
约 816 MB
```

NUC 上可以直接运行仓库提供的安装脚本：

```bash
cd /home/qyb413/CyberLuban/campus_road_inspection_ws
sudo ./scripts/install_hikrobot_camera_deps.sh
```

脚本会安装 OpenCV、NumPy、ROS `cv_bridge`/`rqt_image_view`、`camera_aravis2`，下载并解压官方 MVS SDK，然后验证 Python wrapper 能否导入。

```text
MvCameraControl_class.py
libMvCameraControl.so
```

如果 wrapper 不在默认目录，设置参数或环境变量：

```bash
export MVCAM_SDK_PATH=/opt/MVS/Samples/64/Python
```

也可以在 `src/cri_drivers/hikrobot_camera/config/camera_params.yaml` 中填写：

```yaml
mvs_python_path: "/path/to/MvCameraControl.py所在目录"
```

## 只启动相机

```bash
cd /home/qyb413/CyberLuban/campus_road_inspection_ws
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 launch hikrobot_camera camera_validation.launch.py
```

也可以直接执行完整检查（网卡、ARP、MVS、构建、ROS 首帧）：

```bash
./scripts/verify_hikrobot_camera.sh
```

默认参数使用：

```text
backend=mvs
camera_ip=10.7.142.102
```

要强制确认 MVS，而不是回退到测试图：

```bash
ros2 run hikrobot_camera hikrobot_camera_node --ros-args \
  -p backend:=mvs \
  -p camera_ip:=10.7.142.102
```

## 验证真实帧

另开终端执行：

```bash
source /opt/ros/humble/setup.bash
ros2 topic hz /camera/image_raw
ros2 topic echo /camera/camera_info --once
ros2 run rqt_image_view rqt_image_view
```

在 `rqt_image_view` 中选择 `/camera/image_raw`。节点日志必须出现：

```text
实际后端: mvs
MVS SDK 相机打开成功
```

如果日志显示“当前发布诊断测试图”，说明没有按真实相机配置启动；真实验证不要使用 `backend=auto`，应让 `backend=mvs` 在 SDK 或相机不可用时直接失败。

## 接入视觉识别

确认真实帧后，再启动视觉节点：

```bash
ros2 run vision_defect_detector vision_defect_detector_node
ros2 topic hz /perception/detection_image
ros2 topic echo /perception/vision_defects
```

识别节点无需修改，仍订阅 `/camera/image_raw`。完整系统启动时，`drivers.launch.py` 会加载同一份 `camera_params.yaml`。
