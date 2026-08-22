# 🚗 路面缺陷检测Demo

基于YOLOv8的路面缺陷实时检测演示程序

## ✨ 功能特点

- ✅ 支持图片、视频、摄像头实时检测
- ✅ 自动下载示例图片
- ✅ 可视化检测结果（边界框+类别+置信度）
- ✅ FPS实时显示
- ✅ 支持批量处理
- ✅ 自动保存结果

## 🎯 检测类别

- crack (裂缝)
- pothole (坑槽)
- alligator_crack (龟裂)
- longitudinal_crack (纵向裂缝)
- transverse_crack (横向裂缝)

## 📦 快速安装

### 1. 安装依赖

```bash
# 安装ultralytics（YOLOv8官方库）
pip install ultralytics

# 或者使用requirements.txt
pip install -r requirements.txt
```

### 2. 验证安装

```bash
python -c "from ultralytics import YOLO; print('✅ YOLOv8安装成功!')"
```

## 🚀 快速开始

### 方式1：使用示例图片（推荐）

```bash
# 自动下载并检测示例图片
python demo_road_defect_detection.py --source demo
```

**首次运行会自动**:
1. 下载YOLOv8n模型（~6MB）
2. 下载示例路面图片
3. 执行检测并可视化

### 方式2：检测自己的图片

```bash
# 单张图片
python demo_road_defect_detection.py --source path/to/image.jpg

# 多张图片（批量处理）
python demo_road_defect_detection.py --source path/to/images/
```

### 方式3：检测视频

```bash
# 视频文件
python demo_road_defect_detection.py --source path/to/video.mp4

# 摄像头实时检测
python demo_road_defect_detection.py --source 0
```

## 📸 使用预训练模型

如果你有专门训练的路面缺陷模型，可以使用：

```bash
# 1. 将模型文件放到 models/ 目录
mkdir models
# 复制你的模型: road_damage_yolov8.pt

# 2. 使用预训练模型
python demo_road_defect_detection.py --source demo --model pretrained
```

## 🎨 运行效果

### 示例输出

```
==============================================================
🚗 路面缺陷检测Demo
==============================================================

📦 初始化模型...
✅ 加载通用YOLOv8模型

📥 下载示例图片...
📥 下载示例图片: crack_sample.jpg
✅ 下载完成: demo/samples/crack_sample.jpg

🔍 开始检测...
📷 处理图片: demo/samples/crack_sample.jpg
   尺寸: 1920x1080
   检测到 3 个缺陷 (耗时: 0.15s)
💾 结果已保存: demo/output/result_crack_sample.jpg
```

### 可视化界面

检测结果会显示:
- ✅ 彩色边界框
- ✅ 缺陷类别标签
- ✅ 置信度分数
- ✅ 统计信息（检测数量、FPS等）

按任意键查看下一张，按 'q' 退出视频模式。

## 📂 输出目录

```
demo/
├── samples/           # 示例图片（自动下载）
└── output/           # 检测结果
    ├── result_crack_sample.jpg
    └── result_video.mp4
```

## ⚙️ 高级参数

```bash
# 指定输出目录
python demo_road_defect_detection.py --source image.jpg --output my_results/

# 调整置信度阈值（代码内修改）
# 编辑 demo_road_defect_detection.py，找到 conf_threshold=0.25，改为0.5
```

## 🔗 获取更好的模型

### 方案1: RDD2020预训练模型

从以下来源获取专门的路面缺陷检测模型:

1. **GitHub**: https://github.com/sekilab/RoadDamageDetector
2. **Roboflow**: https://universe.roboflow.com/road-damage-wbtt6/rdd20
3. **Kaggle**: 搜索 "YOLOv8 pothole detection"

### 方案2: 自己训练

参考 `docs/HARDWARE_AND_MODEL_GUIDE.md` 中的训练教程。

## 🐛 常见问题

### 1. 模块未找到错误

```
ModuleNotFoundError: No module named 'ultralytics'
```

**解决**: 
```bash
pip install ultralytics opencv-python
```

### 2. 下载示例图片失败

**解决**: 
手动下载路面图片并使用 `--source path/to/image.jpg`

### 3. GPU支持

如果有NVIDIA GPU，安装CUDA版PyTorch可加速:
```bash
# 查看CUDA版本
nvidia-smi

# 安装对应版本的PyTorch（示例：CUDA 11.8）
pip install torch torchvision --index-url https://download.pytorch.org/whl/cu118
```

## 📊 性能参考

| 硬件 | FPS | 备注 |
|------|-----|------|
| CPU (Intel i7) | 10-15 FPS | YOLOv8n |
| GPU (RTX 3060) | 60-80 FPS | YOLOv8n |
| GPU (RTX 4090) | 150+ FPS | YOLOv8n |

## 🎓 扩展应用

这个Demo可以作为基础，扩展为:

1. **Web应用**: 集成Flask/FastAPI提供API服务
2. **移动端**: 转换为ONNX/TFLite部署到手机
3. **边缘设备**: 部署到树莓派/Jetson Nano
4. **ROS2集成**: 作为ROS2节点运行（已在项目中集成）

## 📖 相关文档

- YOLOv8官方文档: https://docs.ultralytics.com/
- 项目完整文档: `../docs/`
- 硬件与模型指南: `../docs/HARDWARE_AND_MODEL_GUIDE.md`

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📄 开源协议

MIT License

---

**快速开始**: `python demo_road_defect_detection.py --source demo`

**祝检测顺利！** 🎉
