# 校园道路缺陷检测训练报告

## 结论

最终模型为 `models/campus_road_best.pt`，架构是 YOLO11n 检测模型，输入尺寸 960，部署建议置信度阈值从 `0.40` 开始调整。

原有两个模型实际为同一个 YOLO11s 分割模型。项目的人工标签是检测框，不能原样续训分割头。本次将旧模型的 493/499 个兼容权重迁移到 YOLO11s 检测架构进行训练，并与标准 YOLO11n 检测预训练权重做同集对照。标准 YOLO11n 在独立验证集上更好且更轻，因此被选为最终模型。

| 候选 | 权重来源 | 大小 | pothole mAP50 | pothole Recall |
|---|---|---:|---:|---:|
| YOLO11s-detect | 旧道路分割模型迁移骨干 | 19.3 MB | 0.327 | 0.375 |
| YOLO11n-detect | 标准检测预训练 | 5.5 MB | **0.446** | 0.375 |

## 数据与划分

- 44 张人工审核图片，92 个框：`crack=4`、`pothole=88`、`rutting=0`。
- 按两段连拍序列分别抽取验证图，36 张训练、8 张验证，避免完全随机拆分造成相邻帧泄漏。
- 验证集为 `crack=1`、`pothole=16`；`rutting` 无样本，无法训练或验证该类。
- 训练时使用在线几何和颜色增强，不再预先生成增强图；验证集不做增强。

## 验证结果

独立 8 张 holdout：

| 指标 | 总体 | pothole | crack |
|---|---:|---:|---:|
| mAP50 | 0.223 | 0.446 | 0.000 |
| mAP50-95 | 0.120 | 0.240 | 0.000 |
| Recall | 0.188 | 0.375 | 0.000 |

全 44 张回代结果为 mAP50=0.690、mAP50-95=0.418、Precision=0.876、Recall=0.666。该结果包含 36 张训练图，只说明模型能够拟合现有数据，不能作为泛化性能。

`Roaddata` 共 8 张且没有真值标签，所以只做定性验证。在 `conf=0.40` 时产生 2 个 `pothole` 框，均位于排水板规则开槽，属于明显误报；其余 7 张无框。

## 当前限制

1. 类别极端不平衡，`crack` 仅 4 个框，`rutting` 为 0，当前模型实际只能视为校园场景 `pothole/路面破损` 检测器。
2. 训练集中每张图都有缺陷，没有正常路面、排水沟、砖缝、木板正常接缝等负样本，导致规则缝隙误报。
3. 现有 `pothole` 同时包含传统坑槽、木板翘起、石板隆起，建议业务显示为“路面破损/高差”，或下一轮拆成独立类别。
4. 44 张来自两段高度相似的连拍，独立验证集仍然很小；上线前需要采集不同路线、光照、距离和相机姿态。

## 复现命令

```powershell
python prepare_campus_dataset.py --force
python train_campus_detector.py --base yolo11n.pt --name standard_yolo11n --epochs 120 --imgsz 960 --batch 8 --device 0
python evaluate_campus_and_roaddata.py --model models/campus_road_best.pt --conf 0.40 --device 0 --output training_data/evaluation_conf40
```

最终可视化位于 `training_data/evaluation_conf40/campus_44_predictions` 和 `training_data/evaluation_conf40/roaddata_predictions`。
