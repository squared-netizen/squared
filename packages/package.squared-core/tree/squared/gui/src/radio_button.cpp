#include <squared/gui/radio_button.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/check_box.hpp>

#include <string>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

RadioButton::RadioButton(std::string text, bool checked)
    : CheckBox(std::move(text), checked)
{
    set_check_style("radio");
}

} // namespace sq::gui
