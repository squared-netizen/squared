#pragma once

#include <string>
#include <string_view>

namespace sq::messaging {

/**
 * @brief Stable namespaced identifier for one Telegram kind.
 *
 * Valid identifiers contain 1-128 ASCII letters, digits, dots, dashes,
 * underscores, slashes, or colons and include at least one namespace
 * separator (`.`, `/`, or `:`).
 */
class MessageId final {
public:
    /** @brief Construct an empty, invalid identifier. */
    MessageId() noexcept = default;

    /**
     * @brief Construct from a candidate namespace string.
     * @param value Candidate identifier; validated on construction.
     */
    explicit MessageId(std::string value);

    /**
     * @brief Validate the stored identifier.
     * @return true when the stored value satisfied the identifier rules.
     */
    [[nodiscard]] bool valid() const noexcept;

    /**
     * @brief Access the stored identifier text.
     * @return Text of the candidate identifier.
     */
    [[nodiscard]] std::string_view value() const noexcept;

    /** @brief Return whether two identifiers are textually equal. */
    friend bool operator==(
        const MessageId&,
        const MessageId&
    ) = default;

private:
    std::string value_;
    bool valid_{false};
};

}  // namespace sq::messaging
