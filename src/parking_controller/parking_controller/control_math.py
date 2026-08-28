"""Pure control functions used by the action server and unit tests."""

from dataclasses import dataclass
from math import atan2, cos, hypot, pi, sin


@dataclass(frozen=True)
class ParkingCommand:
    linear_mps: float
    angular_rps: float
    distance_error_m: float
    lateral_error_m: float
    heading_error_rad: float


def clamp(value: float, lower: float, upper: float) -> float:
    return max(lower, min(value, upper))


def normalize_angle(angle_rad: float) -> float:
    return atan2(sin(angle_rad), cos(angle_rad))


def calculate_command(
    relative_x_m: float,
    relative_y_m: float,
    relative_tag_yaw_rad: float,
    desired_distance_m: float,
    max_linear_mps: float,
    max_angular_rps: float,
    distance_gain: float,
    heading_gain: float,
    lateral_gain: float,
) -> ParkingCommand:
    """Calculate bounded commands from a TagPose expressed in base_link."""
    distance_error_m = relative_x_m - desired_distance_m
    lateral_error_m = relative_y_m
    # A tag facing the vehicle has pi relative yaw when the robot is aligned.
    heading_error_rad = normalize_angle(relative_tag_yaw_rad - pi)
    target_heading_rad = atan2(lateral_error_m, max(relative_x_m, 0.05))
    linear_mps = clamp(distance_gain * distance_error_m, 0.0, max_linear_mps)
    angular_rps = clamp(
        heading_gain * heading_error_rad + lateral_gain * target_heading_rad,
        -max_angular_rps,
        max_angular_rps,
    )
    return ParkingCommand(
        linear_mps=linear_mps,
        angular_rps=angular_rps,
        distance_error_m=distance_error_m,
        lateral_error_m=lateral_error_m,
        heading_error_rad=heading_error_rad,
    )


def pose_is_within_tolerance(
    command: ParkingCommand,
    distance_tolerance_m: float,
    lateral_tolerance_m: float,
    yaw_tolerance_rad: float,
) -> bool:
    return (
        abs(command.distance_error_m) <= distance_tolerance_m
        and abs(command.lateral_error_m) <= lateral_tolerance_m
        and abs(command.heading_error_rad) <= yaw_tolerance_rad
    )


def distance_to_tag_m(relative_x_m: float, relative_y_m: float) -> float:
    return hypot(relative_x_m, relative_y_m)
