#include <squared/gui/dialog.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/alignment.hpp>
#include <squared/gui/button.hpp>
#include <squared/gui/label.hpp>
#include <squared/gui/table.hpp>
#include <squared/gui/window.hpp>

#include <memory>
#include <string>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Dialog::Dialog(std::string title, ResultCallback result)
    : Window(std::move(title)), result_(std::move(result))
{
    set_modal(true);
    auto content = std::make_unique<Table>();
    dialog_content_ = content.get();
    content_table().add(std::move(content)).grow().fill();
    content_table().row();
    auto buttons = std::make_unique<Table>();
    buttons_ = buttons.get();
    content_table().add(std::move(buttons)).grow_x().fill_x();
}

Dialog& Dialog::text(std::string text_value)
{
    dialog_content_->add(std::make_unique<Label>(std::move(text_value)))
        .align(Alignment::start);
    dialog_content_->row();
    return *this;
}

Dialog& Dialog::button(std::string text_value, std::string result_value)
{
    auto action = std::make_unique<Button>(std::move(text_value));
    action->set_on_click([this, result = std::move(result_value)]() mutable {
        choose(std::move(result));
    });
    buttons_->add(std::move(action)).grow_x().fill_x();
    return *this;
}

void Dialog::set_on_result(ResultCallback result) { result_ = std::move(result); }

bool Dialog::escape_closes() const noexcept { return cancel_on_escape_; }

void Dialog::choose(std::string result)
{
    request_close();
    if (result_) result_(result);
}

} // namespace sq::gui
