#!/usr/bin/env python3
"""
UE5 虚实双向桥接节点
- 虚拟端 → 实车: 接收 UE5 控制指令，转发到 /cmd_vel
- 实车 → 虚拟端: 聚合实车状态并发布到 /ue5/robot_state
"""
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist, PoseStamped
from sensor_msgs.msg import PointCloud2
from nav_msgs.msg import Odometry
from cri_msgs.msg import DefectArray, UE5Command, UE5State


class UE5BridgeNode(Node):
    def __init__(self):
        super().__init__('ue5_bridge_node')
        
        self.declare_parameter('bridge_mode', 'bidirectional')
        
        # 订阅: UE5 -> 实车
        self.ue5_cmd_sub = self.create_subscription(
            UE5Command, '/ue5/command', self.ue5_command_callback, 10)
        
        # 发布: UE5 控制指令转发
        self.cmd_vel_pub = self.create_publisher(Twist, '/cmd_vel', 10)
        
        # 订阅: 实车状态
        self.odom_sub = self.create_subscription(Odometry, '/odom', self.odom_callback, 10)
        self.defect_sub = self.create_subscription(DefectArray, '/perception/fused_defects', self.defect_callback, 10)
        self.pc_sub = self.create_subscription(PointCloud2, '/livox/pointcloud', self.pc_callback, 10)
        
        # 发布: 实车 -> UE5
        self.ue5_state_pub = self.create_publisher(UE5State, '/ue5/robot_state', 10)
        
        # 状态缓存
        self.robot_pose = PoseStamped()
        self.recent_defects = DefectArray()
        self.pointcloud = PointCloud2()
        
        # 定时发布聚合状态
        self.timer = self.create_timer(0.1, self.publish_ue5_state)  # 10Hz
        
        self.get_logger().info('UE5 虚实双向桥接节点已启动')
    
    def ue5_command_callback(self, msg: UE5Command):
        """接收 UE5 控制指令并转发"""
        if msg.control_mode == 'manual':
            self.cmd_vel_pub.publish(msg.velocity_command)
        elif msg.control_mode == 'emergency_stop':
            stop_cmd = Twist()
            self.cmd_vel_pub.publish(stop_cmd)
    
    def odom_callback(self, msg: Odometry):
        self.robot_pose.header = msg.header
        self.robot_pose.pose = msg.pose.pose
    
    def defect_callback(self, msg: DefectArray):
        self.recent_defects = msg
    
    def pc_callback(self, msg: PointCloud2):
        self.pointcloud = msg
    
    def publish_ue5_state(self):
        """聚合并发布实车状态到 UE5"""
        msg = UE5State()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.robot_pose = self.robot_pose
        msg.system_status = 'inspecting'
        msg.battery_level = 85.0
        msg.recent_defects = self.recent_defects
        msg.lidar_pointcloud = self.pointcloud
        self.ue5_state_pub.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    node = UE5BridgeNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
