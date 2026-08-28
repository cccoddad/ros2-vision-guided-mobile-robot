#!/usr/bin/env python3
"""Verify that a stale simulated TagPose aborts parking and stops the robot."""

import sys
import time

import rclpy
from nav_msgs.msg import Odometry
from rclpy.action import ActionClient
from rclpy.node import Node
from rclpy.parameter import Parameter
from rclpy.parameter_client import AsyncParameterClient
from robot_interfaces.action import ParkToTag


EXPECTED_TAG_TIMEOUT = 1


class TagLossVerifier(Node):
    def __init__(self) -> None:
        super().__init__('tag_loss_verifier')
        self._action_client = ActionClient(self, ParkToTag, '/park_to_tag')
        self._tag_parameter_client = AsyncParameterClient(self, 'sim_tag_pose_publisher')
        self._latest_linear_mps = None
        self._latest_angular_rps = None
        self._subscription = self.create_subscription(Odometry, '/odom', self._on_odometry, 10)

    def _on_odometry(self, message: Odometry) -> None:
        self._latest_linear_mps = message.twist.twist.linear.x
        self._latest_angular_rps = message.twist.twist.angular.z

    def set_tag_enabled(self, enabled: bool) -> bool:
        future = self._tag_parameter_client.set_parameters([
            Parameter('enabled', value=enabled),
        ])
        rclpy.spin_until_future_complete(self, future, timeout_sec=3.0)
        response = future.result()
        return response is not None and all(item.successful for item in response.values)

    def run(self) -> int:
        if not self._action_client.wait_for_server(timeout_sec=5.0):
            print('FAIL: /park_to_tag action server was unavailable.', file=sys.stderr)
            return 2
        if not self._tag_parameter_client.wait_for_service(timeout_sec=5.0):
            print('FAIL: simulated TagPose parameter service was unavailable.', file=sys.stderr)
            return 3
        if not self.set_tag_enabled(True):
            print('FAIL: could not enable simulated TagPose.', file=sys.stderr)
            return 4

        goal = ParkToTag.Goal()
        goal.tag_id = 0
        # Use a closer target than the normal demo so the goal remains active
        # when this test follows a successful 0.35 m parking run.
        goal.desired_distance_m = 0.15
        goal.lateral_tolerance_m = 0.06
        goal.yaw_tolerance_rad = 0.10
        goal.timeout_s = 10.0
        print('Sending parking goal; simulated TagPose will be disabled after 1 second.')
        goal_future = self._action_client.send_goal_async(goal)
        rclpy.spin_until_future_complete(self, goal_future)
        goal_handle = goal_future.result()
        if goal_handle is None or not goal_handle.accepted:
            print('FAIL: parking goal was rejected.', file=sys.stderr)
            return 5

        result_future = goal_handle.get_result_async()
        disable_at_s = time.monotonic() + 1.0
        while not result_future.done() and rclpy.ok():
            rclpy.spin_once(self, timeout_sec=0.05)
            if disable_at_s is not None and time.monotonic() >= disable_at_s:
                if not self.set_tag_enabled(False):
                    print('FAIL: could not disable simulated TagPose.', file=sys.stderr)
                    return 6
                print('Simulated TagPose disabled; waiting for controller timeout and stop.')
                disable_at_s = None

        action_result = result_future.result()
        if action_result is None:
            print('FAIL: action result was unavailable.', file=sys.stderr)
            return 7
        result = action_result.result

        stop_deadline_s = time.monotonic() + 1.0
        while time.monotonic() < stop_deadline_s and rclpy.ok():
            rclpy.spin_once(self, timeout_sec=0.05)

        linear_mps = self._latest_linear_mps
        angular_rps = self._latest_angular_rps
        print(
            f'Result: success={result.success}, failure_code={result.failure_code}, '
            f'linear_speed={linear_mps:.4f} m/s, angular_speed={angular_rps:.4f} rad/s'
        )
        if result.success or result.failure_code != EXPECTED_TAG_TIMEOUT:
            print('FAIL: expected a TagPose-timeout result with failure_code=1.', file=sys.stderr)
            return 8
        if linear_mps is None or angular_rps is None or abs(linear_mps) > 0.01 or abs(angular_rps) > 0.01:
            print('FAIL: Gazebo odometry did not settle to zero speed.', file=sys.stderr)
            return 9
        print('PASS: Tag loss aborted parking and Gazebo odometry settled at zero speed.')
        return 0


def main() -> None:
    rclpy.init()
    node = TagLossVerifier()
    try:
        exit_code = node.run()
    finally:
        if node._tag_parameter_client.service_is_ready():
            node.set_tag_enabled(True)
        node.destroy_node()
        rclpy.shutdown()
    raise SystemExit(exit_code)


if __name__ == '__main__':
    main()
