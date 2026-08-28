"""ParkToTag action server for structured simulated or real tag-pose inputs."""

import math
import time
from typing import Optional

import rclpy
from geometry_msgs.msg import Twist
from rclpy.action import ActionServer, CancelResponse, GoalResponse
from rclpy.executors import MultiThreadedExecutor
from rclpy.node import Node
from robot_interfaces.action import ParkToTag
from robot_interfaces.msg import TagPose

from .control_math import calculate_command, distance_to_tag_m, normalize_angle, pose_is_within_tolerance


STATE_WAITING_FOR_TAG = 1
STATE_ALIGNING = 2
STATE_APPROACHING = 3
STATE_PARKED = 4

FAILURE_NONE = 0
FAILURE_TAG_TIMEOUT = 1
FAILURE_TASK_TIMEOUT = 2
FAILURE_CANCELLED = 3


class ParkingController(Node):
    def __init__(self) -> None:
        super().__init__('parking_controller')
        self.declare_parameter('tag_pose_topic', '/sim/tag_pose')
        self.declare_parameter('supported_tag_id', 0)
        self.declare_parameter('tag_timeout_s', 0.5)
        self.declare_parameter('distance_tolerance_m', 0.04)
        self.declare_parameter('max_linear_mps', 0.18)
        self.declare_parameter('max_angular_rps', 0.70)
        self.declare_parameter('distance_gain', 0.60)
        self.declare_parameter('heading_gain', 1.40)
        self.declare_parameter('lateral_gain', 1.20)

        self._tag_timeout_s = float(self.get_parameter('tag_timeout_s').value)
        self._distance_tolerance_m = float(self.get_parameter('distance_tolerance_m').value)
        self._supported_tag_id = int(self.get_parameter('supported_tag_id').value)
        self._latest_tag: Optional[TagPose] = None
        self._latest_tag_received_s = 0.0

        topic_name = str(self.get_parameter('tag_pose_topic').value)
        self._tag_subscription = self.create_subscription(TagPose, topic_name, self._on_tag_pose, 10)
        self._command_publisher = self.create_publisher(Twist, '/cmd_vel', 10)
        self._action_server = ActionServer(
            self,
            ParkToTag,
            '/park_to_tag',
            execute_callback=self._execute,
            goal_callback=self._on_goal,
            cancel_callback=self._on_cancel,
        )
        self.get_logger().info(f'Parking controller ready; waiting for TagPose on {topic_name}.')

    def _on_tag_pose(self, message: TagPose) -> None:
        self._latest_tag = message
        self._latest_tag_received_s = time.monotonic()

    def _on_goal(self, goal: ParkToTag.Goal) -> GoalResponse:
        if goal.tag_id != self._supported_tag_id:
            self.get_logger().warning(f'Rejecting unsupported tag id {goal.tag_id}.')
            return GoalResponse.REJECT
        if goal.desired_distance_m < 0.10 or goal.timeout_s <= 0.0:
            self.get_logger().warning('Rejecting invalid parking goal.')
            return GoalResponse.REJECT
        return GoalResponse.ACCEPT

    def _on_cancel(self, _goal_handle) -> CancelResponse:
        return CancelResponse.ACCEPT

    def _execute(self, goal_handle):
        goal = goal_handle.request
        started_s = time.monotonic()
        feedback = ParkToTag.Feedback()
        result = ParkToTag.Result()
        failure_code = FAILURE_NONE
        succeeded = False

        try:
            while rclpy.ok():
                elapsed_s = time.monotonic() - started_s
                if goal_handle.is_cancel_requested:
                    failure_code = FAILURE_CANCELLED
                    goal_handle.canceled()
                    break
                if elapsed_s > goal.timeout_s:
                    failure_code = FAILURE_TASK_TIMEOUT
                    goal_handle.abort()
                    break
                tag_is_stale = (
                    self._latest_tag is None
                    or time.monotonic() - self._latest_tag_received_s > self._tag_timeout_s
                )
                if tag_is_stale:
                    feedback.state = STATE_WAITING_FOR_TAG
                    feedback.tag_visible = False
                    feedback.command_linear_mps = 0.0
                    feedback.command_angular_rps = 0.0
                    goal_handle.publish_feedback(feedback)
                    self._publish_stop()
                    if elapsed_s > self._tag_timeout_s:
                        failure_code = FAILURE_TAG_TIMEOUT
                        goal_handle.abort()
                        break
                    time.sleep(0.05)
                    continue

                tag = self._latest_tag
                if tag.tag_id != goal.tag_id:
                    time.sleep(0.05)
                    continue

                pose = tag.pose
                relative_yaw_rad = self._yaw_from_quaternion(pose.orientation)
                command = calculate_command(
                    pose.position.x,
                    pose.position.y,
                    relative_yaw_rad,
                    goal.desired_distance_m,
                    float(self.get_parameter('max_linear_mps').value),
                    float(self.get_parameter('max_angular_rps').value),
                    float(self.get_parameter('distance_gain').value),
                    float(self.get_parameter('heading_gain').value),
                    float(self.get_parameter('lateral_gain').value),
                )
                is_parked = pose_is_within_tolerance(
                    command,
                    self._distance_tolerance_m,
                    goal.lateral_tolerance_m,
                    goal.yaw_tolerance_rad,
                )
                feedback.state = STATE_PARKED if is_parked else (
                    STATE_ALIGNING if abs(command.heading_error_rad) > goal.yaw_tolerance_rad else STATE_APPROACHING
                )
                feedback.tag_visible = True
                feedback.relative_x_m = pose.position.x
                feedback.relative_y_m = pose.position.y
                feedback.relative_yaw_rad = relative_yaw_rad
                feedback.command_linear_mps = 0.0 if is_parked else command.linear_mps
                feedback.command_angular_rps = 0.0 if is_parked else command.angular_rps
                feedback.retry_count = 0
                goal_handle.publish_feedback(feedback)

                if is_parked:
                    self._publish_stop()
                    goal_handle.succeed()
                    succeeded = True
                    break

                self._publish_command(command.linear_mps, command.angular_rps)
                time.sleep(0.05)
        finally:
            self._publish_stop()

        tag = self._latest_tag
        result.success = succeeded
        result.failure_code = failure_code
        result.elapsed_time_s = float(time.monotonic() - started_s)
        if tag is not None:
            result.final_distance_m = distance_to_tag_m(tag.pose.position.x, tag.pose.position.y)
            result.final_lateral_error_m = tag.pose.position.y
            result.final_yaw_error_rad = normalize_angle(
                self._yaw_from_quaternion(tag.pose.orientation) - math.pi
            )
        return result

    def _publish_command(self, linear_mps: float, angular_rps: float) -> None:
        command = Twist()
        command.linear.x = linear_mps
        command.angular.z = angular_rps
        self._command_publisher.publish(command)

    def _publish_stop(self) -> None:
        self._command_publisher.publish(Twist())

    @staticmethod
    def _yaw_from_quaternion(quaternion) -> float:
        return math.atan2(
            2.0 * (quaternion.w * quaternion.z + quaternion.x * quaternion.y),
            1.0 - 2.0 * (quaternion.y * quaternion.y + quaternion.z * quaternion.z),
        )


def main() -> None:
    rclpy.init()
    node = ParkingController()
    executor = MultiThreadedExecutor(num_threads=2)
    executor.add_node(node)
    try:
        executor.spin()
    except KeyboardInterrupt:
        pass
    finally:
        executor.shutdown()
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
