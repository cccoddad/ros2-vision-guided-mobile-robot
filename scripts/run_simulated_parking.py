#!/usr/bin/env python3
"""Send a ParkToTag goal and report its feedback and result."""

import sys

import rclpy
from rclpy.action import ActionClient
from rclpy.node import Node
from robot_interfaces.action import ParkToTag


STATE_NAMES = {1: 'waiting_for_tag', 2: 'aligning', 3: 'approaching', 4: 'parked'}


class ParkingActionClient(Node):
    def __init__(self) -> None:
        super().__init__('simulated_parking_client')
        self._client = ActionClient(self, ParkToTag, '/park_to_tag')
        self._last_state = None

    def feedback_callback(self, feedback_message) -> None:
        feedback = feedback_message.feedback
        state = STATE_NAMES.get(feedback.state, f'unknown_{feedback.state}')
        if state != self._last_state:
            print(
                f'Feedback: state={state}, tag_visible={feedback.tag_visible}, '
                f'x={feedback.relative_x_m:.3f} m, y={feedback.relative_y_m:.3f} m, '
                f'command=({feedback.command_linear_mps:.3f} m/s, '
                f'{feedback.command_angular_rps:.3f} rad/s)'
            )
            self._last_state = state

    def run(self) -> int:
        if not self._client.wait_for_server(timeout_sec=5.0):
            print('FAIL: /park_to_tag action server was not available within 5 seconds.', file=sys.stderr)
            return 2

        goal = ParkToTag.Goal()
        goal.tag_id = 0
        goal.desired_distance_m = 0.35
        goal.lateral_tolerance_m = 0.06
        goal.yaw_tolerance_rad = 0.10
        goal.timeout_s = 30.0
        print('Sending goal: park 0.35 m in front of simulated AprilTag 0.')
        goal_future = self._client.send_goal_async(goal, feedback_callback=self.feedback_callback)
        rclpy.spin_until_future_complete(self, goal_future)
        goal_handle = goal_future.result()
        if goal_handle is None or not goal_handle.accepted:
            print('FAIL: parking goal was rejected.', file=sys.stderr)
            return 3

        result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(self, result_future)
        action_result = result_future.result()
        if action_result is None:
            print('FAIL: action result was unavailable.', file=sys.stderr)
            return 4

        result = action_result.result
        print(
            f'Result: success={result.success}, failure_code={result.failure_code}, '
            f'distance={result.final_distance_m:.3f} m, '
            f'lateral_error={result.final_lateral_error_m:.3f} m, '
            f'yaw_error={result.final_yaw_error_rad:.3f} rad, '
            f'elapsed={result.elapsed_time_s:.2f} s'
        )
        if not result.success:
            print('FAIL: controller did not complete the parking goal.', file=sys.stderr)
            return 5
        print('PASS: simulated ParkToTag action completed within the requested tolerances.')
        return 0


def main() -> None:
    rclpy.init()
    node = ParkingActionClient()
    try:
        exit_code = node.run()
    finally:
        node.destroy_node()
        rclpy.shutdown()
    raise SystemExit(exit_code)


if __name__ == '__main__':
    main()
