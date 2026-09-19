#include <squared/messaging/subscription.hpp>

#include "detail/messaging_detail.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace sq::messaging {

Subscription::Subscription(
    std::weak_ptr<Registry> registry,
    std::uint64_t token
) noexcept
    : registry_(std::move(registry)),
      token_(token)
{
}

Subscription::~Subscription()
{
    reset();
}

Subscription::Subscription(Subscription&& other) noexcept
    : registry_(std::move(other.registry_)),
      token_(std::exchange(other.token_, 0))
{
}

Subscription& Subscription::operator=(Subscription&& other) noexcept
{
    if (this == &other) return *this;
    reset();
    registry_ = std::move(other.registry_);
    token_ = std::exchange(other.token_, 0);
    return *this;
}

void Subscription::reset() noexcept
{
    if (token_ == 0) return;
    if (const auto registry = registry_.lock()) {
        registry->remove(token_);
    }
    token_ = 0;
    registry_.reset();
}

bool Subscription::active() const noexcept
{
    if (token_ == 0) return false;
    const auto registry = registry_.lock();
    return registry && registry->contains(token_);
}

}  // namespace sq::messaging
