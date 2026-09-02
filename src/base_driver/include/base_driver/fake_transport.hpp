#ifndef BASE_DRIVER__FAKE_TRANSPORT_HPP_
#define BASE_DRIVER__FAKE_TRANSPORT_HPP_

#include <deque>
#include <optional>
#include <utility>

#include "base_driver/transport.hpp"

namespace base_driver
{

class FakeTransport final : public Transport
{
public:
  bool write(Frame frame) override
  {
    written_frames_.push_back(std::move(frame));
    return true;
  }

  std::optional<Frame> read() override
  {
    if (received_frames_.empty()) {
      return std::nullopt;
    }

    Frame frame = std::move(received_frames_.front());
    received_frames_.pop_front();
    return frame;
  }

  void push_received(Frame frame)
  {
    received_frames_.push_back(std::move(frame));
  }

  std::optional<Frame> take_written()
  {
    if (written_frames_.empty()) {
      return std::nullopt;
    }

    Frame frame = std::move(written_frames_.front());
    written_frames_.pop_front();
    return frame;
  }

private:
  std::deque<Frame> received_frames_;
  std::deque<Frame> written_frames_;
};

}  // namespace base_driver

#endif  // BASE_DRIVER__FAKE_TRANSPORT_HPP_
