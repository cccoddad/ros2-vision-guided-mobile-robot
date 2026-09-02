from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


def generate_launch_description():
    simulation_share = Path(get_package_share_directory('robot_simulation'))
    gazebo_launch = simulation_share / 'launch' / 'gazebo.launch.py'

    return LaunchDescription([
        IncludeLaunchDescription(PythonLaunchDescriptionSource(str(gazebo_launch))),
        Node(
            package='parking_controller',
            executable='parking_controller',
            name='parking_controller',
            output='screen',
        ),
    ])
