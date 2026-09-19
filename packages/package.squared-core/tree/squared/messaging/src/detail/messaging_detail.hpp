#pragma once

// Internal to the squared build: shared helpers lifted out of
// a per-translation-unit anonymous namespace so each type can
// own its own translation unit. Not installed and not part of
// the public API.

#include <squared/messaging/endpoint_id.hpp>
#include <squared/messaging/message_id.hpp>
#include <squared/messaging/subscription.hpp>
#include <squared/messaging/telegram_provider.hpp>
#include <squared/messaging/telegraph.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sq::messaging::detail {

inline bool valid_stable_id(std::string_view value) noexcept
{
    if (value.empty() || value.size() > 128) return false;
    const auto alphanumeric = [](unsigned char byte) {
        return
            (byte >= 'a' && byte <= 'z') ||
            (byte >= 'A' && byte <= 'Z') ||
            (byte >= '0' && byte <= '9');
    };
    if (!alphanumeric(
            static_cast<unsigned char>(value.front())
        ) ||
        !alphanumeric(
            static_cast<unsigned char>(value.back())
        )) {
        return false;
    }
    bool namespaced = false;
    for (const unsigned char byte : value) {
        const bool separator =
            byte == '.' || byte == '/' || byte == ':';
        if (!alphanumeric(byte) && !separator &&
            byte != '-' && byte != '_') {
            return false;
        }
        namespaced = namespaced || separator;
    }
    return namespaced;
}

}  // namespace sq::messaging::detail

namespace sq::messaging {

struct Subscription::Registry {
    enum class Kind {
        Endpoint,
        Broadcast,
        Provider
    };

    struct Binding {
        std::uint64_t token;
        Kind kind;
        EndpointId endpoint;
        MessageId message;
        Telegraph* telegraph;
        TelegramProvider* provider;
    };

    explicit Registry(std::size_t capacity_value)
        : capacity(capacity_value)
    {
        bindings.reserve(capacity);
    }

    void remove(std::uint64_t token) noexcept
    {
        const auto iterator = std::find_if(
            bindings.begin(),
            bindings.end(),
            [token](const Binding& binding) {
                return binding.token == token;
            }
        );
        if (iterator != bindings.end()) bindings.erase(iterator);
    }

    [[nodiscard]] bool contains(std::uint64_t token) const noexcept
    {
        return std::any_of(
            bindings.begin(),
            bindings.end(),
            [token](const Binding& binding) {
                return binding.token == token;
            }
        );
    }

    std::vector<Binding> bindings;
    std::size_t capacity;
    std::uint64_t next_token{0};
};

}  // namespace sq::messaging
