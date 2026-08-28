from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    config_file = (
        get_package_share_directory('base_driver_mock') + '/config/mock_base.yaml'
    )
    return LaunchDescription([
        Node(
            package='base_driver_mock',
            executable='mock_base',
            name='base_driver_mock',
            output='screen',
            parameters=[config_file],
        ),
    ])
