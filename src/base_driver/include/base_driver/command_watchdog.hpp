#ifndef BASE_DRIVER__COMMAND_WATCHDOG_HPP_
#define BASE_DRIVER__COMMAND_WATCHDOG_HPP_

#include <algorithm>
#include <chrono>
#include <optional>

namespace base_driver
{

class CommandWatchdog
{
public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  explicit CommandWatchdog(std::chrono::nanoseconds timeout)
  : timeout_(timeout) {}

  void record_command(TimePoint now)
  {
    last_command_ = now;
  }

  bool timed_out(TimePoint now) const
  {
    return !last_command_.has_value() || now - *last_command_ > timeout_;
  }

  double command_age_seconds(TimePoint now) const
  {
    if (!last_command_.has_value()) {
      return 1'000'000.0;
    }
    const auto age = std::max(std::chrono::nanoseconds::zero(), now - *last_command_);
    return std::chrono::duration<double>(age).count();
  }

private:
  std::chrono::nanoseconds timeout_;
  std::optional<TimePoint> last_command_;
};

}  // namespace base_driver

#endif  // BASE_DRIVER__COMMAND_WATCHDOG_HPP_
