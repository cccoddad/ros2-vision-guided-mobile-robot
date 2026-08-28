#!/usr/bin/env python3
"""Publish synthetic AprilTag ground truth relative to the simulated robot."""

import math

import rclpy
from nav_msgs.msg import Odometry
from rclpy.node import Node
from robot_interfaces.msg import TagPose


class SimTagPosePublisher(Node):
    def __init__(self) -> None:
        super().__init__('sim_tag_pose_publisher')
        self.declare_parameter('tag_id', 0)
        self.declare_parameter('tag_x_m', 1.5)
        self.declare_parameter('tag_y_m', 0.0)
        self.declare_parameter('tag_yaw_rad', math.pi)
        self._tag_id = int(self.get_parameter('tag_id').value)
        self._tag_x_m = float(self.get_parameter('tag_x_m').value)
        self._tag_y_m = float(self.get_parameter('tag_y_m').value)
        self._tag_yaw_rad = float(self.get_parameter('tag_yaw_rad').value)
        self._publisher = self.create_publisher(TagPose, '/sim/tag_pose', 10)
        self._subscription = self.create_subscription(Odometry, '/odom', self._on_odometry, 10)
        self.get_logger().info('Publishing synthetic TagPose on /sim/tag_pose from Gazebo odometry.')

    def _on_odometry(self, odometry: Odometry) -> None:
        position = odometry.pose.pose.position
        orientation = odometry.pose.pose.orientation
        robot_yaw = math.atan2(
            2.0 * (orientation.w * orientation.z + orientation.x * orientation.y),
            1.0 - 2.0 * (orientation.y * orientation.y + orientation.z * orientation.z),
        )
        dx = self._tag_x_m - position.x
        dy = self._tag_y_m - position.y
        tag = TagPose()
        tag.header = odometry.header
        tag.header.frame_id = 'base_link'
        tag.tag_id = self._tag_id
        tag.pose.position.x = math.cos(robot_yaw) * dx + math.sin(robot_yaw) * dy
        tag.pose.position.y = -math.sin(robot_yaw) * dx + math.cos(robot_yaw) * dy
        relative_yaw = math.atan2(
            math.sin(self._tag_yaw_rad - robot_yaw),
            math.cos(self._tag_yaw_rad - robot_yaw),
        )
        tag.pose.orientation.z = math.sin(relative_yaw / 2.0)
        tag.pose.orientation.w = math.cos(relative_yaw / 2.0)
        self._publisher.publish(tag)


def main() -> None:
    rclpy.init()
    node = SimTagPosePublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
