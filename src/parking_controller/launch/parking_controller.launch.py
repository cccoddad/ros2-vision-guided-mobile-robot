from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='parking_controller',
            executable='parking_controller',
            name='parking_controller',
            output='screen',
        ),
    ])
