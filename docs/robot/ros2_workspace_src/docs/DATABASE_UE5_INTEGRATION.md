# 缺陷数据库与UE5地图集成方案

## 🎯 系统架构

```
检测识别 → 分类标注 → GPS定位 → 数据库存储 → UE5地图可视化
   ↓          ↓          ↓          ↓            ↓
 YOLOv8    类别+置信度  RTK坐标   PostgreSQL    虚幻引擎
```

---

## 📊 完整数据流

### 1. 检测与分类
```python
# 输入: 图像/点云
# 输出: 检测结果
{
    "defect_id": "D001_20260702_001",
    "type": "crack",              # 裂缝
    "sub_type": "D00",            # 横向裂缝
    "confidence": 0.87,
    "dimensions": {
        "length": 0.5,  # 米
        "width": 0.1,
        "depth": 0.02
    },
    "severity": "medium"          # low/medium/high/critical
}
```

### 2. GPS定位打包
```python
# 输入: 检测结果 + RTK定位
# 输出: 定位包
{
    "defect_id": "D001_20260702_001",
    "location": {
        "gps": {
            "latitude": 31.026470,    # 纬度
            "longitude": 121.434550,  # 经度
            "altitude": 5.23          # 海拔
        },
        "utm": {                      # UTM投影坐标
            "easting": 326543.21,
            "northing": 3436789.45,
            "zone": "51N"
        },
        "local": {                    # 校园局部坐标
            "x": 123.45,              # 东
            "y": 456.78,              # 北
            "z": 5.23                 # 高
        }
    },
    "pose": {
        "roll": 0.01,
        "pitch": 0.02,
        "yaw": 1.57
    },
    "timestamp": "2026-07-02T10:30:45.123Z"
}
```

### 3. 数据库结构
```sql
-- 缺陷主表
CREATE TABLE defects (
    id SERIAL PRIMARY KEY,
    defect_id VARCHAR(50) UNIQUE NOT NULL,
    
    -- 分类信息
    type VARCHAR(20) NOT NULL,           -- crack/pothole/depression等
    sub_type VARCHAR(10),                -- D00/D10/D20等RDD分类
    confidence FLOAT,
    severity VARCHAR(20),
    
    -- 尺寸信息
    length FLOAT,
    width FLOAT,
    depth FLOAT,
    area FLOAT,
    volume FLOAT,
    
    -- GPS定位
    latitude DOUBLE PRECISION NOT NULL,
    longitude DOUBLE PRECISION NOT NULL,
    altitude FLOAT,
    
    -- UTM坐标
    utm_easting DOUBLE PRECISION,
    utm_northing DOUBLE PRECISION,
    utm_zone VARCHAR(10),
    
    -- 校园局部坐标
    local_x FLOAT,
    local_y FLOAT,
    local_z FLOAT,
    
    -- 姿态
    roll FLOAT,
    pitch FLOAT,
    yaw FLOAT,
    
    -- 元数据
    detected_time TIMESTAMP NOT NULL,
    robot_id VARCHAR(50),
    image_path TEXT,
    pointcloud_path TEXT,
    
    -- 状态
    status VARCHAR(20) DEFAULT 'detected',  -- detected/verified/repaired
    priority INTEGER DEFAULT 0,
    
    -- PostGIS几何字段（用于空间查询）
    geom GEOMETRY(Point, 4326),
    
    -- 索引
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 创建空间索引
CREATE INDEX idx_defects_geom ON defects USING GIST (geom);
CREATE INDEX idx_defects_type ON defects (type);
CREATE INDEX idx_defects_severity ON defects (severity);

-- 缺陷演化历史表
CREATE TABLE defect_evolution (
    id SERIAL PRIMARY KEY,
    defect_id VARCHAR(50) REFERENCES defects(defect_id),
    
    -- 演化预测
    prediction_date DATE NOT NULL,
    predicted_length FLOAT,
    predicted_width FLOAT,
    predicted_depth FLOAT,
    predicted_risk VARCHAR(20),
    
    -- 实际观测（如果有）
    actual_length FLOAT,
    actual_width FLOAT,
    actual_depth FLOAT,
    actual_risk VARCHAR(20),
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 养护记录表
CREATE TABLE maintenance_records (
    id SERIAL PRIMARY KEY,
    defect_id VARCHAR(50) REFERENCES defects(defect_id),
    
    maintenance_type VARCHAR(50),    -- repair/monitor/urgent
    scheduled_date DATE,
    completed_date DATE,
    cost DECIMAL(10, 2),
    notes TEXT,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

---

## 🗺️ UE5地图集成方案

### 方案1: 实时WebSocket通信（推荐）

#### 架构
```
ROS2节点 → rosbridge → WebSocket → UE5蓝图
   ↓
PostgreSQL数据库
```

#### UE5蓝图逻辑
```
1. 连接WebSocket服务器
   - 地址: ws://localhost:9090
   - 订阅话题: /roadmap/defects

2. 接收缺陷数据
   - 解析JSON消息
   - 提取GPS坐标

3. 坐标转换
   GPS(lat,lon) → UE5世界坐标(X,Y,Z)
   - 使用校园原点作为参考
   - 转换公式见下方

4. 生成缺陷标记
   - 在UE5地图上实例化Actor
   - 根据类型选择颜色/图标
   - 根据严重程度调整大小

5. 交互功能
   - 点击标记显示详细信息
   - 查询周边缺陷
   - 显示演化预测曲线
```

#### 坐标转换（GPS → UE5）
```cpp
// UE5 C++代码
FVector GPSToUE5Coordinates(double Latitude, double Longitude, double Altitude)
{
    // 校园原点GPS坐标（需要实际测量）
    const double OriginLat = 31.026000;  // 示例
    const double OriginLon = 121.434000;
    const double OriginAlt = 5.0;
    
    // 转换为局部米制坐标
    // 纬度1度 ≈ 111km，经度1度 ≈ 111km * cos(lat)
    double MetersPerDegreeLat = 111320.0;
    double MetersPerDegreeLon = 111320.0 * FMath::Cos(FMath::DegreesToRadians(OriginLat));
    
    double LocalX = (Longitude - OriginLon) * MetersPerDegreeLon;
    double LocalY = (Latitude - OriginLat) * MetersPerDegreeLat;
    double LocalZ = Altitude - OriginAlt;
    
    // 转换为UE5坐标（厘米，Z轴向上）
    // UE5使用厘米单位，需要 * 100
    FVector UE5Coord;
    UE5Coord.X = LocalX * 100.0f;  // 东 → X
    UE5Coord.Y = LocalY * 100.0f;  // 北 → Y  
    UE5Coord.Z = LocalZ * 100.0f;  // 高 → Z
    
    return UE5Coord;
}
```

#### UE5蓝图示例（文字描述）
```
【Event BeginPlay】
    ↓
【Connect to WebSocket】
    URL: ws://localhost:9090
    ↓
【Subscribe to Topic】
    Topic: /roadmap/defects
    ↓
【On Message Received】
    ↓
    Parse JSON → Get GPS (lat, lon, alt)
    ↓
    Convert GPS to UE5 Coords
    ↓
    【Spawn Defect Marker Actor】
        Location: UE5 Coords
        Type: Defect Type (crack/pothole)
        Severity: Color Mapping
            - Low: Green
            - Medium: Yellow  
            - High: Orange
            - Critical: Red
    ↓
【On Marker Clicked】
    ↓
    Show Widget with Details:
        - Defect ID
        - Type & Severity
        - Dimensions
        - Detection Time
        - Predicted Evolution
        - [View Image] Button
        - [Schedule Repair] Button
```

---

### 方案2: 数据库直接查询

#### 架构
```
UE5 HTTP插件 → REST API服务器 → PostgreSQL
```

#### REST API示例
```python
# FastAPI服务器
from fastapi import FastAPI
from typing import List
import asyncpg

app = FastAPI()

@app.get("/api/defects/")
async def get_defects(
    min_lat: float = None,
    max_lat: float = None,
    min_lon: float = None,
    max_lon: float = None,
    type: str = None,
    severity: str = None
):
    """查询指定区域的缺陷"""
    conn = await asyncpg.connect('postgresql://user:pass@localhost/defects_db')
    
    query = """
        SELECT defect_id, type, sub_type, severity,
               latitude, longitude, altitude,
               length, width, depth,
               detected_time
        FROM defects
        WHERE 1=1
    """
    
    params = []
    if min_lat and max_lat:
        query += " AND latitude BETWEEN $1 AND $2"
        params.extend([min_lat, max_lat])
    
    # ... 其他过滤条件
    
    rows = await conn.fetch(query, *params)
    await conn.close()
    
    return [dict(row) for row in rows]

@app.get("/api/defects/{defect_id}/evolution")
async def get_defect_evolution(defect_id: str):
    """获取缺陷演化预测"""
    # 查询world_model预测结果
    pass
```

---

## 🎨 UE5可视化效果

### 缺陷标记样式
```
【裂缝 Crack】
- 图标: 闪电符号
- 颜色: 
    Low: 浅绿色
    Medium: 黄色
    High: 橙色
    Critical: 红色
- 大小: 根据长度缩放

【坑槽 Pothole】
- 图标: 圆形凹陷
- 颜色: 同上
- 大小: 根据面积缩放

【沉降 Depression】
- 图标: 下箭头
- 颜色: 蓝色系
- 大小: 根据深度缩放
```

### 交互功能
```
1. 鼠标悬停 → 显示简要信息
2. 点击标记 → 弹出详细面板
3. 双击标记 → 摄像机飞向该位置
4. 右键菜单:
    - 查看详细信息
    - 查看演化预测
    - 安排养护
    - 导出数据
```

### 热力图显示
```
- 按密度显示缺陷分布
- 按严重程度显示风险区域
- 按时间显示新增缺陷
- 按类型显示缺陷分布
```

---

## 📦 完整数据包格式

### ROS2消息定义
```python
# cri_msgs/msg/DefectWithLocation.msg
std_msgs/Header header

# 缺陷信息
string defect_id
string type
string sub_type
float32 confidence
string severity

# 尺寸
float32 length
float32 width  
float32 depth
float32 area
float32 volume

# GPS定位
float64 latitude
float64 longitude
float32 altitude

# UTM坐标
float64 utm_easting
float64 utm_northing
string utm_zone

# 校园局部坐标
float32 local_x
float32 local_y
float32 local_z

# 姿态
float32 roll
float32 pitch
float32 yaw

# 元数据
string robot_id
string image_path
string pointcloud_path

# 演化预测
DefectPrediction[] evolution_predictions
```

---

## 🔄 完整工作流程

```
1. 机器人巡检
   ↓
2. 检测到缺陷 → YOLOv8识别
   ↓
3. 分类标注 → 类型 + 严重程度
   ↓
4. RTK定位 → 获取精确GPS坐标
   ↓
5. 数据打包 → DefectWithLocation消息
   ↓
6. 发布ROS2话题 → /roadmap/defects
   ↓
7. 数据库存储 → PostgreSQL + PostGIS
   ↓
8. rosbridge转发 → WebSocket
   ↓
9. UE5接收 → 解析JSON
   ↓
10. 坐标转换 → GPS → UE5世界坐标
   ↓
11. 实例化标记 → 在地图上显示
   ↓
12. 用户交互 → 点击查看详情
```

---

## 🚀 实施步骤

### 阶段1: 数据库搭建（1-2天）
1. 安装PostgreSQL + PostGIS
2. 创建数据库表结构
3. 编写数据插入接口

### 阶段2: ROS2集成（1天）
1. 创建DefectWithLocation消息
2. 修改动态地图节点，发布完整数据包
3. 测试消息发布

### 阶段3: UE5基础显示（2-3天）
1. 实现GPS到UE5坐标转换
2. 创建缺陷标记Actor蓝图
3. 实现WebSocket接收
4. 实例化标记显示

### 阶段4: 交互功能（2天）
1. 实现点击交互
2. 显示详细信息面板
3. 查询功能
4. 热力图显示

### 阶段5: 优化与测试（1天）
1. 性能优化
2. 批量加载
3. 完整测试

---

## 💡 技术要点

### GPS定位精度
- RTK精度: ±2cm
- 足够在UE5地图上精确定位
- 需要校准校园原点坐标

### 坐标系统
- WGS84 (GPS) → UTM → 局部ENU → UE5世界坐标
- 统一使用右手坐标系
- 注意单位转换（米 → 厘米）

### 数据同步
- 实时: WebSocket推送新缺陷
- 批量: REST API查询历史数据
- 缓存: UE5本地缓存已加载数据

### 性能优化
- LOD: 远距离简化标记显示
- 剔除: 视锥外的标记不渲染
- 分块加载: 按区域动态加载
- 实例化: 使用Instance Static Mesh

---

这个方案完整覆盖了从检测到可视化的全链路，可以根据实际情况调整！
