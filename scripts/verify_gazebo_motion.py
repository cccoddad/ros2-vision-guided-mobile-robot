#!/usr/bin/env python3
"""Verify that the Gazebo ROS bridge produces measurable simulated motion."""

import argparse
import math
import sys
import time
from dataclasses import dataclass
from typing import Optional

import rclpy
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
from rclpy.node import Node


@dataclass(frozen=True)
class Pose2D:
    x_m: float
    y_m: float
    yaw_rad: float


class GazeboMotionVerifier(Node):
    def __init__(self) -> None:
        super().__init__('gazebo_motion_verifier')
        self._publisher = self.create_publisher(Twist, '/cmd_vel', 10)
        self._subscription = self.create_subscription(Odometry, '/odom', self._on_odometry, 10)
        self.latest_pose: Optional[Pose2D] = None

    def _on_odometry(self, message: Odometry) -> None:
        orientation = message.pose.pose.orientation
        yaw_rad = math.atan2(
            2.0 * (orientation.w * orientation.z + orientation.x * orientation.y),
            1.0 - 2.0 * (orientation.y * orientation.y + orientation.z * orientation.z),
        )
        position = message.pose.pose.position
        self.latest_pose = Pose2D(position.x, position.y, yaw_rad)

    def publish_velocity(self, linear_mps: float, angular_rps: float) -> None:
        command = Twist()
        command.linear.x = linear_mps
        command.angular.z = angular_rps
        self._publisher.publish(command)


def wait_for_pose(node: GazeboMotionVerifier, timeout_s: float) -> Optional[Pose2D]:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline and rclpy.ok():
        rclpy.spin_once(node, timeout_sec=0.1)
        if node.latest_pose is not None:
            return node.latest_pose
    return None


def normalize_angle(angle_rad: float) -> float:
    return math.atan2(math.sin(angle_rad), math.cos(angle_rad))


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--duration-s', type=float, default=4.0)
    parser.add_argument('--linear-mps', type=float, default=0.20)
    parser.add_argument('--angular-rps', type=float, default=0.45)
    parser.add_argument('--minimum-distance-m', type=float, default=0.20)
    parser.add_argument('--minimum-yaw-rad', type=float, default=0.20)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.duration_s <= 0.0:
        raise ValueError('--duration-s must be positive')

    rclpy.init()
    node = GazeboMotionVerifier()
    try:
        print('Waiting for /odom from the Gazebo bridge...')
        start_pose = wait_for_pose(node, timeout_s=3.0)
        if start_pose is None:
            print('FAIL: no /odom received within 3 seconds.', file=sys.stderr)
            return 2

        print(
            f'Start pose: x={start_pose.x_m:.3f} m, y={start_pose.y_m:.3f} m, '
            f'yaw={start_pose.yaw_rad:.3f} rad'
        )
        deadline = time.monotonic() + arguments.duration_s
        while time.monotonic() < deadline and rclpy.ok():
            node.publish_velocity(arguments.linear_mps, arguments.angular_rps)
            rclpy.spin_once(node, timeout_sec=0.05)

        for _ in range(3):
            node.publish_velocity(0.0, 0.0)
            rclpy.spin_once(node, timeout_sec=0.05)

        end_pose = node.latest_pose
        if end_pose is None:
            print('FAIL: /odom stopped during the motion test.', file=sys.stderr)
            return 3

        distance_m = math.hypot(end_pose.x_m - start_pose.x_m, end_pose.y_m - start_pose.y_m)
        yaw_change_rad = abs(normalize_angle(end_pose.yaw_rad - start_pose.yaw_rad))
        print(
            f'End pose:   x={end_pose.x_m:.3f} m, y={end_pose.y_m:.3f} m, '
            f'yaw={end_pose.yaw_rad:.3f} rad'
        )
        print(f'Measured displacement: {distance_m:.3f} m; yaw change: {yaw_change_rad:.3f} rad')

        if distance_m < arguments.minimum_distance_m or yaw_change_rad < arguments.minimum_yaw_rad:
            print('FAIL: simulated motion was below the expected threshold.', file=sys.stderr)
            return 4

        print('PASS: Gazebo received /cmd_vel and published changing /odom.')
        return 0
    finally:
        node.publish_velocity(0.0, 0.0)
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    raise SystemExit(main())
