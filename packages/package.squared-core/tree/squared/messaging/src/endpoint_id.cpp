#include <squared/messaging/endpoint_id.hpp>

#include "detail/messaging_detail.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace sq::messaging {

EndpointId::EndpointId(std::string value)
    : value_(std::move(value)),
      valid_(detail::valid_stable_id(value_))
{
}

bool EndpointId::valid() const noexcept
{
    return valid_;
}

std::string_view EndpointId::value() const noexcept
{
    return value_;
}

}  // namespace sq::messaging
