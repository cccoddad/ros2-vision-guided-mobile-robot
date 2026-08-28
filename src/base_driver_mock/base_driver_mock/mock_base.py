"""A deterministic differential-drive base for software-in-the-loop tests."""

import math
from typing import Optional

import rclpy
from geometry_msgs.msg import TransformStamped, Twist
from nav_msgs.msg import Odometry
from rclpy.node import Node
from robot_interfaces.msg import BaseStatus
from tf2_ros import TransformBroadcaster


COMMUNICATION_TIMEOUT = 1 << 0


class MockBase(Node):
    """Consumes cmd_vel and publishes odometry, TF, and base health state."""

    def __init__(self) -> None:
        super().__init__('base_driver_mock')

        self.declare_parameter('update_rate_hz', 50.0)
        self.declare_parameter('command_timeout_s', 0.3)
        self.declare_parameter('wheel_separation_m', 0.18)
        self.declare_parameter('max_linear_speed_mps', 0.20)
        self.declare_parameter('max_angular_speed_rps', 0.80)
        self.declare_parameter('max_linear_accel_mps2', 0.25)
        self.declare_parameter('max_angular_accel_rps2', 1.00)
        self.declare_parameter('battery_voltage', 12.0)
        self.declare_parameter('odom_frame', 'odom')
        self.declare_parameter('base_frame', 'base_link')

        self._update_rate_hz = self._parameter('update_rate_hz')
        self._command_timeout_s = self._parameter('command_timeout_s')
        self._wheel_separation_m = self._parameter('wheel_separation_m')
        self._max_linear_speed_mps = self._parameter('max_linear_speed_mps')
        self._max_angular_speed_rps = self._parameter('max_angular_speed_rps')
        self._max_linear_accel_mps2 = self._parameter('max_linear_accel_mps2')
        self._max_angular_accel_rps2 = self._parameter('max_angular_accel_rps2')
        self._battery_voltage = self._parameter('battery_voltage')
        self._odom_frame = self._parameter('odom_frame')
        self._base_frame = self._parameter('base_frame')

        if self._update_rate_hz <= 0.0 or self._wheel_separation_m <= 0.0:
            raise ValueError('update_rate_hz and wheel_separation_m must be positive')

        self._desired_linear_mps = 0.0
        self._desired_angular_rps = 0.0
        self._linear_mps = 0.0
        self._angular_rps = 0.0
        self._x_m = 0.0
        self._y_m = 0.0
        self._yaw_rad = 0.0
        self._status_sequence = 0
        self._last_command_time_ns: Optional[int] = None
        self._last_update_time_ns = self.get_clock().now().nanoseconds

        self._odom_publisher = self.create_publisher(Odometry, '/odom', 10)
        self._status_publisher = self.create_publisher(BaseStatus, '/base_status', 10)
        self._tf_broadcaster = TransformBroadcaster(self)
        self._cmd_subscription = self.create_subscription(Twist, '/cmd_vel', self._on_cmd_vel, 10)
        self._timer = self.create_timer(1.0 / self._update_rate_hz, self._on_timer)

        self.get_logger().info(
            'Mock base ready: publishing /odom and /base_status; '
            f'command timeout is {self._command_timeout_s:.2f} s.'
        )

    def _parameter(self, name: str):
        return self.get_parameter(name).value

    def _on_cmd_vel(self, message: Twist) -> None:
        self._desired_linear_mps = self._clamp(
            message.linear.x, -self._max_linear_speed_mps, self._max_linear_speed_mps
        )
        self._desired_angular_rps = self._clamp(
            message.angular.z, -self._max_angular_speed_rps, self._max_angular_speed_rps
        )
        self._last_command_time_ns = self.get_clock().now().nanoseconds

    def _on_timer(self) -> None:
        now = self.get_clock().now()
        now_ns = now.nanoseconds
        dt_s = max(0.0, (now_ns - self._last_update_time_ns) / 1_000_000_000.0)
        self._last_update_time_ns = now_ns

        command_age_s = self._command_age_s(now_ns)
        timed_out = command_age_s > self._command_timeout_s
        target_linear_mps = 0.0 if timed_out else self._desired_linear_mps
        target_angular_rps = 0.0 if timed_out else self._desired_angular_rps

        self._linear_mps = self._approach(
            self._linear_mps, target_linear_mps, self._max_linear_accel_mps2 * dt_s
        )
        self._angular_rps = self._approach(
            self._angular_rps, target_angular_rps, self._max_angular_accel_rps2 * dt_s
        )

        self._yaw_rad += self._angular_rps * dt_s
        self._x_m += self._linear_mps * math.cos(self._yaw_rad) * dt_s
        self._y_m += self._linear_mps * math.sin(self._yaw_rad) * dt_s

        self._publish_odometry(now)
        self._publish_status(now, command_age_s, timed_out)

    def _publish_odometry(self, now) -> None:
        half_track = self._wheel_separation_m / 2.0
        left_speed_mps = self._linear_mps - self._angular_rps * half_track
        right_speed_mps = self._linear_mps + self._angular_rps * half_track
        orientation_z = math.sin(self._yaw_rad / 2.0)
        orientation_w = math.cos(self._yaw_rad / 2.0)

        odom = Odometry()
        odom.header.stamp = now.to_msg()
        odom.header.frame_id = self._odom_frame
        odom.child_frame_id = self._base_frame
        odom.pose.pose.position.x = self._x_m
        odom.pose.pose.position.y = self._y_m
        odom.pose.pose.orientation.z = orientation_z
        odom.pose.pose.orientation.w = orientation_w
        odom.twist.twist.linear.x = self._linear_mps
        odom.twist.twist.angular.z = self._angular_rps
        self._odom_publisher.publish(odom)

        transform = TransformStamped()
        transform.header = odom.header
        transform.child_frame_id = self._base_frame
        transform.transform.translation.x = self._x_m
        transform.transform.translation.y = self._y_m
        transform.transform.rotation.z = orientation_z
        transform.transform.rotation.w = orientation_w
        self._tf_broadcaster.sendTransform(transform)

        self._left_speed_mps = left_speed_mps
        self._right_speed_mps = right_speed_mps

    def _publish_status(self, now, command_age_s: float, timed_out: bool) -> None:
        status = BaseStatus()
        status.header.stamp = now.to_msg()
        status.header.frame_id = self._base_frame
        status.left_wheel_speed_mps = self._left_speed_mps
        status.right_wheel_speed_mps = self._right_speed_mps
        status.battery_voltage = self._battery_voltage
        status.fault_flags = COMMUNICATION_TIMEOUT if timed_out else 0
        status.estop_active = False
        status.command_age_s = min(command_age_s, 1_000_000.0)
        status.sequence = self._status_sequence
        self._status_sequence += 1
        self._status_publisher.publish(status)

    def _command_age_s(self, now_ns: int) -> float:
        if self._last_command_time_ns is None:
            return float('inf')
        return max(0.0, (now_ns - self._last_command_time_ns) / 1_000_000_000.0)

    @staticmethod
    def _clamp(value: float, lower: float, upper: float) -> float:
        return max(lower, min(value, upper))

    @staticmethod
    def _approach(current: float, target: float, maximum_step: float) -> float:
        if current < target:
            return min(target, current + maximum_step)
        return max(target, current - maximum_step)


def main() -> None:
    rclpy.init()
    node = MockBase()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
