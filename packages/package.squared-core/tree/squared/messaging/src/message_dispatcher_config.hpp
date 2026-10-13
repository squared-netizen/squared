#pragma once

#include <cstddef>

namespace sq::messaging {

/** @brief Capacity limits that bound every dispatcher queue. */
struct MessageDispatcherConfig {
    /** @brief Maximum pending Telegram count; must be at least one. */
    std::size_t pending_capacity{1024};

    /** @brief Maximum simultaneous producer registrations; at least one. */
    std::size_t subscription_capacity{256};

    /** @brief Maximum deliveries drained per update() call; at least one. */
    std::size_t maximum_deliveries_per_update{256};
};

}  // namespace sq::messaging
