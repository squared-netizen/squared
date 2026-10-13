#pragma once

#include <cstdint>
#include <memory>

namespace sq::messaging {

/**
 * @brief Move-only scoped endpoint registration or broadcast subscription.
 *
 * Destroying or resetting the handle unregisters it. A subscription may
 * safely outlive its dispatcher, but a referenced Telegraph or
 * TelegramProvider must remain alive while the subscription is active.
 */
class Subscription final {
public:
    /** @brief Construct an inactive, empty subscription handle. */
    Subscription() noexcept = default;

    /** @brief Unregister from the dispatcher when still active. */
    ~Subscription();

    /**
     * @brief Move-construct from another handle.
     * @param other Handle to assume; left inactive afterward.
     */
    Subscription(Subscription&& other) noexcept;

    /**
     * @brief Move-assign from another handle.
     * @param other Handle to assume; left inactive afterward.
     * @return Reference to this handle.
     * @note Releases the current registration first, if any.
     */
    Subscription& operator=(Subscription&& other) noexcept;

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    /** @brief Unregister from the dispatcher, if still active. */
    void reset() noexcept;

    /**
     * @brief Check registration state.
     * @return true while the subscription is still registered.
     */
    [[nodiscard]] bool active() const noexcept;

private:
    struct Registry;

    Subscription(
        std::weak_ptr<Registry> registry,
        std::uint64_t token
    ) noexcept;

    std::weak_ptr<Registry> registry_;
    std::uint64_t token_{0};

    friend class MessageDispatcher;
};

}  // namespace sq::messaging
