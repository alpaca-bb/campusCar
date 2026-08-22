#!/usr/bin/env python3
# -*- coding: utf-8 -*-
r"""
路面缺陷专用 YOLOv8 训练脚本
============================
功能:
    1. 一键创建 datasets/rdd2024 的目录骨架
    2. 检查训练/验证图片是否齐全
    3. 调用 ultralytics YOLO 训练 yolov8n 作为 backbone
    4. 训练完成后自动把 best.pt 复制到 models/yolov8n-road-defect.pt
       (streamlit_app.py 下拉框"YOLOv8 (路面缺陷专用)"会自动识别它)

使用步骤 (Windows PowerShell / CMD):
    cd campus_road_inspection_ws\demo
    # 1) 先把 N-RDD2024 的 images 和 labels 按要求放到 datasets/rdd2024/
    # 2) 直接训练:
    python train_road_defect.py

数据集目录结构要求:
    demo/
    ├─ data.yaml                ← 本脚本同目录已自带
    ├─ train_road_defect.py
    └─ datasets/
        └─ rdd2024/
            ├─ images/
            │   ├─ train/      任意数量 .jpg / .png
            │   └─ val/        任意数量 .jpg / .png
            └─ labels/
                ├─ train/      与图片同名的 .txt (YOLO xywh 归一化)
                └─ val/        与图片同名的 .txt

数据获取:
    N-RDD2024 数据集下载链接 (已在项目中保存):
        demo/Road-Damage-and-Defect-Recognition-Model/Compiled Dataset of N-RDD2024
    打开文件内 Google Drive 链接 → 下载 India / Japan / Czech 的数据，
    按 images/ + labels/ 配对放置即可 (txt 用 YOLO 格式, 0~6 共 7 类)。
"""
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parent          # demo/
DATA_YAML = ROOT / "data.yaml"
DATASET_DIR = ROOT / "datasets" / "rdd2024"
MODELS_DIR = ROOT / "models"
BEST_PT_SRC = ROOT / "runs" / "detect" / "train" / "weights" / "best.pt"
BEST_PT_DST = MODELS_DIR / "yolov8n-road-defect.pt"


def ensure_dirs():
    """确保需要的目录都存在，不存在就建骨架 + 给提示"""
    for p in [
        DATASET_DIR / "images" / "train",
        DATASET_DIR / "images" / "val",
        DATASET_DIR / "labels" / "train",
        DATASET_DIR / "labels" / "val",
        MODELS_DIR,
    ]:
        p.mkdir(parents=True, exist_ok=True)


def count_images_and_labels(subset: str):
    """统计 train/val 的图片和标签数"""
    img_dir = DATASET_DIR / "images" / subset
    lbl_dir = DATASET_DIR / "labels" / subset
    imgs = list(img_dir.glob("*.jpg")) + list(img_dir.glob("*.png")) + list(img_dir.glob("*.JPG"))
    txts = set(p.stem for p in lbl_dir.glob("*.txt"))
    matched = sum(1 for i in imgs if i.stem in txts)
    return len(imgs), len(txts), matched


def banner(msg: str, code: str = "="):
    n = max(60, len(msg) + 8)
    print("\n" + code * n)
    print(f"  {msg}")
    print(code * n + "\n")


def main():
    banner("路面缺陷 YOLOv8 训练脚本启动")

    ensure_dirs()

    # 1) 检查依赖
    try:
        from ultralytics import YOLO
    except ImportError:
        print("[X] 缺少 ultralytics！请先:\n    pip install -r requirements.txt")
        sys.exit(1)

    backbone = ROOT / "yolov8n.pt"
    if not backbone.exists():
        print(f"[!] 找不到预训练 backbone {backbone.name}，首次训练会自动从 Ultralytics 服务器下载 (~6MB)")

    # 2) 检查数据集
    tr_imgs, tr_txts, tr_match = count_images_and_labels("train")
    va_imgs, va_txts, va_match = count_images_and_labels("val")

    print(f"[数据集检查] train: 图片={tr_imgs} 标签={tr_txts} 配对成功={tr_match}")
    print(f"[数据集检查] val  : 图片={va_imgs} 标签={va_txts} 配对成功={va_match}")

    if tr_imgs == 0 or va_imgs == 0:
        banner("数据集为空 ❌", code="-")
        print(
            "请按以下结构放入 RDD2020 / N-RDD2024 YOLO 格式数据:\n"
            f"  {DATASET_DIR}\\images\\train\\  (放训练图 .jpg/.png)\n"
            f"  {DATASET_DIR}\\images\\val\\    (放验证图 .jpg/.png)\n"
            f"  {DATASET_DIR}\\labels\\train\\  (对应同名 .txt, YOLO xywh, class 0-6)\n"
            f"  {DATASET_DIR}\\labels\\val\\    (对应同名 .txt)\n\n"
            "N-RDD2024 下载链接见文件:\n"
            "  Road-Damage-and-Defect-Recognition-Model\\Compiled Dataset of N-RDD2024"
        )
        sys.exit(2)

    if tr_match < max(1, tr_imgs // 2):
        print("[!] 警告: 训练集中超过一半的图片没有匹配的 .txt 标签，训练效果会很差。")

    # 3) 训练
    banner("开始训练 🚀")

    # 启动训练: yolov8n nano 最快，640 标准尺寸，50 epochs 够用
    model = YOLO(str(backbone) if backbone.exists() else "yolov8n.pt")

    # 根据显存大小自动选 batch：无 GPU 时 batch=8 防 CPU OOM
    import torch
    cuda_ok = torch.cuda.is_available()
    batch = 16 if cuda_ok else 8
    device = 0 if cuda_ok else "cpu"
    print(f"[训练参数] device={'GPU:' + torch.cuda.get_device_name(0) if cuda_ok else 'CPU'}  batch={batch}")

    results = model.train(
        data=str(DATA_YAML),
        epochs=50,
        imgsz=640,
        batch=batch,
        device=device,
        patience=15,           # 15 个 epoch mAP 不涨就早停
        save=True,
        project=str(ROOT / "runs" / "detect"),
        name="train",
        exist_ok=True,         # 复跑直接覆盖老的 train/ 目录
        verbose=True,
    )

    # 4) 训练结束 -> 复制 best.pt 到 models/
    banner("训练结束，复制模型权重")
    if BEST_PT_SRC.exists():
        BEST_PT_DST.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(BEST_PT_SRC, BEST_PT_DST)
        print(f"[OK] best.pt 已复制到: {BEST_PT_DST}")
        print("\n现在回到 Streamlit 界面:")
        print("  在左侧『选择模型』下拉框选『YOLOv8 (路面缺陷专用)』即可使用！")
    else:
        print(f"[!] 训练完成但找不到 {BEST_PT_SRC}，请检查 runs/detect/train/weights/ 目录")
        sys.exit(3)


if __name__ == "__main__":
    main()
