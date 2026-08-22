#!/usr/bin/env python3
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    """启动完整系统的所有节点"""
    
    cri_bringup_dir = get_package_share_directory('cri_bringup')
    
    return LaunchDescription([
        # 驱动层
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(cri_bringup_dir, 'launch', 'drivers.launch.py')
            )
        ),
        
        # UE5 桥接
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(get_package_share_directory('ue5_bridge'), 'launch', 'ue5_bridge.launch.py')
            )
        ),
        
        # TODO: 感知层、算法层、导航层启动项
        # 由于这些节点尚未完整实现，暂时注释
    ])
