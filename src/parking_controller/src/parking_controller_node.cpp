#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

#include "geometry_msgs/msg/quaternion.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/executors/multi_threaded_executor.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "robot_interfaces/action/park_to_tag.hpp"
#include "robot_interfaces/msg/tag_pose.hpp"

#include "parking_controller/control_math.hpp"

namespace
{

constexpr uint8_t kStateWaitingForTag = 1U;
constexpr uint8_t kStateAligning = 2U;
constexpr uint8_t kStateApproaching = 3U;
constexpr uint8_t kStateParked = 4U;

constexpr uint8_t kFailureNone = 0U;
constexpr uint8_t kFailureTagTimeout = 1U;
constexpr uint8_t kFailureTaskTimeout = 2U;
constexpr uint8_t kFailureCancelled = 3U;

class ParkingControllerNode : public rclcpp::Node
{
public:
  using ParkToTag = robot_interfaces::action::ParkToTag;
  using GoalHandleParkToTag = rclcpp_action::ServerGoalHandle<ParkToTag>;

  ParkingControllerNode()
  : Node("parking_controller")
  {
    tag_pose_topic_ = declare_parameter<std::string>("tag_pose_topic", "/sim/tag_pose");
    supported_tag_id_ = declare_parameter<int>("supported_tag_id", 0);
    tag_timeout_s_ = declare_parameter<double>("tag_timeout_s", 0.5);
    distance_tolerance_m_ = declare_parameter<double>("distance_tolerance_m", 0.04);
    max_linear_mps_ = declare_parameter<double>("max_linear_mps", 0.18);
    max_angular_rps_ = declare_parameter<double>("max_angular_rps", 0.70);
    distance_gain_ = declare_parameter<double>("distance_gain", 0.60);
    heading_gain_ = declare_parameter<double>("heading_gain", 1.40);
    lateral_gain_ = declare_parameter<double>("lateral_gain", 1.20);

    if (tag_timeout_s_ <= 0.0 || distance_tolerance_m_ <= 0.0 ||
      max_linear_mps_ <= 0.0 || max_angular_rps_ <= 0.0)
    {
      throw std::invalid_argument("parking-controller timeouts, tolerances, and speed limits must be positive");
    }

    tag_subscription_ = create_subscription<robot_interfaces::msg::TagPose>(
      tag_pose_topic_, 10,
      std::bind(&ParkingControllerNode::on_tag_pose, this, std::placeholders::_1));
    command_publisher_ = create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
    action_server_ = rclcpp_action::create_server<ParkToTag>(
      this,
      "/park_to_tag",
      std::bind(&ParkingControllerNode::handle_goal, this, std::placeholders::_1, std::placeholders::_2),
      std::bind(&ParkingControllerNode::handle_cancel, this, std::placeholders::_1),
      std::bind(&ParkingControllerNode::handle_accepted, this, std::placeholders::_1));

    RCLCPP_INFO(
      get_logger(), "C++ parking controller ready; waiting for TagPose on %s.", tag_pose_topic_.c_str());
  }

private:
  struct TimedTagPose
  {
    robot_interfaces::msg::TagPose message;
    std::chrono::steady_clock::time_point received_at;
  };

  void on_tag_pose(const robot_interfaces::msg::TagPose::SharedPtr message)
  {
    std::lock_guard<std::mutex> lock(tag_mutex_);
    latest_tag_ = TimedTagPose{*message, std::chrono::steady_clock::now()};
  }

  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID &,
    std::shared_ptr<const ParkToTag::Goal> goal)
  {
    if (goal->tag_id != supported_tag_id_) {
      RCLCPP_WARN(get_logger(), "Rejecting unsupported Tag id %d.", goal->tag_id);
      return rclcpp_action::GoalResponse::REJECT;
    }
    if (goal->desired_distance_m < 0.10F || goal->timeout_s <= 0.0F) {
      RCLCPP_WARN(get_logger(), "Rejecting invalid parking goal.");
      return rclcpp_action::GoalResponse::REJECT;
    }
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse handle_cancel(const std::shared_ptr<GoalHandleParkToTag>)
  {
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void handle_accepted(const std::shared_ptr<GoalHandleParkToTag> goal_handle)
  {
    std::thread{std::bind(&ParkingControllerNode::execute, this, goal_handle)}.detach();
  }

  void execute(const std::shared_ptr<GoalHandleParkToTag> goal_handle)
  {
    const auto goal = goal_handle->get_goal();
    const auto started_at = std::chrono::steady_clock::now();
    rclcpp::Rate rate(20.0);
    bool succeeded = false;
    uint8_t failure_code = kFailureNone;

    while (rclcpp::ok()) {
      const auto steady_now = std::chrono::steady_clock::now();
      const double elapsed_s = std::chrono::duration<double>(steady_now - started_at).count();

      if (goal_handle->is_canceling()) {
        failure_code = kFailureCancelled;
        publish_stop();
        finish_goal(goal_handle, false, failure_code, started_at, true);
        return;
      }
      if (elapsed_s > goal->timeout_s) {
        failure_code = kFailureTaskTimeout;
        publish_stop();
        finish_goal(goal_handle, false, failure_code, started_at, false);
        return;
      }

      const auto latest_tag = latest_tag_copy();
      const bool tag_is_stale = !latest_tag.has_value() ||
        std::chrono::duration<double>(steady_now - latest_tag->received_at).count() > tag_timeout_s_;
      if (tag_is_stale) {
        publish_feedback(goal_handle, kStateWaitingForTag, false, 0.0, 0.0, 0.0, 0.0, 0.0);
        publish_stop();
        if (elapsed_s > tag_timeout_s_) {
          failure_code = kFailureTagTimeout;
          finish_goal(goal_handle, false, failure_code, started_at, false);
          return;
        }
        rate.sleep();
        continue;
      }

      const auto & tag = latest_tag->message;
      if (tag.tag_id != goal->tag_id) {
        rate.sleep();
        continue;
      }

      const auto relative_yaw_rad = yaw_from_quaternion(tag.pose.orientation);
      const auto command = parking_controller::calculate_command(
        tag.pose.position.x,
        tag.pose.position.y,
        relative_yaw_rad,
        goal->desired_distance_m,
        max_linear_mps_,
        max_angular_rps_,
        distance_gain_,
        heading_gain_,
        lateral_gain_);
      const bool is_parked = parking_controller::pose_is_within_tolerance(
        command,
        distance_tolerance_m_,
        goal->lateral_tolerance_m,
        goal->yaw_tolerance_rad);
      const auto state = is_parked ? kStateParked :
        (std::abs(command.heading_error_rad) > goal->yaw_tolerance_rad ?
        kStateAligning : kStateApproaching);

      publish_feedback(
        goal_handle,
        state,
        true,
        tag.pose.position.x,
        tag.pose.position.y,
        relative_yaw_rad,
        is_parked ? 0.0 : command.linear_mps,
        is_parked ? 0.0 : command.angular_rps);
      if (is_parked) {
        publish_stop();
        succeeded = true;
        break;
      }

      publish_command(command.linear_mps, command.angular_rps);
      rate.sleep();
    }

    publish_stop();
    finish_goal(goal_handle, succeeded, failure_code, started_at, false);
  }

  void finish_goal(
    const std::shared_ptr<GoalHandleParkToTag> & goal_handle,
    bool succeeded,
    uint8_t failure_code,
    const std::chrono::steady_clock::time_point & started_at,
    bool cancelled)
  {
    const auto result = std::make_shared<ParkToTag::Result>();
    result->success = succeeded;
    result->failure_code = failure_code;
    result->elapsed_time_s = static_cast<float>(
      std::chrono::duration<double>(std::chrono::steady_clock::now() - started_at).count());

    const auto tag = latest_tag_copy();
    if (tag.has_value()) {
      result->final_distance_m = static_cast<float>(
        parking_controller::distance_to_tag_m(tag->message.pose.position.x, tag->message.pose.position.y));
      result->final_lateral_error_m = static_cast<float>(tag->message.pose.position.y);
      result->final_yaw_error_rad = static_cast<float>(parking_controller::normalize_angle(
          yaw_from_quaternion(tag->message.pose.orientation) - parking_controller::kPi));
    }

    if (cancelled) {
      goal_handle->canceled(result);
    } else if (succeeded) {
      goal_handle->succeed(result);
    } else {
      goal_handle->abort(result);
    }
  }

  std::optional<TimedTagPose> latest_tag_copy() const
  {
    std::lock_guard<std::mutex> lock(tag_mutex_);
    return latest_tag_;
  }

  void publish_feedback(
    const std::shared_ptr<GoalHandleParkToTag> & goal_handle,
    uint8_t state,
    bool tag_visible,
    double relative_x_m,
    double relative_y_m,
    double relative_yaw_rad,
    double linear_mps,
    double angular_rps)
  {
    auto feedback = std::make_shared<ParkToTag::Feedback>();
    feedback->state = state;
    feedback->tag_visible = tag_visible;
    feedback->relative_x_m = static_cast<float>(relative_x_m);
    feedback->relative_y_m = static_cast<float>(relative_y_m);
    feedback->relative_yaw_rad = static_cast<float>(relative_yaw_rad);
    feedback->command_linear_mps = static_cast<float>(linear_mps);
    feedback->command_angular_rps = static_cast<float>(angular_rps);
    feedback->retry_count = 0U;
    goal_handle->publish_feedback(feedback);
  }

  void publish_command(double linear_mps, double angular_rps)
  {
    geometry_msgs::msg::Twist command;
    command.linear.x = linear_mps;
    command.angular.z = angular_rps;
    command_publisher_->publish(command);
  }

  void publish_stop()
  {
    command_publisher_->publish(geometry_msgs::msg::Twist{});
  }

  static double yaw_from_quaternion(const geometry_msgs::msg::Quaternion & quaternion)
  {
    return std::atan2(
      2.0 * (quaternion.w * quaternion.z + quaternion.x * quaternion.y),
      1.0 - 2.0 * (quaternion.y * quaternion.y + quaternion.z * quaternion.z));
  }

  std::string tag_pose_topic_;
  int supported_tag_id_{0};
  double tag_timeout_s_{0.5};
  double distance_tolerance_m_{0.04};
  double max_linear_mps_{0.18};
  double max_angular_rps_{0.70};
  double distance_gain_{0.60};
  double heading_gain_{1.40};
  double lateral_gain_{1.20};
  mutable std::mutex tag_mutex_;
  std::optional<TimedTagPose> latest_tag_;
  rclcpp::Subscription<robot_interfaces::msg::TagPose>::SharedPtr tag_subscription_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr command_publisher_;
  rclcpp_action::Server<ParkToTag>::SharedPtr action_server_;
};

}  // namespace

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ParkingControllerNode>();
  rclcpp::executors::MultiThreadedExecutor executor(rclcpp::ExecutorOptions(), 2U);
  executor.add_node(node);
  executor.spin();
  rclcpp::shutdown();
  return 0;
}
