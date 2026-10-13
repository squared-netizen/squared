#pragma once

namespace sq::messaging {

/** @brief Outcome category of one authoritative current-state request. */
enum class ProviderStatus {
    Provided,
    NoCurrentState,
    Failed
};

}  // namespace sq::messaging
