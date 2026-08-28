from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import ExecuteProcess, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


def generate_launch_description():
    package_share = Path(get_package_share_directory('robot_simulation'))
    bridge_launch = (
        Path(get_package_share_directory('ros_gz_bridge')) / 'launch' / 'ros_gz_bridge.launch.py'
    )
    world_file = str(package_share / 'worlds' / 'parking_world.sdf')
    bridge_config = str(package_share / 'config' / 'bridge.yaml')

    return LaunchDescription([
        ExecuteProcess(
            cmd=['gz', 'sim', '-r', '-v', '3', world_file],
            output='screen',
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(str(bridge_launch)),
            launch_arguments={
                'bridge_name': 'gazebo_ros_bridge',
                'config_file': bridge_config,
            }.items(),
        ),
        Node(
            package='robot_simulation',
            executable='sim_tag_pose.py',
            name='sim_tag_pose_publisher',
            output='screen',
        ),
    ])
