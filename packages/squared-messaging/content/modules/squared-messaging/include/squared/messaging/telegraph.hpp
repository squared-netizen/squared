#pragma once

namespace squared::messaging {

class Telegram;

/** @brief Receiver of Telegram values on the dispatcher's calling thread. */
class Telegraph {
public:
    virtual ~Telegraph() = default;

    /**
     * @brief Handle one Telegram.
     * @param telegram Immutable-access envelope to handle; valid for the
     * duration of the call.
     * @return true when the message was handled; false otherwise.
     * @throws Any exception propagates to the dispatcher caller; keep the
     * handler noexcept-safe.
     * @note Invoked synchronously on the dispatcher's calling thread. Must
     * not invoke dispatcher methods recursively.
     */
    [[nodiscard]]
    virtual bool handle_message(const Telegram& telegram) = 0;
};

}  // namespace squared::messaging
