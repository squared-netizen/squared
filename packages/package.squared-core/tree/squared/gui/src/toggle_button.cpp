#include <squared/gui/toggle_button.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/button.hpp>
#include <squared/gui/button_group.hpp>

#include <string>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

ToggleButton::ToggleButton(std::string text, bool checked)
    : Button(std::move(text)), checked_(checked)
{
}

ToggleButton::~ToggleButton()
{
    if (group_ != nullptr) {
        try {
            static_cast<void>(group_->remove(*this));
        } catch (...) {
            // Destruction has already detached this button before rebalance.
        }
    }
}

void ToggleButton::set_checked(bool checked)
{
    if (checked_ == checked) return;
    if (group_ != nullptr) {
        static_cast<void>(group_->request_state(*this, checked));
        return;
    }
    apply_checked(checked);
}

void ToggleButton::apply_checked(bool checked)
{
    if (checked_ == checked) return;
    checked_ = checked;
    if (change_callback_) change_callback_(checked_);
}

void ToggleButton::set_on_change(ChangeCallback callback)
{
    change_callback_ = std::move(callback);
}

void ToggleButton::activate()
{
    set_checked(!checked_);
    Button::activate();
}

bool ToggleButton::selected() const noexcept { return checked_; }

} // namespace sq::gui
