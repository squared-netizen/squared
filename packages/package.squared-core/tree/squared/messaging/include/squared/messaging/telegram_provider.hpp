#pragma once

#include <squared/messaging/provider_result.hpp>

namespace sq::messaging {

class MessageId;

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

}  // namespace sq::messaging
