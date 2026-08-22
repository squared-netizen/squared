#include <algorithm>
#include <cmath>
#include <utility>

#include <squared/gui/gui.hpp>

namespace squared::gui {

ScrollBar::ScrollBar(Direction direction) noexcept : direction_(direction) {}

void ScrollBar::set_range(float minimum, float maximum) noexcept
{
    minimum_ = std::min(minimum, maximum);
    maximum_ = std::max(minimum, maximum);
    value_ = std::clamp(value_, minimum_, maximum_);
}

void ScrollBar::set_page_size(float page_size) noexcept
{
    page_size_ = std::max(0.0F, page_size);
}

void ScrollBar::set_value(float value)
{
    const float adjusted = std::clamp(value, minimum_, maximum_);
    if (adjusted == value_) return;
    value_ = adjusted;
    if (callback_) callback_(value_);
}

void ScrollBar::set_step(float step) noexcept
{
    step_ = std::max(0.0F, step);
}

void ScrollBar::set_on_change(ChangeCallback callback)
{
    callback_ = std::move(callback);
}

void ScrollBar::set_style(std::string style)
{
    style_ = std::move(style);
}

Size ScrollBar::minimum_size(Painter&, const Skin& skin) const
{
    const auto& style = skin.scroll_bar_style(style_);
    return direction_ == Direction::horizontal
               ? Size{style.minimum_touch_size * 2.0F, style.minimum_touch_size}
               : Size{style.minimum_touch_size, style.minimum_touch_size * 2.0F};
}

Size ScrollBar::preferred_size(Painter& painter, const Skin& skin) const
{
    return minimum_size(painter, skin);
}

float ScrollBar::axis_length() const noexcept
{
    return direction_ == Direction::horizontal ? width() : height();
}

float ScrollBar::knob_length(const ScrollBarStyle& style) const noexcept
{
    const float range = maximum_ - minimum_;
    const float total = range + page_size_;
    const float proportional = total > 0.0F ? axis_length() * page_size_ / total : axis_length();
    return std::clamp(proportional, std::min(axis_length(), style.minimum_knob_length),
                      axis_length());
}

void ScrollBar::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const auto& style = skin.scroll_bar_style(style_);
    const Rectangle bounds{x, y, width(), height()};
    if (style.track) style.track->draw(painter, bounds);
    const float knob = knob_length(style);
    const float travel = std::max(0.0F, axis_length() - knob);
    const float ratio = maximum_ > minimum_ ? (value_ - minimum_) / (maximum_ - minimum_) : 0.0F;
    Rectangle thumb = bounds;
    if (direction_ == Direction::horizontal) {
        thumb.x += travel * ratio;
        thumb.width = knob;
    } else {
        thumb.y += travel * ratio;
        thumb.height = knob;
    }
    if (style.knob) style.knob->draw(painter, thumb);
    if (focused_) painter.stroke_rectangle(bounds, skin.accent, 2.0F);
}

void ScrollBar::update_from_pointer(float coordinate, float grab_offset)
{
    const float total = page_size_ + maximum_ - minimum_;
    const float proportional = total > 0.0F ? axis_length() * page_size_ / total : axis_length();
    const float knob = std::min(axis_length(), std::max(24.0F, proportional));
    const float travel = std::max(0.0F, axis_length() - knob);
    const float ratio =
        travel > 0.0F ? std::clamp((coordinate - grab_offset) / travel, 0.0F, 1.0F) : 0.0F;
    set_value(minimum_ + ratio * (maximum_ - minimum_));
}

bool ScrollBar::pointer_event(const PointerEvent& event)
{
    if (!enabled()) return false;
    const float coordinate = direction_ == Direction::horizontal ? event.x : event.y;
    if (event.action == PointerAction::down && contains(event.x, event.y)) {
        const float total = page_size_ + maximum_ - minimum_;
        const float proportional =
            total > 0.0F ? axis_length() * page_size_ / total : axis_length();
        const float knob = std::min(axis_length(), std::max(24.0F, proportional));
        const float travel = std::max(0.0F, axis_length() - knob);
        const float ratio =
            maximum_ > minimum_ ? (value_ - minimum_) / (maximum_ - minimum_) : 0.0F;
        const float start = travel * ratio;
        if (coordinate >= start && coordinate <= start + knob) {
            grab_offset_ = coordinate - start;
        } else {
            grab_offset_ = knob * 0.5F;
            set_value(value_ + (coordinate < start ? -page_size_ : page_size_));
        }
        drag_pointer_ = event.pointer_id;
        return true;
    }
    if (!drag_pointer_ || *drag_pointer_ != event.pointer_id) return false;
    if (event.action == PointerAction::move) {
        update_from_pointer(coordinate, grab_offset_);
        return true;
    }
    drag_pointer_.reset();
    if (event.action == PointerAction::up) {
        update_from_pointer(coordinate, grab_offset_);
    }
    return true;
}

bool ScrollBar::key_down(Key key, KeyModifiers)
{
    if (!enabled()) return false;
    const bool decreasing = key == Key::left || key == Key::up;
    const bool increasing = key == Key::right || key == Key::down;
    if (key == Key::home) {
        set_value(minimum_);
        return true;
    }
    if (key == Key::end) {
        set_value(maximum_);
        return true;
    }
    if (!decreasing && !increasing) return false;
    const float amount = step_ > 0.0F ? step_ : std::max(1.0F, (maximum_ - minimum_) * 0.05F);
    set_value(value_ + (decreasing ? -amount : amount));
    return true;
}

void ScrollBar::focus_changed(bool focused)
{
    focused_ = focused;
}

bool ScrollBar::focusable() const noexcept
{
    return enabled();
}

bool SingleListSelectionModel::selected(std::size_t index) const noexcept
{
    return selected_ == index;
}

std::optional<std::size_t> SingleListSelectionModel::primary() const noexcept
{
    return selected_;
}

void SingleListSelectionModel::select(std::size_t index, KeyModifiers)
{
    selected_ = index;
}

void SingleListSelectionModel::clear() noexcept
{
    selected_.reset();
}

void SingleListSelectionModel::trim(std::size_t item_count) noexcept
{
    if (selected_ && *selected_ >= item_count) selected_.reset();
}

ListView::ListView(std::vector<std::string> items)
    : items_(std::move(items)), selection_model_(std::make_shared<SingleListSelectionModel>())
{}

void ListView::set_items(std::vector<std::string> items)
{
    items_ = std::move(items);
    selection_model_->trim(items_.size());
    scroll_index_ = std::min(scroll_index_, items_.size());
    invalidate_layout();
}

std::span<const std::string> ListView::items() const noexcept
{
    return items_;
}

void ListView::set_selection_model(std::shared_ptr<ListSelectionModel> model)
{
    selection_model_ = model ? std::move(model) : std::make_shared<SingleListSelectionModel>();
    selection_model_->trim(items_.size());
}

const std::shared_ptr<ListSelectionModel>& ListView::selection_model() const noexcept
{
    return selection_model_;
}

void ListView::set_scroll_index(std::size_t index) noexcept
{
    scroll_index_ = std::min(index, items_.size());
}

void ListView::set_on_selection_changed(SelectionCallback callback)
{
    callback_ = std::move(callback);
}

void ListView::set_style(std::string style)
{
    style_ = std::move(style);
}

Size ListView::minimum_size(Painter&, const Skin& skin) const
{
    const auto& style = skin.list_view_style(style_);
    resolved_row_height_ = std::max(1.0F, style.row_height);
    return {style.horizontal_padding * 2.0F + 40.0F, resolved_row_height_};
}

Size ListView::preferred_size(Painter& painter, const Skin& skin) const
{
    const auto& style = skin.list_view_style(style_);
    resolved_row_height_ = std::max(1.0F, style.row_height);
    float width = 0.0F;
    for (const auto& item : items_) {
        width = std::max(width, painter.measure_text(item, style.font.get()).width);
    }
    const float rows = static_cast<float>(std::min<std::size_t>(items_.size(), 6U));
    return {width + style.horizontal_padding * 2.0F, std::max(1.0F, rows) * resolved_row_height_};
}

std::size_t ListView::visible_rows(float row_height) const noexcept
{
    return std::max<std::size_t>(1U, static_cast<std::size_t>(std::floor(height() / row_height)));
}

void ListView::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const auto& style = skin.list_view_style(style_);
    resolved_row_height_ = std::max(1.0F, style.row_height);
    if (style.background) style.background->draw(painter, {x, y, width(), height()});
    const std::size_t end =
        std::min(items_.size(), scroll_index_ + visible_rows(resolved_row_height_) + 1U);
    for (std::size_t index = scroll_index_; index < end; ++index) {
        const float row_y = y + static_cast<float>(index - scroll_index_) * resolved_row_height_;
        const Rectangle row{x, row_y, width(), resolved_row_height_};
        const bool selected = selection_model_->selected(index);
        if (selected && style.selection) style.selection->draw(painter, row);
        painter.draw_text(items_[index], x + style.horizontal_padding,
                          row_y
                              + (resolved_row_height_
                                 - painter.measure_text(items_[index], style.font.get()).height)
                                    * 0.5F,
                          style.font.get(), selected ? style.selected_text : style.unselected_text);
    }
    if (focused_) painter.stroke_rectangle({x, y, width(), height()}, skin.accent, 2.0F);
}

void ListView::select_index(std::size_t index, KeyModifiers modifiers)
{
    if (index >= items_.size()) return;
    selection_model_->select(index, modifiers);
    reveal(index, resolved_row_height_);
    if (callback_) callback_(selection_model_->primary());
}

void ListView::reveal(std::size_t index, float row_height) noexcept
{
    const std::size_t visible = visible_rows(std::max(1.0F, row_height));
    if (index < scroll_index_)
        scroll_index_ = index;
    else if (index >= scroll_index_ + visible)
        scroll_index_ = index - visible + 1U;
}

bool ListView::pointer_event(const PointerEvent& event)
{
    if (!enabled() || event.action != PointerAction::down || !contains(event.x, event.y))
        return false;
    const auto row =
        static_cast<std::size_t>(std::max(0.0F, std::floor(event.y / resolved_row_height_)));
    select_index(scroll_index_ + row, {});
    return true;
}

bool ListView::key_down(Key key, KeyModifiers modifiers)
{
    if (!enabled() || items_.empty()) return false;
    std::size_t index = selection_model_->primary().value_or(0U);
    if (key == Key::up && index > 0U)
        --index;
    else if (key == Key::down && index + 1U < items_.size())
        ++index;
    else if (key == Key::home)
        index = 0U;
    else if (key == Key::end)
        index = items_.size() - 1U;
    else if (key != Key::up && key != Key::down)
        return false;
    select_index(index, modifiers);
    return true;
}

void ListView::focus_changed(bool focused)
{
    focused_ = focused;
}

bool ListView::focusable() const noexcept
{
    return enabled();
}

} // namespace squared::gui
