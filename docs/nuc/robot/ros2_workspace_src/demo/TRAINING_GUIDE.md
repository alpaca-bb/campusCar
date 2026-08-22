# 路面缺陷检测模型训练 - 完整指南

## ✅ 已完成工作

1. ✅ 创建训练脚本: `train_road_damage_yolov8.py`
2. ✅ 创建数据集配置: `models/n_rdd2024.yaml`
3. ✅ 更新Streamlit应用: 支持多模型选择
4. ✅ GitHub仓库已克隆: `Road-Damage-and-Defect-Recognition-Model/`

## 📊 Streamlit应用更新

### 新功能
- ✅ **多模型选择**:
  - YOLOv8n (通用模型) - 立即可用
  - 路面缺陷专用模型 - 训练后可用
  - RDD2020预训练模型 - 下载后可用

- ✅ **缺陷类别显示**: 中文标注
- ✅ **示例图片**: 可选择示例测试
- ✅ **详细统计**: 类别分布和置信度
- ✅ **训练说明**: 内置完整教程

### 运行方式
```bash
cd demo
streamlit run streamlit_app.py
```

## 🎓 训练路面缺陷模型

### 方案1: 完整训练 (推荐GPU)

#### 步骤1: 下载数据集
```bash
# 查看数据集链接
cat Road-Damage-and-Defect-Recognition-Model/"Compiled Dataset of N-RDD2024"

# 从Google Drive下载数据集
# 解压到 datasets/n_rdd2024/
```

#### 步骤2: 准备数据结构
```
datasets/n_rdd2024/
├── images/
│   ├── train/          # 训练图片
│   └── val/            # 验证图片
└── labels/
    ├── train/          # 训练标注 (.txt)
    └── val/            # 验证标注 (.txt)
```

#### 步骤3: 运行训练
```bash
python train_road_damage_yolov8.py
```

**训练参数**:
- Epochs: 50 (建议100+用于完整训练)
- Batch: 16 (根据显存调整)
- Image Size: 640x640
- 预计时间: 2-4小时 (GPU) / 数天 (CPU)

#### 步骤4: 使用训练好的模型
```bash
# 复制最佳模型
cp road_damage_training/yolov8n_nrdd2024/weights/best.pt models/road_damage_yolov8.pt

# 在Streamlit中选择"路面缺陷专用模型"
```

### 方案2: 使用预训练模型 (最快)

#### 从Roboflow下载
```bash
# 1. 访问 https://universe.roboflow.com/
# 2. 搜索 "road damage detection yolov8"
# 3. 选择项目并导出 YOLOv8 PyTorch
# 4. 下载 .pt 文件
# 5. 放置到 models/ 目录
```

#### 推荐项目
- RDD2020 Road Damage Detection
- Road Defect Detection
- Pothole and Crack Detection

### 方案3: 微调现有模型
```python
from ultralytics import YOLO

# 加载预训练模型
model = YOLO('yolov8n.pt')

# 少量epoch微调
model.train(
    data='models/n_rdd2024.yaml',
    epochs=20,  # 快速微调
    imgsz=640,
    batch=8
)
```

## 📝 数据集配置详情

### n_rdd2024.yaml
```yaml
path: ./datasets/n_rdd2024
train: images/train
val: images/val

nc: 10
names: ['D00', 'D10', 'D20', 'D30', 'D40', 'D50', 'D60', 'D70', 'D80', 'D90']
```

### 类别定义
- **D00**: 横向裂缝
- **D10**: 纵向裂缝
- **D20**: 龟裂
- **D30**: 边缘裂缝
- **D40**: 坑槽
- **D50**: 修补区域
- **D60**: 井盖破损
- **D70**: 路面标线损坏
- **D80**: 排水设施损坏
- **D90**: 其他损坏

## 🎯 模型集成状态

| 模型 | 状态 | 路径 | 说明 |
|------|------|------|------|
| YOLOv8n通用 | ✅ 可用 | yolov8n.pt | 自动下载 |
| 路面缺陷专用 | ⏳ 待训练 | models/road_damage_yolov8.pt | 需训练 |
| RDD2020预训练 | ⏳ 待下载 | models/rdd2020_best.pt | 需下载 |

## 🚀 立即使用

### 1. 启动Web应用
```bash
streamlit run streamlit_app.py
```

### 2. 选择模型
- 当前可用: YOLOv8n通用模型
- 未来可选: 训练后的专用模型

### 3. 上传图片
- 选择路面图片
- 或使用示例图片
- 调整置信度阈值

### 4. 查看结果
- 检测框和类别
- 统计信息
- 详细列表

## 💡 建议

### 演示用途
- ✅ 使用当前的YOLOv8n通用模型
- ✅ 展示完整的Web界面和功能
- ✅ 说明可集成专用训练模型

### 生产部署
- ⏳ 下载或训练路面缺陷专用模型
- ⏳ 使用RDD2020数据集训练
- ⏳ 在真实路面图片上测试

## 📊 训练监控

训练过程中可以查看:
```bash
# TensorBoard
tensorboard --logdir road_damage_training

# 或查看训练日志
cat road_damage_training/yolov8n_nrdd2024/results.txt
```

## ✅ 总结

**当前状态**:
- ✅ Streamlit应用已更新
- ✅ 支持多模型切换
- ✅ 通用模型立即可用
- ✅ 训练脚本和配置就绪
- ⏳ 等待数据集下载和训练

**立即可演示**:
```bash
streamlit run streamlit_app.py
```
选择"YOLOv8n (通用模型)"即可使用！

**训练专用模型后**:
选择"路面缺陷专用模型"获得更好的路面检测效果！
