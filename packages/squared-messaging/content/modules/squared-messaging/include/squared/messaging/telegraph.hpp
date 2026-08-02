#pragma once

namespace squared::messaging {

class Telegram;

/** @brief Receiver of Telegram values on the dispatcher's calling thread. */
class Telegraph {
public:
    virtual ~Telegraph() = default;

    /**
     * @brief Handle one Telegram.
     * @return true when the message was handled; false otherwise.
     */
    [[nodiscard]]
    virtual bool handle_message(const Telegram& telegram) = 0;
};

}  // namespace squared::messaging
