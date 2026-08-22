# 硬件连接与模型训练指南

## 📷 需要连接相机的地方

### 1. 海康工业相机驱动节点
**文件**: `src/cri_drivers/hikrobot_camera/hikrobot_camera/hikrobot_camera_node.py`

**需要的硬件**:
- 海康 MV-CS016-10GC GigE 工业相机
- 千兆网线连接到计算机
- 海康 MVS SDK（MVS 后端需要；节点会自动搜索常见安装目录）

**连接方式**:
节点通过 `mvs_camera.py` 封装 MVS 枚举、打开、取帧和像素格式转换，并统一输出 BGR 图像。`backend` 支持 `auto`、`mvs`、`opencv`、`test`；配置文件默认使用 `backend=mvs`，避免把测试图误认为真实画面。

**替代方案（如果没有硬件）**:
- 使用USB摄像头: `cv2.VideoCapture(0)`
- 使用rosbag录制的测试数据
- 使用模拟发布器（已提供）

---

## 🔦 需要连接激光雷达的地方

### 1. Livox雷达驱动节点
**文件**: `src/cri_drivers/livox_driver/livox_driver/livox_driver_node.py`

**需要的硬件**:
- Livox Mid-360S 固态激光雷达
- 网线连接（雷达默认IP: 192.168.1.1xx）
- Livox SDK2

**连接方式**:
```python
# 当前是占位代码，需要替换为官方SDK
# 需要安装: pip install livox-sdk-python
# 或使用官方的 livox_ros_driver2

# 第52-66行需要改为真实的SDK初始化
import livox_sdk

def init_livox():
    livox_sdk.init()
    # 设置回调
    livox_sdk.set_point_cloud_callback(self.on_pointcloud)
    # 连接设备
    livox_sdk.connect("192.168.1.100")
```

**替代方案（推荐）**:
使用官方驱动包而不是重新封装:
```bash
# 克隆官方驱动
cd ~/campus_road_inspection_ws/src
git clone https://github.com/Livox-SDK/livox_ros_driver2.git

# 编译
colcon build --packages-select livox_ros_driver2

# 配置并启动
ros2 launch livox_ros_driver2 msg_MID360_launch.py
```

**替代方案（如果没有硬件）**:
- 使用模拟点云发布器（已提供）
- 使用公开的点云数据集（KITTI、nuScenes等）
- 录制rosbag数据

---

## 🤖 需要训练YOLO模型的地方

### 1. 视觉语义缺陷检测节点
**文件**: `src/cri_perception/vision_defect_detector/vision_defect_detector/vision_defect_detector_node.py`

**当前状态**: 
- 代码已实现YOLOv8推理流程（第100-150行）
- 缺少实际的模型文件
- 需要训练或获取预训练模型

**需要的模型**:
- YOLOv8n格式（轻量级）
- OpenVINO IR格式（.xml + .bin）
- 5类路面缺陷：crack, pothole, tile_uplift, depression, construction

---

## 📊 数据集与模型获取方案

### 方案1: 使用公开的路面缺陷数据集（推荐）

#### 1.1 RDD2020（Road Damage Detection）
**来源**: https://github.com/sekilab/RoadDamageDetector

**特点**:
- ✅ 最适合我们的任务
- 包含26,620张标注图像
- 4个国家的路面数据（日本、印度、捷克、挪威）
- 8种缺陷类型（可合并为我们的5类）
- 已有YOLO格式标注

**下载**:
```bash
# 1. 访问官网下载
# https://data.mendeley.com/datasets/5ty2wb6gvg/1

# 2. 或使用Kaggle数据集
kaggle datasets download -d chitholian/road-damage-detection-dataset
```

**类别映射**:
```python
# RDD2020 → 我们的类别
RDD_to_CRI = {
    'D00': 'crack',      # 横向裂缝
    'D10': 'crack',      # 纵向裂缝  
    'D20': 'crack',      # 龟裂
    'D40': 'pothole',    # 坑槽
    'D43': 'pothole',    # 十字交叉口损坏
    'D44': 'depression', # 沉降
    'D50': 'construction' # 施工区域
}
```

#### 1.2 CFD（Concrete Fracture Dataset）
**来源**: https://github.com/cuilimeng/CrackForest-dataset

**特点**:
- 专注于裂缝检测
- 高分辨率图像
- 可作为crack类别的补充

#### 1.3 CrackSegmentation Dataset
**来源**: https://www.kaggle.com/datasets/lakshaymiddha/crack-segmentation-dataset

**特点**:
- 5000+张裂缝图像
- 像素级标注
- 适合语义分割（可转为目标检测）

---

### 方案2: 使用预训练模型（最快方案）

#### 2.1 直接使用RDD2020预训练模型
**来源**: https://github.com/sekilab/RoadDamageDetector/tree/master/RDD2020

**已提供模型**:
- YOLOv5预训练权重
- 可直接转换为YOLOv8
- 性能已验证（mAP ~60%）

**使用步骤**:
```bash
# 1. 克隆仓库
git clone https://github.com/sekilab/RoadDamageDetector.git

# 2. 下载预训练权重
cd RoadDamageDetector/RDD2020
# 权重文件: best.pt

# 3. 转换为YOLOv8格式（如需要）
# 或直接使用YOLOv5
```

#### 2.2 使用通用YOLOv8模型微调
**来源**: Ultralytics官方

```python
from ultralytics import YOLO

# 加载预训练模型
model = YOLO('yolov8n.pt')  # nano版本

# 在RDD2020数据集上微调
model.train(
    data='rdd2020.yaml',
    epochs=100,
    imgsz=640,
    batch=16,
    device=0  # GPU
)
```

---

## 🔧 完整训练流程（推荐方案）

### Step 1: 准备数据集

#### 下载RDD2020
```bash
mkdir -p ~/datasets/rdd2020
cd ~/datasets/rdd2020

# 手动从 https://data.mendeley.com/datasets/5ty2wb6gvg/1 下载
# 或使用Kaggle
kaggle datasets download -d chitholian/road-damage-detection-dataset
unzip road-damage-detection-dataset.zip
```

#### 转换为YOLOv8格式
```bash
# 创建转换脚本
cat > convert_rdd_to_yolo.py << 'EOF'
import json
import os
from pathlib import Path

# RDD2020类别映射
class_mapping = {
    'D00': 0,  # crack
    'D10': 0,  # crack
    'D20': 0,  # crack
    'D40': 1,  # pothole
    'D43': 1,  # pothole
    'D44': 2,  # depression
    'D50': 4   # construction
}

# 处理标注文件...
# （具体转换代码）
EOF

python convert_rdd_to_yolo.py
```

#### 创建数据集配置
```yaml
# rdd2020_cri.yaml
path: /home/user/datasets/rdd2020_cri
train: images/train
val: images/val
test: images/test

nc: 5
names: ['crack', 'pothole', 'tile_uplift', 'depression', 'construction']
```

---

### Step 2: 训练YOLOv8模型

```python
#!/usr/bin/env python3
# train_road_defect_yolo.py

from ultralytics import YOLO

# 1. 加载预训练模型
model = YOLO('yolov8n.pt')

# 2. 训练
results = model.train(
    data='rdd2020_cri.yaml',
    epochs=100,
    imgsz=640,
    batch=16,
    device=0,  # 使用GPU
    project='road_defect_detection',
    name='yolov8n_rdd2020',
    
    # 优化参数
    patience=20,
    save=True,
    save_period=10,
    
    # 数据增强
    hsv_h=0.015,
    hsv_s=0.7,
    hsv_v=0.4,
    degrees=10.0,
    translate=0.1,
    scale=0.5,
    mosaic=1.0,
)

# 3. 验证
metrics = model.val()
print(f"mAP50: {metrics.box.map50}")
print(f"mAP50-95: {metrics.box.map}")

# 4. 导出为ONNX
model.export(format='onnx')

print("训练完成！模型保存在: road_defect_detection/yolov8n_rdd2020/weights/best.pt")
```

**运行**:
```bash
python train_road_defect_yolo.py
```

**预期结果**:
- 训练时间: 2-4小时（GPU）
- mAP50: ~60-70%
- 模型大小: ~6MB

---

### Step 3: 转换为OpenVINO格式

```bash
# 方式1：使用YOLOv8内置导出
from ultralytics import YOLO
model = YOLO('road_defect_detection/yolov8n_rdd2020/weights/best.pt')
model.export(format='openvino')

# 方式2：使用OpenVINO工具
mo --input_model best.onnx \
   --output_dir openvino_model \
   --data_type FP16

# 输出文件:
# - best.xml (模型结构)
# - best.bin (权重)
```

---

### Step 4: 集成到项目

```bash
# 1. 复制模型文件
cp best.xml ~/campus_road_inspection_ws/src/cri_perception/vision_defect_detector/models/road_defect_yolov8n.xml
cp best.bin ~/campus_road_inspection_ws/src/cri_perception/vision_defect_detector/models/road_defect_yolov8n.bin

# 2. 更新配置
nano src/cri_perception/vision_defect_detector/config/vision_params.yaml
# 修改 model_path 为实际路径
```

---

## 🚀 快速启动方案（无硬件测试）

### 使用模拟数据 + 预训练模型

```bash
# 1. 下载我为你准备的预训练模型（示例）
# 实际需要你从RDD2020训练或下载

# 2. 启动模拟数据发布
python test/mock_lidar_pub.py &
python test/mock_camera_pub.py &

# 3. 启动感知链路（会使用模拟检测）
ros2 launch cri_bringup perception.launch.py

# 4. 验证输出
ros2 topic echo /perception/vision_defects
```

---

## 📝 总结与建议

### 立即可做（无需硬件）

1. **下载RDD2020数据集** ✅
   - 链接: https://data.mendeley.com/datasets/5ty2wb6gvg/1
   - 或Kaggle搜索 "Road Damage Detection"

2. **使用预训练模型** ✅
   - RDD2020官方已提供YOLOv5权重
   - 可直接转换使用

3. **模拟测试** ✅
   - 使用mock数据发布器
   - 验证完整链路

### 短期任务（有GPU环境）

1. **训练自己的模型** ⏳
   - 使用提供的训练脚本
   - 微调为5类缺陷
   - 转换为OpenVINO

2. **性能优化** ⏳
   - 量化为INT8
   - 测试推理速度

### 中期任务（有硬件）

1. **对接Livox雷达** ⏳
   - 使用官方livox_ros_driver2
   - 配置IP和参数

2. **对接海康相机** ⏳
   - 安装 MVS SDK 并让 `MvCameraControl_class.py` 和 `libMvCameraControl.so` 可见
   - 设置相机 IP `10.7.142.102`
   - 运行 `ros2 launch hikrobot_camera camera_validation.launch.py`
   - 使用 `rqt_image_view` 查看 `/camera/image_raw`

---

## 🔗 资源链接汇总

### 数据集
- **RDD2020**: https://data.mendeley.com/datasets/5ty2wb6gvg/1
- **Kaggle镜像**: https://www.kaggle.com/datasets/chitholian/road-damage-detection-dataset
- **CrackForest**: https://github.com/cuilimeng/CrackForest-dataset

### 预训练模型
- **RDD2020官方**: https://github.com/sekilab/RoadDamageDetector
- **YOLOv8官方**: https://github.com/ultralytics/ultralytics

### SDK与驱动
- **Livox SDK2**: https://github.com/Livox-SDK/Livox-SDK2
- **Livox ROS Driver2**: https://github.com/Livox-SDK/livox_ros_driver2
- **海康MVS SDK**: https://www.hikrobotics.com/cn/machinevision/service/download

---

**当前状态**: 所有代码已就绪，只缺模型文件  
**推荐方案**: 先用RDD2020预训练模型测试，后续再微调  
**测试方式**: 使用模拟数据验证完整链路

需要我帮你下载数据集或编写训练脚本吗？
