#!/usr/bin/env python3
"""启动奥比中光 RGBD 相机输入。"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    package_dir = get_package_share_directory('hikrobot_camera')
    config = os.path.join(package_dir, 'config', 'orbbec_params.yaml')
    return LaunchDescription([
        Node(
            package='hikrobot_camera',
            executable='orbbec_camera_node',
            name='orbbec_camera',
            output='screen',
            parameters=[config],
        ),
    ])
