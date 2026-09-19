#pragma once

#include <string>
#include <string_view>

namespace sq::messaging {

/** @brief Stable namespaced address for one directed Telegraph endpoint. */
class EndpointId final {
public:
    /** @brief Construct an empty, invalid endpoint address. */
    EndpointId() noexcept = default;

    /**
     * @brief Construct from a candidate endpoint string.
     * @param value Candidate endpoint; validated on construction.
     */
    explicit EndpointId(std::string value);

    /**
     * @brief Validate the stored endpoint address.
     * @return true when the stored value satisfied the address rules.
     */
    [[nodiscard]] bool valid() const noexcept;

    /**
     * @brief Access the stored endpoint text.
     * @return Text of the candidate endpoint.
     */
    [[nodiscard]] std::string_view value() const noexcept;

    /** @brief Return whether two endpoints are textually equal. */
    friend bool operator==(
        const EndpointId&,
        const EndpointId&
    ) = default;

private:
    std::string value_;
    bool valid_{false};
};

}  // namespace sq::messaging
