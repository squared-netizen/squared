#pragma once

#include <squared/data/json.hpp>

#include <string>
#include <utility>

namespace squared::messaging {

class MessageId;

enum class ProviderStatus {
    Provided,
    NoCurrentState,
    Failed
};

/** @brief Owned result of one authoritative current-state request. */
struct ProviderResult {
    ProviderStatus status{ProviderStatus::NoCurrentState};
    squared::data::JsonValue payload;
    std::string detail;

    [[nodiscard]] static ProviderResult provided(
        squared::data::JsonValue value
    )
    {
        return {
            ProviderStatus::Provided,
            std::move(value),
            {}
        };
    }

    [[nodiscard]] static ProviderResult no_current_state()
    {
        return {};
    }

    [[nodiscard]] static ProviderResult failed(std::string reason)
    {
        return {
            ProviderStatus::Failed,
            {},
            std::move(reason)
        };
    }
};

/**
 * @brief Generates authoritative current state for a new subscriber.
 *
 * Providers replace generic retained-message replay, which can deliver stale
 * historical envelopes. The dispatcher queues a fresh result only for the
 * newly registered subscriber.
 */
class TelegramProvider {
public:
    virtual ~TelegramProvider() = default;

    [[nodiscard]]
    virtual ProviderResult provide(
        const MessageId& message
    ) = 0;
};

}  // namespace squared::messaging
