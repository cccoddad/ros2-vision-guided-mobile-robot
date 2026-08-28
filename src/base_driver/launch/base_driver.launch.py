from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    config_path = Path(get_package_share_directory('base_driver')) / 'config' / 'base_driver.yaml'
    return LaunchDescription([
        Node(
            package='base_driver',
            executable='base_driver_node',
            name='base_driver',
            output='screen',
            parameters=[str(config_path)],
        ),
    ])
