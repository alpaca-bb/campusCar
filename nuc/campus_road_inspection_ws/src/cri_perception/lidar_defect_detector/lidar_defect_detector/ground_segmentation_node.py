#!/usr/bin/env python3
"""
地面分割节点 - 基于改进的Patchwork算法
订阅: /livox/pointcloud (sensor_msgs/PointCloud2)
发布: /perception/ground_cloud (sensor_msgs/PointCloud2) - 地面点
发布: /perception/nonground_cloud (sensor_msgs/PointCloud2) - 非地面点
"""
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2, PointField
import numpy as np
import struct
from std_msgs.msg import Header


class GroundSegmentationNode(Node):
    def __init__(self):
        super().__init__('ground_segmentation_node')
        
        # 参数声明
        self.declare_parameter('num_sectors', 64)
        self.declare_parameter('num_rings', 20)
        self.declare_parameter('max_range', 30.0)
        self.declare_parameter('min_range', 1.0)
        self.declare_parameter('sensor_height', 0.5)
        self.declare_parameter('ground_threshold', 0.15)
        self.declare_parameter('normal_angle_threshold', 10.0)
        self.declare_parameter('initial_seed_points', 200)
        
        # 获取参数
        self.num_sectors = self.get_parameter('num_sectors').value
        self.num_rings = self.get_parameter('num_rings').value
        self.max_range = self.get_parameter('max_range').value
        self.min_range = self.get_parameter('min_range').value
        self.sensor_height = self.get_parameter('sensor_height').value
        self.ground_threshold = self.get_parameter('ground_threshold').value
        self.normal_angle_threshold = np.deg2rad(self.get_parameter('normal_angle_threshold').value)
        self.initial_seed_points = self.get_parameter('initial_seed_points').value
        
        # 订阅与发布
        self.pc_sub = self.create_subscription(
            PointCloud2, '/livox/pointcloud', self.pointcloud_callback, 10)
        self.ground_pub = self.create_publisher(PointCloud2, '/perception/ground_cloud', 10)
        self.nonground_pub = self.create_publisher(PointCloud2, '/perception/nonground_cloud', 10)
        
        self.get_logger().info('地面分割节点已启动 (Patchwork算法)')
        self.get_logger().info(f'参数: sectors={self.num_sectors}, rings={self.num_rings}, '
                              f'threshold={self.ground_threshold}m')
    
    def pointcloud_callback(self, msg: PointCloud2):
        """点云回调，执行地面分割"""
        # 解析点云
        points = self.parse_pointcloud(msg)
        if points is None or len(points) < 100:
            return
        
        # 执行地面分割
        ground_mask = self.segment_ground(points)
        ground_points = points[ground_mask]
        nonground_points = points[~ground_mask]
        
        # 发布地面点云
        if len(ground_points) > 0:
            ground_msg = self.create_pointcloud_msg(ground_points, msg.header)
            self.ground_pub.publish(ground_msg)
        
        # 发布非地面点云
        if len(nonground_points) > 0:
            nonground_msg = self.create_pointcloud_msg(nonground_points, msg.header)
            self.nonground_pub.publish(nonground_msg)
        
        self.get_logger().debug(f'地面点: {len(ground_points)}, 非地面点: {len(nonground_points)}')
    
    def parse_pointcloud(self, msg: PointCloud2):
        """解析PointCloud2消息为numpy数组"""
        try:
            # 获取点云数据
            fmt = 'ffff'  # x, y, z, intensity
            point_step = 16
            num_points = len(msg.data) // point_step
            
            points = []
            for i in range(num_points):
                offset = i * point_step
                x, y, z, intensity = struct.unpack_from(fmt, msg.data, offset)
                points.append([x, y, z, intensity])
            
            return np.array(points, dtype=np.float32)
        except Exception as e:
            self.get_logger().error(f'解析点云失败: {e}')
            return None
    
    def segment_ground(self, points):
        """
        改进的Patchwork地面分割算法
        核心思想：基于扇区-环形网格的多步迭代地面拟合
        """
        n_points = len(points)
        ground_mask = np.zeros(n_points, dtype=bool)
        
        # 1. 极坐标转换
        xy = points[:, :2]
        ranges = np.linalg.norm(xy, axis=1)
        angles = np.arctan2(points[:, 1], points[:, 0])
        
        # 过滤范围外的点
        valid_mask = (ranges >= self.min_range) & (ranges <= self.max_range)
        if np.sum(valid_mask) < 100:
            return ground_mask
        
        # 2. 扇区-环形网格划分
        sector_step = 2 * np.pi / self.num_sectors
        ring_step = (self.max_range - self.min_range) / self.num_rings
        
        for sector in range(self.num_sectors):
            sector_start = -np.pi + sector * sector_step
            sector_end = sector_start + sector_step
            
            for ring in range(self.num_rings):
                ring_start = self.min_range + ring * ring_step
                ring_end = ring_start + ring_step
                
                # 提取当前patch内的点
                patch_mask = valid_mask & \
                             (angles >= sector_start) & (angles < sector_end) & \
                             (ranges >= ring_start) & (ranges < ring_end)
                
                patch_indices = np.where(patch_mask)[0]
                if len(patch_indices) < 3:
                    continue
                
                patch_points = points[patch_indices]
                
                # 3. 平面拟合（最小二乘法）
                ground_indices = self.fit_ground_plane(patch_points)
                if len(ground_indices) > 0:
                    ground_mask[patch_indices[ground_indices]] = True
        
        return ground_mask
    
    def fit_ground_plane(self, patch_points):
        """
        对patch内的点进行平面拟合
        返回属于地面的点索引
        """
        # 初始种子点选择：高度最低的N个点
        heights = patch_points[:, 2]
        sorted_indices = np.argsort(heights)
        seed_indices = sorted_indices[:min(self.initial_seed_points, len(patch_points))]
        
        # 迭代拟合
        for iteration in range(3):
            seed_points = patch_points[seed_indices]
            
            # 最小二乘平面拟合：ax + by + cz + d = 0
            centroid = np.mean(seed_points[:, :3], axis=0)
            centered = seed_points[:, :3] - centroid
            
            if len(centered) < 3:
                break
            
            # SVD分解求法向量
            try:
                _, _, vh = np.linalg.svd(centered)
                normal = vh[-1, :]  # 最小奇异值对应的向量
                
                # 确保法向量向上
                if normal[2] < 0:
                    normal = -normal
                
                # 检查法向量与竖直方向的夹角
                z_axis = np.array([0, 0, 1])
                angle = np.arccos(np.clip(np.dot(normal, z_axis), -1.0, 1.0))
                
                if angle > self.normal_angle_threshold:
                    # 法向量偏离竖直太多，不是地面
                    return []
                
                # 计算所有点到平面的距离
                d = -np.dot(normal, centroid)
                distances = np.abs(np.dot(patch_points[:, :3], normal) + d)
                
                # 更新种子点：距离小于阈值的点
                seed_indices = np.where(distances < self.ground_threshold)[0]
                
                if len(seed_indices) < 3:
                    return []
                
            except np.linalg.LinAlgError:
                return []
        
        return seed_indices
    
    def create_pointcloud_msg(self, points, header):
        """将numpy数组转换为PointCloud2消息"""
        msg = PointCloud2()
        msg.header = header
        msg.height = 1
        msg.width = len(points)
        
        msg.fields = [
            PointField(name='x', offset=0, datatype=PointField.FLOAT32, count=1),
            PointField(name='y', offset=4, datatype=PointField.FLOAT32, count=1),
            PointField(name='z', offset=8, datatype=PointField.FLOAT32, count=1),
            PointField(name='intensity', offset=12, datatype=PointField.FLOAT32, count=1),
        ]
        
        msg.is_bigendian = False
        msg.point_step = 16
        msg.row_step = msg.point_step * msg.width
        msg.is_dense = True
        
        # 打包数据
        data = []
        for point in points:
            data.extend(struct.pack('ffff', point[0], point[1], point[2], point[3]))
        msg.data = bytes(data)
        
        return msg


def main(args=None):
    rclpy.init(args=args)
    node = GroundSegmentationNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
