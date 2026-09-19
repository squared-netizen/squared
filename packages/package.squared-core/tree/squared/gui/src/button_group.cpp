#include <squared/gui/button_group.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/toggle_button.hpp>

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

ButtonGroup::ButtonGroup(
    std::size_t minimum_checked,
    std::size_t maximum_checked
) : minimum_checked_(minimum_checked), maximum_checked_(maximum_checked)
{
    if (minimum_checked_ > maximum_checked_) {
        throw std::invalid_argument("ButtonGroup minimum exceeds maximum");
    }
}

ButtonGroup::~ButtonGroup()
{
    clear();
}

void ButtonGroup::add(ToggleButton& button)
{
    if (button.group_ == this) return;
    if (button.group_ != nullptr) {
        throw std::invalid_argument("ToggleButton already belongs to a group");
    }
    buttons_.push_back(&button);
    button.group_ = this;
    rebalance();
}

bool ButtonGroup::remove(ToggleButton& button)
{
    const auto found = std::find(buttons_.begin(), buttons_.end(), &button);
    if (found == buttons_.end()) return false;
    buttons_.erase(found);
    button.group_ = nullptr;
    rebalance();
    return true;
}

void ButtonGroup::clear() noexcept
{
    for (ToggleButton* button : buttons_) button->group_ = nullptr;
    buttons_.clear();
}

void ButtonGroup::set_limits(
    std::size_t minimum_checked,
    std::size_t maximum_checked
)
{
    if (minimum_checked > maximum_checked) {
        throw std::invalid_argument("ButtonGroup minimum exceeds maximum");
    }
    minimum_checked_ = minimum_checked;
    maximum_checked_ = maximum_checked;
    rebalance();
}

std::size_t ButtonGroup::checked_count() const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        buttons_.begin(), buttons_.end(),
        [](const ToggleButton* button) { return button->checked_; }
    ));
}

ToggleButton* ButtonGroup::checked_button() const noexcept
{
    const auto found = std::find_if(
        buttons_.begin(), buttons_.end(),
        [](const ToggleButton* button) { return button->checked_; }
    );
    return found == buttons_.end() ? nullptr : *found;
}

bool ButtonGroup::request_state(ToggleButton& button, bool checked)
{
    if (button.group_ != this || button.checked_ == checked) return false;
    std::size_t count = checked_count();
    if (!checked) {
        if (count <= std::min(minimum_checked_, buttons_.size())) return false;
        button.apply_checked(false);
        return true;
    }
    if (maximum_checked_ == 0) return false;
    for (ToggleButton* member : buttons_) {
        if (count < maximum_checked_) break;
        if (member != &button && member->checked_) {
            member->apply_checked(false);
            --count;
        }
    }
    if (count >= maximum_checked_) return false;
    button.apply_checked(true);
    return true;
}

void ButtonGroup::rebalance()
{
    std::size_t count = checked_count();
    for (auto iterator = buttons_.rbegin();
         count > maximum_checked_ && iterator != buttons_.rend(); ++iterator) {
        if ((*iterator)->checked_) {
            (*iterator)->apply_checked(false);
            --count;
        }
    }
    const std::size_t required = std::min(minimum_checked_, buttons_.size());
    for (ToggleButton* button : buttons_) {
        if (count >= required) break;
        if (!button->checked_) {
            button->apply_checked(true);
            ++count;
        }
    }
}

} // namespace sq::gui
