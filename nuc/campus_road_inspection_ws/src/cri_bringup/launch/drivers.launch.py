#!/usr/bin/env python3
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    """启动驱动层所有节点"""
    
    chassis_driver_dir = get_package_share_directory('chassis_driver')
    
    return LaunchDescription([
        # 底盘驱动
        Node(
            package='chassis_driver',
            executable='chassis_driver_node',
            name='chassis_driver',
            output='screen',
            parameters=[os.path.join(chassis_driver_dir, 'config', 'chassis_params.yaml')]
        ),
        
        # Livox 雷达驱动
        Node(
            package='livox_driver',
            executable='livox_driver_node',
            name='livox_driver',
            output='screen',
        ),
        
        # 海康相机驱动
        Node(
            package='hikrobot_camera',
            executable='hikrobot_camera_node',
            name='hikrobot_camera',
            output='screen',
        ),
    ])
