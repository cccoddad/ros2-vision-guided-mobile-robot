#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"
#include "robot_interfaces/msg/base_status.hpp"

#include "base_driver/command_watchdog.hpp"

namespace
{

constexpr std::uint32_t kCommunicationTimeout = 1U << 0;
constexpr std::uint32_t kHardwareTransportDisabled = 1U << 1;

double clamp(double value, double lower, double upper)
{
  return std::max(lower, std::min(value, upper));
}

class BaseDriverNode : public rclcpp::Node
{
public:
  BaseDriverNode()
  : Node("base_driver"), watchdog_(std::chrono::milliseconds(300))
  {
    const auto update_rate_hz = declare_parameter<double>("update_rate_hz", 20.0);
    const auto command_timeout_s = declare_parameter<double>("command_timeout_s", 0.3);
    max_linear_speed_mps_ = declare_parameter<double>("max_linear_speed_mps", 0.20);
    max_angular_speed_rps_ = declare_parameter<double>("max_angular_speed_rps", 0.80);
    battery_voltage_ = declare_parameter<double>("battery_voltage", 0.0);
    base_frame_ = declare_parameter<std::string>("base_frame", "base_link");
    const auto transport_mode = declare_parameter<std::string>("transport_mode", "disabled");

    if (update_rate_hz <= 0.0 || command_timeout_s <= 0.0 ||
      max_linear_speed_mps_ <= 0.0 || max_angular_speed_rps_ <= 0.0)
    {
      throw std::invalid_argument("update rate, timeout, and speed limits must be positive");
    }
    if (transport_mode != "disabled") {
      throw std::invalid_argument("only the disabled transport mode is implemented in this stage");
    }

    watchdog_ = base_driver::CommandWatchdog(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::duration<double>(command_timeout_s)));

    status_publisher_ = create_publisher<robot_interfaces::msg::BaseStatus>("/base_status", 10);
    command_subscription_ = create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel", 10,
      std::bind(&BaseDriverNode::on_command, this, std::placeholders::_1));
    timer_ = create_wall_timer(
      std::chrono::duration<double>(1.0 / update_rate_hz),
      std::bind(&BaseDriverNode::publish_disabled_status, this));

    RCLCPP_WARN(
      get_logger(),
      "C++ base_driver scaffold is running with transport disabled. It accepts /cmd_vel and "
      "publishes /base_status, but deliberately publishes no /odom or TF without STM32 telemetry.");
  }

private:
  void on_command(const geometry_msgs::msg::Twist::SharedPtr command)
  {
    requested_linear_mps_ = clamp(
      command->linear.x, -max_linear_speed_mps_, max_linear_speed_mps_);
    requested_angular_rps_ = clamp(
      command->angular.z, -max_angular_speed_rps_, max_angular_speed_rps_);
    watchdog_.record_command(base_driver::CommandWatchdog::Clock::now());
  }

  void publish_disabled_status()
  {
    const auto steady_now = base_driver::CommandWatchdog::Clock::now();
    const bool timed_out = watchdog_.timed_out(steady_now);

    robot_interfaces::msg::BaseStatus status;
    status.header.stamp = now();
    status.header.frame_id = base_frame_;
    status.left_wheel_speed_mps = 0.0F;
    status.right_wheel_speed_mps = 0.0F;
    status.battery_voltage = static_cast<float>(battery_voltage_);
    status.fault_flags = kHardwareTransportDisabled | (timed_out ? kCommunicationTimeout : 0U);
    status.estop_active = false;
    status.command_age_s = static_cast<float>(watchdog_.command_age_seconds(steady_now));
    status.sequence = status_sequence_++;
    status_publisher_->publish(status);
  }

  base_driver::CommandWatchdog watchdog_;
  double max_linear_speed_mps_{0.0};
  double max_angular_speed_rps_{0.0};
  double battery_voltage_{0.0};
  std::string base_frame_;
  double requested_linear_mps_{0.0};
  double requested_angular_rps_{0.0};
  std::uint32_t status_sequence_{0U};
  rclcpp::Publisher<robot_interfaces::msg::BaseStatus>::SharedPtr status_publisher_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr command_subscription_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<BaseDriverNode>());
  rclcpp::shutdown();
  return 0;
}
