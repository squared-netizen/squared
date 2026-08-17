#pragma once

#include <squared/data/json.hpp>

#include <string>
#include <utility>

namespace squared::messaging {

class MessageId;

/** @brief Outcome category of one authoritative current-state request. */
enum class ProviderStatus {
    Provided,
    NoCurrentState,
    Failed
};

/** @brief Owned result of one authoritative current-state request. */
struct ProviderResult {
    /** @brief Outcome category of the request. */
    ProviderStatus status{ProviderStatus::NoCurrentState};

    /** @brief Owned JSON payload valid for the Provided outcome. */
    squared::data::JsonValue payload;

    /** @brief Human-readable failure detail valid for the Failed outcome. */
    std::string detail;

    /**
     * @brief Build a result carrying authoritative current state.
     * @param value Owned JSON payload; ownership is transferred into the
     * result.
     * @return Result with Provided status and the transferred payload.
     */
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

    /**
     * @brief Build a result declaring that no current state exists.
     * @return Default result with NoCurrentState status and an empty payload.
     */
    [[nodiscard]] static ProviderResult no_current_state()
    {
        return {};
    }

    /**
     * @brief Build a failed result.
     * @param reason Human-readable failure detail.
     * @return Result with Failed status and the transferred detail text.
     */
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

    /**
     * @brief Produce the authoritative current state for one message kind.
     * @param message Kind for which current state is requested.
     * @return Owned result carrying the current state, no state, or a failure;
     * ownership of any payload transfers to the caller.
     * @note Called synchronously on the dispatcher's calling thread during
     * subscriber registration. Must not invoke dispatcher methods.
     */
    [[nodiscard]]
    virtual ProviderResult provide(
        const MessageId& message
    ) = 0;
};

}  // namespace squared::messaging
