import math
import unittest

from parking_controller.control_math import calculate_command, pose_is_within_tolerance


class ControlMathTests(unittest.TestCase):
    def command(self, x, y, yaw):
        return calculate_command(x, y, yaw, 0.35, 0.18, 0.70, 0.60, 1.40, 1.20)

    def test_aligned_tag_drives_forward(self):
        command = self.command(1.20, 0.0, math.pi)
        self.assertGreater(command.linear_mps, 0.0)
        self.assertAlmostEqual(command.angular_rps, 0.0, places=6)

    def test_lateral_error_turns_toward_tag(self):
        command = self.command(1.20, 0.20, math.pi)
        self.assertGreater(command.angular_rps, 0.0)

    def test_heading_error_is_corrected(self):
        command = self.command(1.20, 0.0, math.pi - 0.30)
        self.assertLess(command.angular_rps, 0.0)

    def test_in_tolerance_stops(self):
        command = self.command(0.37, 0.01, math.pi + 0.02)
        self.assertTrue(pose_is_within_tolerance(command, 0.04, 0.03, 0.05))


if __name__ == '__main__':
    unittest.main()
