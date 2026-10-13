#pragma once

#include <cstddef>
#include <limits>
#include <vector>

namespace sq::gui {

class ToggleButton;

/**
 * @brief Non-owning coordinator for toggle buttons and radio-style choices.
 *
 * A button may belong to at most one group. The group and every registered
 * button detach from each other during destruction, so either may be owned by
 * an ordinary Widget subtree without imposing a second ownership hierarchy.
 */
class ButtonGroup final {
public:
    /**
     * @brief Construct a checked-count policy.
     * @param minimum_checked Minimum checked buttons while members exist.
     * @param maximum_checked Maximum checked buttons; must not be smaller
     * than `minimum_checked`.
     * @throws std::invalid_argument when the limits are reversed.
     */
    explicit ButtonGroup(
        std::size_t minimum_checked = 0,
        std::size_t maximum_checked =
            std::numeric_limits<std::size_t>::max()
    );
    ~ButtonGroup();

    ButtonGroup(const ButtonGroup&) = delete;
    ButtonGroup& operator=(const ButtonGroup&) = delete;
    ButtonGroup(ButtonGroup&&) = delete;
    ButtonGroup& operator=(ButtonGroup&&) = delete;

    /**
     * @brief Register a non-owned toggle button.
     * @param button Button that must outlive the call; duplicate adds are no-ops.
     * @throws std::invalid_argument when the button already belongs to another
     * group.
     */
    void add(ToggleButton& button);

    /**
     * @brief Detach one registered button.
     * @param button Candidate member.
     * @return true when the button was a member.
     */
    bool remove(ToggleButton& button);

    /** @brief Detach every member without destroying any button. */
    void clear() noexcept;

    /**
     * @brief Replace the checked-count policy and rebalance current members.
     * @param minimum_checked Minimum checked buttons while members exist.
     * @param maximum_checked Maximum checked buttons.
     * @throws std::invalid_argument when the limits are reversed.
     */
    void set_limits(std::size_t minimum_checked, std::size_t maximum_checked);

    /** @brief Return the number of registered buttons. */
    [[nodiscard]] std::size_t size() const noexcept { return buttons_.size(); }

    /** @brief Return the number of currently checked members. */
    [[nodiscard]] std::size_t checked_count() const noexcept;

    /** @brief Return the first checked member, or null when none is checked. */
    [[nodiscard]] ToggleButton* checked_button() const noexcept;

private:
    friend class ToggleButton;
    bool request_state(ToggleButton& button, bool checked);
    void rebalance();

    std::vector<ToggleButton*> buttons_;
    std::size_t minimum_checked_{0};
    std::size_t maximum_checked_{std::numeric_limits<std::size_t>::max()};
};

} // namespace sq::gui
