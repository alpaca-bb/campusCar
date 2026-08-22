# CyberLuban 项目重构方案

## 📊 当前问题分析

### 1. 目录混乱
- ❌ 多个重复的项目版本（`campusCar`, `_ref_campusCar`, `campusCar.zip`）
- ❌ 多个 UE5 版本（`Windows`, `Windows73`，共占用 10.3GB）
- ❌ 测试数据集散落在多个位置（`新建文件夹`, `Roaddata`, `demo/`）
- ❌ 根目录有大量临时文件和测试脚本
- ❌ 文档分散在多个位置，有大量重复的总结文档

### 2. 文档问题
- 38 个 Markdown 文档，内容重复
- 多个 FINAL/SUMMARY 类型的文档
- 缺少统一的项目入口文档

### 3. 空间占用
- Windows73/: 9.6GB (UE5 项目)
- 新建文件夹/: 4.7GB (训练数据)
- RTK/: 1.4GB (RTK 工具)
- Windows/: 816MB (旧版 UE5)
- 总计：约 16GB

---

## 🎯 重构目标

### 清晰的三层架构

```
CyberLuban/                          # 项目根目录
├── README.md                         # 项目总入口文档
├── docs/                             # 📚 统一文档中心
├── robot/                            # 🤖 机器人端（Ubuntu NUC）
├── brain/                            # 🧠 校园大脑端（Windows UE5）
├── datasets/                         # 📦 数据集
├── tools/                            # 🔧 工具脚本
└── archive/                          # 📦 归档（旧版本、备份）
```

---

## 📋 详细重构计划

### 阶段 1：创建新结构（不删除旧文件）

#### 1.1 创建顶层目录结构
```bash
mkdir -p docs/{architecture,api,guides,development}
mkdir -p robot/{ros2_workspace,drivers,configs,scripts}
mkdir -p brain/{ue5_project,plugins,blueprints}
mkdir -p datasets/{training,testing,samples}
mkdir -p tools/{camera,network,debug}
mkdir -p archive/{old_versions,deprecated}
```

#### 1.2 文档整合（docs/）
```
docs/
├── README.md                         # 文档索引
├── architecture/                     # 系统架构
│   ├── system_overview.md           # 系统概述
│   ├── hardware.md                   # 硬件配置
│   └── communication.md              # 通信架构
├── api/                              # API 文档
│   ├── ros2_topics.md               # ROS2 话题定义
│   └── ue5_interfaces.md            # UE5 接口
├── guides/                           # 使用指南
│   ├── quickstart.md                # 快速开始
│   ├── robot_setup.md               # 机器人端设置
│   ├── brain_setup.md               # 校园大脑设置
│   ├── camera_setup.md              # 相机配置
│   └── troubleshooting.md           # 故障排查
└── development/                      # 开发文档
    ├── contributing.md              # 贡献指南
    └── changelog.md                 # 更新日志
```

#### 1.3 机器人端整合（robot/）
```
robot/
├── README.md                         # 机器人端说明
├── ros2_workspace/                   # ROS2 工作空间
│   └── (campus_road_inspection_ws 内容)
├── drivers/                          # 硬件驱动
│   ├── hikrobot_camera/             # 海康相机
│   ├── livox_lidar/                 # Livox 雷达
│   └── chassis/                     # 底盘控制
├── configs/                          # 配置文件
│   ├── camera_params.yaml
│   ├── lidar_params.yaml
│   └── chassis_params.yaml
└── scripts/                          # 启动脚本
    ├── start_all.sh
    ├── start_drivers.sh
    └── diagnostics.sh
```

#### 1.4 校园大脑整合（brain/）
```
brain/
├── README.md                         # 校园大脑说明
├── ue5_project/                      # UE5 主项目
│   └── (Windows73/CampusBrain 内容)
├── plugins/                          # UE5 插件
│   └── ROSIntegration/
└── configs/                          # 配置文件
    └── ros_connection.ini
```

#### 1.5 数据集整合（datasets/）
```
datasets/
├── README.md                         # 数据集说明
├── training/                         # 训练数据
│   └── (新建文件夹内容)
├── testing/                          # 测试数据
│   └── (Roaddata 内容)
└── samples/                          # 示例数据
    └── (demo/samples 内容)
```

#### 1.6 工具整合（tools/）
```
tools/
├── camera/                           # 相机工具
│   ├── enum_hikrobot_camera.py
│   ├── discover_camera_raw.py
│   └── force_camera_ip.py
├── network/                          # 网络工具
│   └── check_connection.py
└── debug/                            # 调试工具
    └── topic_monitor.py
```

### 阶段 2：迁移文件（保留原文件）

#### 2.1 迁移文档
- 合并所有 SUMMARY 文档 → `docs/development/changelog.md`
- UE5 相关文档 → `docs/guides/brain_setup.md`
- ROS2 相关文档 → `docs/guides/robot_setup.md`
- 架构图和设计 → `docs/architecture/`

#### 2.2 迁移代码
- `campus_road_inspection_ws` → `robot/ros2_workspace/`
- `campusCar`（如果还在用）→ `robot/legacy/` 或删除
- `Windows73/CampusBrain` → `brain/ue5_project/`

#### 2.3 迁移数据
- `新建文件夹/` → `datasets/training/`
- `Roaddata/` → `datasets/testing/`
- `campus_road_inspection_ws/demo/samples` → `datasets/samples/`

#### 2.4 迁移工具
- 所有 `*camera*.py` → `tools/camera/`
- RTK 相关工具 → `tools/rtk/`

### 阶段 3：创建统一文档

#### 3.1 根目录 README.md（新）
```markdown
# 🤖 CyberLuban - 校园智能巡检机器人系统

虚实联动的校园路面巡检机器人系统

## 快速开始
- [5分钟快速开始](docs/guides/quickstart.md)
- [机器人端设置](docs/guides/robot_setup.md)
- [校园大脑设置](docs/guides/brain_setup.md)

## 项目结构
- `robot/` - 机器人端（Ubuntu + ROS2）
- `brain/` - 校园大脑（Windows + UE5）
- `docs/` - 完整文档
- `datasets/` - 训练和测试数据
- `tools/` - 开发工具

## 文档索引
详见 [docs/README.md](docs/README.md)
```

### 阶段 4：清理归档

#### 4.1 归档旧版本
```bash
# 移动到 archive/
mv _ref_campusCar archive/old_versions/
mv Windows archive/old_versions/ue5_old/
mv *.zip archive/backups/
```

#### 4.2 删除可确认不需要的文件
```bash
# 临时测试脚本（确认后删除）
rm test_*.py
rm discover_*.py
rm gige_forceip.py

# 重复的文档（内容已整合）
rm CODE_STATUS.md
rm FUSION_README.md
rm INTEGRATION_DESIGN.md
# ... 其他重复文档
```

---

## ✅ 重构后的最终结构

```
CyberLuban/
├── README.md                         # 🏠 项目主页
├── .gitignore
├── LICENSE
│
├── docs/                             # 📚 文档中心
│   ├── README.md                     # 文档索引
│   ├── architecture/                 # 系统架构
│   ├── api/                          # API 参考
│   ├── guides/                       # 使用指南
│   └── development/                  # 开发文档
│
├── robot/                            # 🤖 机器人端
│   ├── README.md
│   ├── ros2_workspace/              # ROS2 工作空间
│   │   ├── src/
│   │   ├── install/
│   │   └── build/
│   ├── drivers/                      # 硬件驱动
│   ├── configs/                      # 配置文件
│   └── scripts/                      # 启动脚本
│
├── brain/                            # 🧠 校园大脑
│   ├── README.md
│   ├── ue5_project/                 # UE5 项目
│   │   ├── CampusBrain.uproject
│   │   ├── Content/
│   │   └── Plugins/
│   └── configs/                      # 配置文件
│
├── datasets/                         # 📦 数据集
│   ├── README.md
│   ├── training/                     # 训练数据 (4.7GB)
│   ├── testing/                      # 测试数据
│   └── samples/                      # 示例数据
│
├── tools/                            # 🔧 工具脚本
│   ├── camera/                       # 相机工具
│   ├── network/                      # 网络工具
│   ├── rtk/                          # RTK 工具
│   └── debug/                        # 调试工具
│
└── archive/                          # 📦 归档
    ├── old_versions/                 # 旧版本
    ├── deprecated/                   # 已废弃
    └── backups/                      # 备份文件
```

---

## 🚀 执行建议

### 优先级 1（立即执行）
1. ✅ 创建新的目录结构
2. ✅ 整合文档到 `docs/`
3. ✅ 创建统一的 README.md

### 优先级 2（本周完成）
4. ✅ 迁移代码到对应模块
5. ✅ 整理工具脚本到 `tools/`
6. ✅ 归档旧版本到 `archive/`

### 优先级 3（确认后执行）
7. ⚠️ 删除确认不需要的重复文件
8. ⚠️ 清理临时测试脚本

---

## ⚠️ 注意事项

1. **不要直接删除**，先移动到 `archive/`
2. **保留所有 .git 历史**
3. **重构前做完整备份**
4. **逐步执行，每步验证**
5. **更新所有脚本中的路径引用**

---

## 📝 待确认的决策

### Q1: campusCar 目录
- 选项 A: 完全废弃，移到 archive/
- 选项 B: 保留部分配置，整合到 robot/
- **建议**: 选项 A（已被 campus_road_inspection_ws 替代）

### Q2: Windows 目录（旧版 UE5）
- 选项 A: 删除（已有 Windows73）
- 选项 B: 移到 archive/
- **建议**: 选项 B（保留备份）

### Q3: RTK 目录
- 选项 A: 整合到 robot/drivers/rtk/
- 选项 B: 保持独立 tools/rtk/
- **建议**: 选项 B（RTK 是可选工具）

---

## 📊 预期效果

### 空间优化
- 归档压缩可节省约 5-8GB 空间
- 删除重复文件可节省约 1-2GB

### 可维护性
- ✅ 清晰的三层架构（robot/brain/docs）
- ✅ 文档集中管理
- ✅ 代码模块化
- ✅ 新人友好，5分钟了解项目

### 协作效率
- ✅ 减少 80% 的文档查找时间
- ✅ 统一的入口和规范
- ✅ 便于版本管理和持续集成
