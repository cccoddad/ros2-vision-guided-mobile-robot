#ifndef BASE_DRIVER__DRIVER_HEALTH_HPP_
#define BASE_DRIVER__DRIVER_HEALTH_HPP_

#include <string>

#include "base_driver/base_status_receiver.hpp"

namespace base_driver
{

enum class DriverLifecycleState
{
  kUnconfigured,
  kInactive,
  kActive,
  kError,
};

enum class DriverDiagnosticLevel
{
  kOk,
  kWarning,
  kError,
};

struct DriverDiagnostic
{
  DriverDiagnosticLevel level;
  DriverLifecycleState lifecycle_state;
  std::string summary;
};

class DriverHealth
{
public:
  bool configure()
  {
    if (lifecycle_state_ != DriverLifecycleState::kUnconfigured) {
      return false;
    }
    lifecycle_state_ = DriverLifecycleState::kInactive;
    return true;
  }

  bool activate()
  {
    if (lifecycle_state_ != DriverLifecycleState::kInactive || !status_is_healthy()) {
      return false;
    }
    lifecycle_state_ = DriverLifecycleState::kActive;
    return true;
  }

  bool deactivate()
  {
    if (lifecycle_state_ != DriverLifecycleState::kActive) {
      return false;
    }
    lifecycle_state_ = DriverLifecycleState::kInactive;
    return true;
  }

  bool recover()
  {
    if (lifecycle_state_ != DriverLifecycleState::kError || !status_is_healthy()) {
      return false;
    }
    lifecycle_state_ = DriverLifecycleState::kInactive;
    return true;
  }

  void observe(
    StatusReceiveResult receive_result,
    bool communication_timed_out,
    bool estop_active)
  {
    receive_result_ = receive_result;
    communication_timed_out_ = communication_timed_out;
    estop_active_ = estop_active;

    if (communication_timed_out_ || estop_active_) {
      lifecycle_state_ = DriverLifecycleState::kError;
    }
  }

  DriverDiagnostic diagnostic() const
  {
    if (estop_active_) {
      return {DriverDiagnosticLevel::kError, lifecycle_state_, "STM32 emergency stop active"};
    }
    if (communication_timed_out_) {
      return {DriverDiagnosticLevel::kError, lifecycle_state_, "base status communication timeout"};
    }
    if (receive_result_ == StatusReceiveResult::kCrcMismatch) {
      return {DriverDiagnosticLevel::kWarning, lifecycle_state_, "base status CRC mismatch"};
    }
    if (receive_result_ == StatusReceiveResult::kOutOfOrderSequence) {
      return {DriverDiagnosticLevel::kWarning, lifecycle_state_, "base status sequence out of order"};
    }
    if (receive_result_ != StatusReceiveResult::kAccepted) {
      return {DriverDiagnosticLevel::kWarning, lifecycle_state_, "awaiting a valid base status"};
    }
    return {DriverDiagnosticLevel::kOk, lifecycle_state_, "base status healthy"};
  }

  DriverLifecycleState lifecycle_state() const
  {
    return lifecycle_state_;
  }

private:
  bool status_is_healthy() const
  {
    return receive_result_ == StatusReceiveResult::kAccepted &&
           !communication_timed_out_ && !estop_active_;
  }

  DriverLifecycleState lifecycle_state_{DriverLifecycleState::kUnconfigured};
  StatusReceiveResult receive_result_{StatusReceiveResult::kNoFrame};
  bool communication_timed_out_{true};
  bool estop_active_{false};
};

}  // namespace base_driver

#endif  // BASE_DRIVER__DRIVER_HEALTH_HPP_
