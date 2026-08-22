#!/usr/bin/env python3
"""
海康 MV-CS016-10GC GigE 工业相机驱动节点
- 对接海康 SDK，输出 RGB 图像与相机内参
- 发布: /camera/image_raw (sensor_msgs/Image), /camera/camera_info (sensor_msgs/CameraInfo)
"""
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image, CameraInfo
from std_msgs.msg import Header


class HikrobotCameraNode(Node):
    def __init__(self):
        super().__init__('hikrobot_camera_node')
        
        self.declare_parameter('camera_ip', '192.168.1.101')
        self.declare_parameter('frame_id', 'camera_frame')
        self.declare_parameter('fps', 10.0)
        
        self.camera_ip = self.get_parameter('camera_ip').value
        self.frame_id = self.get_parameter('frame_id').value
        self.fps = self.get_parameter('fps').value
        
        self.image_pub = self.create_publisher(Image, '/camera/image_raw', 10)
        self.info_pub = self.create_publisher(CameraInfo, '/camera/camera_info', 10)
        
        self.timer = self.create_timer(1.0 / self.fps, self.publish_image)
        
        self.get_logger().info(f'海康相机驱动节点已启动，相机IP: {self.camera_ip}')
        self.get_logger().warn('当前为占位模式，需要对接海康 SDK')
    
    def publish_image(self):
        """发布图像（占位）"""
        img_msg = Image()
        img_msg.header.stamp = self.get_clock().now().to_msg()
        img_msg.header.frame_id = self.frame_id
        img_msg.height = 1080
        img_msg.width = 1920
        img_msg.encoding = 'bgr8'
        img_msg.step = img_msg.width * 3
        img_msg.data = b'\x00' * (img_msg.height * img_msg.step)
        self.image_pub.publish(img_msg)
        
        info_msg = CameraInfo()
        info_msg.header = img_msg.header
        self.info_pub.publish(info_msg)


def main(args=None):
    rclpy.init(args=args)
    node = HikrobotCameraNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
