#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
简化版Streamlit路面缺陷检测应用
支持两种模型：
  A. YOLOv8n (通用 COCO)     -> yolov8n.pt
  B. YOLOv8 (路面缺陷专用)   -> models/yolov8n-road-defect.pt (用户运行 train_road_defect.py 训练生成)
"""
import streamlit as st
import cv2
import numpy as np
from PIL import Image
from pathlib import Path
import tempfile

# -----------------------------
# 页面配置
# -----------------------------
st.set_page_config(
    page_title="路面缺陷检测系统",
    page_icon="🚗",
    layout="wide"
)

st.title("🚗 路面缺陷检测系统")
st.markdown("基于YOLOv8的实时路面缺陷检测")

# -----------------------------
# 常量 / 配置
# -----------------------------
# 7类路面缺陷 (RDD2020 / N-RDD2024 标准标注)
ROAD_DEFECT_ZH = {
    'D00': '横向裂缝',
    'D10': '纵向裂缝',
    'D20': '龟裂',
    'D40': '坑槽',
    'D43': '交叉口损坏',
    'D44': '沉降',
    'D50': '施工区域',
}
# 下标 -> 缺陷code（和data.yaml保持完全一致的顺序）
IDX2CODE = ['D00', 'D10', 'D20', 'D40', 'D43', 'D44', 'D50']

# 两种模型的候选 .pt 路径（按顺序查找，找到第一个存在的就用）
MODEL_CANDIDATES = {
    "YOLOv8n (通用)": [
        Path(__file__).parent / "yolov8n.pt",
        Path("yolov8n.pt"),
    ],
    "YOLOv8 (路面缺陷专用)": [
        Path(__file__).parent / "models" / "yolov8n-road-defect.pt",
        Path(__file__).parent / "runs" / "detect" / "train" / "weights" / "best.pt",
        Path(__file__).parent / "yolov8n-road-defect.pt",
        Path("models/yolov8n-road-defect.pt"),
        Path("runs/detect/train/weights/best.pt"),
    ],
}

# -----------------------------
# 侧边栏
# -----------------------------
st.sidebar.header("设置")
confidence_threshold = st.sidebar.slider("置信度阈值", 0.0, 1.0, 0.25, 0.05)
model_choice = st.sidebar.selectbox(
    "选择模型",
    list(MODEL_CANDIDATES.keys())  # 用字典key，保持顺序
)

# -----------------------------
# 辅助：找模型文件
# -----------------------------
def find_model_path(model_key: str):
    for p in MODEL_CANDIDATES[model_key]:
        if p.exists():
            return p
    return None

# -----------------------------
# 模型加载（按选项分开缓存，切模型不会互相影响）
# -----------------------------
@st.cache_resource(show_spinner=False)
def load_yolo_model(model_key: str):
    """根据下拉选择，加载对应模型"""
    try:
        from ultralytics import YOLO
    except Exception as e:
        return None, f"未安装 ultralytics 依赖: {e}"

    pt_path = find_model_path(model_key)
    if pt_path is None:
        return None, (
            f"找不到「{model_key}」的模型权重文件。\n\n"
            + ("请先在 Windows 终端执行训练脚本:\n"
               "  cd campus_road_inspection_ws\\demo\n"
               "  python train_road_defect.py\n\n"
               "训练完成后模型会自动输出到: demo\\runs\\detect\\train\\weights\\best.pt"
               if model_key.startswith("YOLOv8 (路面")
               else
               "请把 yolov8n.pt 放到 demo 目录下，或执行: pip install ultralytics 后首次 import 会自动下载。")
        )

    try:
        model = YOLO(str(pt_path))
        # 预热一次，首帧加速
        _ = model(np.zeros((640, 640, 3), dtype=np.uint8), conf=0.25, verbose=False)
        return model, None
    except Exception as e:
        return None, f"模型加载失败 ({pt_path.name}): {e}"


# -----------------------------
# 推理 + 绘制
# -----------------------------
def translate_class_name(raw_name: str, class_id: int, is_road_model: bool) -> str:
    """把通用/路面专用模型的原始类名，转成用户可读的中文标签"""
    if is_road_model:
        # 情况1：模型 names 已经是 D00/D10…  -> 查中文
        if raw_name in ROAD_DEFECT_ZH:
            return f"{ROAD_DEFECT_ZH[raw_name]} ({raw_name})"
        # 情况2：训练时 class_id 没写 names -> 用 IDX2CODE 下标
        if 0 <= class_id < len(IDX2CODE):
            code = IDX2CODE[class_id]
            return f"{ROAD_DEFECT_ZH[code]} ({code})"
        # 兜底：保持原类名
        return raw_name
    # 通用 COCO 模型：保持英文名 (person/car/...)
    return raw_name


def classify_image(image, model, model_key, conf_threshold=0.25):
    if model is None:
        return None, []

    is_road_model = model_key.startswith("YOLOv8 (路面")

    img_array = np.array(image)
    results = model(img_array, conf=conf_threshold, verbose=False)

    detections = []
    # 我们自己画框（把中文标签写进 det，最后用 YOLO plot 原图也行，但 plot 不改 label 名；
    # 折中：先用 plot 拿底图，详细列表再展示中文）
    for result in results:
        boxes = result.boxes
        for box in boxes:
            x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
            confidence = float(box.conf[0])
            class_id = int(box.cls[0])
            raw_name = model.names[class_id] if class_id in model.names else f"class{class_id}"
            zh_label = translate_class_name(raw_name, class_id, is_road_model)

            detections.append({
                'bbox': [int(x1), int(y1), int(x2), int(y2)],
                'confidence': confidence,
                'class_id': class_id,
                'class_name_raw': raw_name,
                'class_name': zh_label,
            })

    annotated = result.plot()
    annotated = cv2.cvtColor(annotated, cv2.COLOR_BGR2RGB)

    return annotated, detections


# -----------------------------
# 主界面
# -----------------------------
st.markdown("---")
col1, col2 = st.columns(2)

# ------ 左侧：上传 ------
with col1:
    st.subheader("📤 上传图片")
    uploaded_file = st.file_uploader(
        "选择路面图片",
        type=['jpg', 'jpeg', 'png'],
        help="支持JPG、JPEG、PNG格式"
    )

    if uploaded_file is not None:
        image = Image.open(uploaded_file)
        st.image(image, caption="原始图片", use_container_width=True)

# ------ 右侧：结果 ------
with col2:
    st.subheader("🔍 检测结果")

    # 先在侧边栏下方提示一下当前模型路径（方便用户 debug）
    current_pt = find_model_path(model_choice)
    with st.sidebar.expander("ℹ️ 模型路径信息", expanded=False):
        if current_pt is not None:
            st.markdown(f"**权重文件位置：**\n`{current_pt.resolve()}`")
        else:
            st.error("⚠️ 权重文件不存在（见正文黄色提示）")

    if uploaded_file is not None:
        # 1) 加载模型
        with st.spinner(f"正在加载模型「{model_choice}」..."):
            model, load_err = load_yolo_model(model_choice)

        if load_err is not None:
            st.warning(load_err)
        else:
            # 2) 推理
            with st.spinner("正在检测..."):
                annotated_img, detections = classify_image(
                    image, model, model_choice, confidence_threshold
                )

            if annotated_img is not None:
                st.image(annotated_img, caption="检测结果", use_container_width=True)

                # 3) 统计
                st.markdown("### 📊 检测统计")
                is_road = model_choice.startswith("YOLOv8 (路面")
                target_label = "路面缺陷" if is_road else "目标"

                if len(detections) > 0:
                    st.success(f"检测到 **{len(detections)}** 个{target_label}")

                    # 统计每个类别数量
                    from collections import Counter
                    cls_cnt = Counter(d['class_name'] for d in detections)
                    st.write("**按类别：**")
                    for name, cnt in cls_cnt.most_common():
                        st.markdown(f"- {name}：**{cnt}** 个")

                    with st.expander("查看每一个检测框的详情"):
                        for i, det in enumerate(detections, 1):
                            st.markdown(f"**{i}. {det['class_name']}**")
                            st.markdown(
                                f"- 置信度: {det['confidence']:.2%}　|　位置: {det['bbox']}"
                            )
                else:
                    st.info(f"未检测到{target_label}")

# -----------------------------
# 底部
# -----------------------------
st.markdown("---")
with st.expander("💡 使用说明 / 训练自己的路面检测模型（点我展开）", expanded=False):
    st.markdown("""
#### 快速使用
1. 左侧边栏选择模型（通用/路面缺陷专用）
2. 上传路面图片（JPG / PNG）
3. 调整置信度阈值，实时查看检测框变化

#### 训练路面缺陷专用模型（得到下拉框第 2 个选项的权重）
1. **准备数据集**：打开 `demo/Road-Damage-and-Defect-Recognition-Model/Compiled Dataset of N-RDD2024` 里的 Google Drive 链接，下载 N-RDD2024 数据集；或使用任何 RDD2020/2022 YOLO 格式数据集，按以下结构放到 `demo/datasets/rdd2024/`：
   ```
   demo/datasets/rdd2024/
   ├── images/
   │   ├── train/    (xxx.jpg)
   │   └── val/      (yyy.jpg)
   └── labels/
       ├── train/    (xxx.txt, YOLO 归一化 xywh)
       └── val/      (yyy.txt)
   ```
2. **一键训练**：
   ```bash
   cd campus_road_inspection_ws\\demo
   python train_road_defect.py
   ```
   训练约 30~100 epochs 后，权重会自动保存到：
   `demo/runs/detect/train/weights/best.pt`
   脚本会同时复制一份到 `demo/models/yolov8n-road-defect.pt`，这样下拉框就能直接找到。
3. 回到 Streamlit 界面，选「YOLOv8 (路面缺陷专用)」即可看到 D00~D50 七类中文缺陷识别。
""")

if st.sidebar.button("关于系统"):
    st.sidebar.info("""
**路面缺陷检测系统**

版本: v1.1 路面专用模型接入版
模型: YOLOv8 (COCO 通用 + RDD2024 七类专用)
框架: Streamlit + Ultralytics
""")
