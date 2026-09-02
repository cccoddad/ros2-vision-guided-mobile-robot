#ifndef BASE_DRIVER__TRANSPORT_HPP_
#define BASE_DRIVER__TRANSPORT_HPP_

#include <cstdint>
#include <optional>
#include <vector>

namespace base_driver
{

class Transport
{
public:
  using Frame = std::vector<std::uint8_t>;

  virtual ~Transport() = default;

  virtual bool write(Frame frame) = 0;
  virtual std::optional<Frame> read() = 0;
};

}  // namespace base_driver

#endif  // BASE_DRIVER__TRANSPORT_HPP_
