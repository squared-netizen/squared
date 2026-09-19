#include <squared/messaging/message_id.hpp>

#include "detail/messaging_detail.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace sq::messaging {

MessageId::MessageId(std::string value)
    : value_(std::move(value)),
      valid_(detail::valid_stable_id(value_))
{
}

bool MessageId::valid() const noexcept
{
    return valid_;
}

std::string_view MessageId::value() const noexcept
{
    return value_;
}

}  // namespace sq::messaging
