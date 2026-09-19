#pragma once

#include <squared/data/json.hpp>
#include <squared/messaging/provider_status.hpp>

#include <string>
#include <utility>

namespace sq::messaging {

/** @brief Owned result of one authoritative current-state request. */
struct ProviderResult {
    /** @brief Outcome category of the request. */
    ProviderStatus status{ProviderStatus::NoCurrentState};

    /** @brief Owned JSON payload valid for the Provided outcome. */
    sq::data::JsonValue payload;

    /** @brief Human-readable failure detail valid for the Failed outcome. */
    std::string detail;

    /**
     * @brief Build a result carrying authoritative current state.
     * @param value Owned JSON payload; ownership is transferred into the
     * result.
     * @return Result with Provided status and the transferred payload.
     */
    [[nodiscard]] static ProviderResult provided(
        sq::data::JsonValue value
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

}  // namespace sq::messaging
