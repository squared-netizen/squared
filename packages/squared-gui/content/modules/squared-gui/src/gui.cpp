#include <squared/gui/gui.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace squared::gui {
namespace {

constexpr float default_padding = 8.0F;
constexpr float default_spacing = 6.0F;
constexpr float default_touch_size = 44.0F;

graphics::Color multiply(
    graphics::Color left,
    graphics::Color right
) noexcept
{
    return {
        left.red * right.red,
        left.green * right.green,
        left.blue * right.blue,
        left.alpha * right.alpha
    };
}

Rectangle bounds_of(const Widget& widget, float x, float y) noexcept
{
    return {x, y, widget.width(), widget.height()};
}

float clamp_dimension(float value, float minimum, float maximum) noexcept
{
    return std::min(std::max(value, minimum), maximum);
}

std::size_t previous_codepoint(const std::string& text, std::size_t cursor)
{
    if (cursor == 0) return 0;
    --cursor;
    while (cursor > 0 &&
           (static_cast<unsigned char>(text[cursor]) & 0xC0U) == 0x80U) {
        --cursor;
    }
    return cursor;
}

std::size_t next_codepoint(const std::string& text, std::size_t cursor)
{
    if (cursor >= text.size()) return text.size();
    ++cursor;
    while (cursor < text.size() &&
           (static_cast<unsigned char>(text[cursor]) & 0xC0U) == 0x80U) {
        ++cursor;
    }
    return cursor;
}

template <typename Map>
const typename Map::mapped_type& style_or_default(
    const Map& styles,
    std::string_view name
)
{
    const auto found = styles.find(std::string(name));
    if (found != styles.end()) return found->second;
    return styles.at("default");
}

} // namespace

ColorDrawable::ColorDrawable(
    graphics::Color color,
    Size minimum,
    Insets insets
) noexcept
    : color_(color), minimum_(minimum), insets_(insets)
{
}

Size ColorDrawable::minimum_size() const noexcept { return minimum_; }
Insets ColorDrawable::content_insets() const noexcept { return insets_; }

void ColorDrawable::draw(
    Painter& painter,
    const Rectangle& rectangle,
    graphics::Color tint
) const
{
    painter.fill_rectangle(rectangle, multiply(color_, tint));
}

RegionDrawable::RegionDrawable(
    const graphics2d::TextureRegion& region,
    Insets insets
) noexcept
    : region_(&region), insets_(insets)
{
}

Size RegionDrawable::minimum_size() const noexcept
{
    return {
        static_cast<float>(region_->width()),
        static_cast<float>(region_->height())
    };
}

Insets RegionDrawable::content_insets() const noexcept { return insets_; }

void RegionDrawable::draw(
    Painter& painter,
    const Rectangle& rectangle,
    graphics::Color tint
) const
{
    painter.draw_region(*region_, rectangle, tint);
}

NinePatchDrawable::NinePatchDrawable(
    const graphics2d::TextureRegion& region,
    NinePatchSplits splits,
    std::optional<Insets> content_insets
)
    : splits_(splits)
{
    if (splits.left < 0 || splits.top < 0 || splits.right < 0 ||
        splits.bottom < 0 || splits.left + splits.right > region.width() ||
        splits.top + splits.bottom > region.height()) {
        throw std::invalid_argument("Nine-patch splits exceed the source region");
    }
    const Insets requested_insets = content_insets.value_or(Insets{
        static_cast<float>(splits.left),
        static_cast<float>(splits.top),
        static_cast<float>(splits.right),
        static_cast<float>(splits.bottom)
    });
    insets_ = {
        std::max(0.0F, requested_insets.left),
        std::max(0.0F, requested_insets.top),
        std::max(0.0F, requested_insets.right),
        std::max(0.0F, requested_insets.bottom)
    };

    const int widths[]{
        splits.left,
        region.width() - splits.left - splits.right,
        splits.right
    };
    const int heights[]{
        splits.top,
        region.height() - splits.top - splits.bottom,
        splits.bottom
    };
    int source_y = 0;
    for (std::size_t row = 0; row < 3; ++row) {
        int source_x = 0;
        for (std::size_t column = 0; column < 3; ++column) {
            if (widths[column] > 0 && heights[row] > 0) {
                regions_[row * 3 + column] = region.subregion(
                    source_x, source_y, widths[column], heights[row]
                );
            }
            source_x += widths[column];
        }
        source_y += heights[row];
    }
}

Size NinePatchDrawable::minimum_size() const noexcept
{
    return {
        static_cast<float>(splits_.left + splits_.right),
        static_cast<float>(splits_.top + splits_.bottom)
    };
}

Insets NinePatchDrawable::content_insets() const noexcept { return insets_; }

void NinePatchDrawable::draw(
    Painter& painter,
    const Rectangle& rectangle,
    graphics::Color tint
) const
{
    const auto segments = [](float total, float leading, float trailing) {
        const float fixed = leading + trailing;
        if (fixed > 0.0F && total < fixed) {
            const float scale = std::max(0.0F, total) / fixed;
            leading *= scale;
            trailing *= scale;
        }
        return std::array<float, 3>{
            leading, std::max(0.0F, total - leading - trailing), trailing
        };
    };
    const auto widths = segments(
        rectangle.width,
        static_cast<float>(splits_.left),
        static_cast<float>(splits_.right)
    );
    const auto heights = segments(
        rectangle.height,
        static_cast<float>(splits_.top),
        static_cast<float>(splits_.bottom)
    );

    float destination_y = rectangle.y;
    for (std::size_t row = 0; row < 3; ++row) {
        float destination_x = rectangle.x;
        for (std::size_t column = 0; column < 3; ++column) {
            const auto& region = regions_[row * 3 + column];
            if (region.width() > 0 && region.height() > 0 &&
                widths[column] > 0.0F && heights[row] > 0.0F) {
                painter.draw_region(
                    region,
                    {destination_x, destination_y,
                     widths[column], heights[row]},
                    tint
                );
            }
            destination_x += widths[column];
        }
        destination_y += heights[row];
    }
}

Skin::Skin()
{
    const auto panel = std::make_shared<ColorDrawable>(surface);
    const auto normal = std::make_shared<ColorDrawable>(
        control, Size{0.0F, minimum_touch_size}, Insets{12, 8, 12, 8}
    );
    const auto hover = std::make_shared<ColorDrawable>(
        control_hover, Size{0.0F, minimum_touch_size}, Insets{12, 8, 12, 8}
    );
    const auto active = std::make_shared<ColorDrawable>(
        accent, Size{0.0F, minimum_touch_size}, Insets{12, 8, 12, 8}
    );
    const auto disabled = std::make_shared<ColorDrawable>(
        graphics::Color::from_rgba8(45, 48, 56),
        Size{0.0F, minimum_touch_size},
        Insets{12, 8, 12, 8}
    );

    add_drawable("panel", panel);
    add_drawable("control", normal);
    add_drawable("control.hover", hover);
    add_drawable("control.active", active);
    add_drawable("control.disabled", disabled);
    add_panel_style("default", {panel});
    add_button_style("default", {
        normal, hover, active, disabled, text, muted_text,
        minimum_touch_size, 12.0F
    });
    add_text_field_style("default", {
        normal, active, text, accent, minimum_touch_size, 10.0F
    });
    add_check_box_style("default", {
        normal, active, disabled, text, 8.0F, minimum_touch_size
    });
    add_slider_style("default", {
        normal, active, active, 120.0F, minimum_touch_size
    });
    add_window_style("default", {
        panel, normal, normal, hover, active, text, text,
        graphics::Color::from_rgba8(0, 0, 0, 140),
        Insets{8.0F, 8.0F, 8.0F, 8.0F}, 36.0F, 28.0F, 8.0F
    });
}

void Skin::add_drawable(std::string name, DrawablePtr drawable)
{
    if (name.empty() || !drawable) {
        throw std::invalid_argument("Skin drawable requires a name and value");
    }
    drawables_[std::move(name)] = std::move(drawable);
}

void Skin::add_region_drawable(
    std::string name,
    const graphics2d::TextureRegion& region,
    Insets insets
)
{
    add_drawable(
        std::move(name),
        std::make_shared<RegionDrawable>(region, insets)
    );
}

void Skin::add_nine_patch_drawable(
    std::string name,
    const graphics2d::TextureRegion& region,
    NinePatchSplits splits,
    std::optional<Insets> content_insets
)
{
    add_drawable(
        std::move(name),
        std::make_shared<NinePatchDrawable>(
            region, splits, content_insets
        )
    );
}

void Skin::add_nine_patch_drawable(
    std::string name,
    const graphics2d::AtlasRegion& region
)
{
    if (!region.splits()) {
        throw std::invalid_argument("Atlas region has no nine-patch splits");
    }
    const auto& split = *region.splits();
    std::optional<Insets> insets;
    if (region.pads()) {
        const auto& pad = *region.pads();
        insets = Insets{
            static_cast<float>(pad[0]), static_cast<float>(pad[2]),
            static_cast<float>(pad[1]), static_cast<float>(pad[3])
        };
    }
    add_nine_patch_drawable(
        std::move(name), region.region(),
        NinePatchSplits{split[0], split[2], split[1], split[3]},
        insets
    );
}

DrawablePtr Skin::drawable(std::string_view name) const noexcept
{
    const auto found = drawables_.find(std::string(name));
    return found == drawables_.end() ? DrawablePtr{} : found->second;
}

void Skin::add_panel_style(std::string name, PanelStyle style)
{
    panel_styles_[std::move(name)] = std::move(style);
}

void Skin::add_button_style(std::string name, ButtonStyle style)
{
    button_styles_[std::move(name)] = std::move(style);
}

void Skin::add_text_field_style(std::string name, TextFieldStyle style)
{
    text_field_styles_[std::move(name)] = std::move(style);
}

void Skin::add_check_box_style(std::string name, CheckBoxStyle style)
{
    check_box_styles_[std::move(name)] = std::move(style);
}

void Skin::add_slider_style(std::string name, SliderStyle style)
{
    slider_styles_[std::move(name)] = std::move(style);
}

void Skin::add_window_style(std::string name, WindowStyle style)
{
    window_styles_[std::move(name)] = std::move(style);
}

const PanelStyle& Skin::panel_style(std::string_view name) const
{
    return style_or_default(panel_styles_, name);
}

const ButtonStyle& Skin::button_style(std::string_view name) const
{
    return style_or_default(button_styles_, name);
}

const TextFieldStyle& Skin::text_field_style(std::string_view name) const
{
    return style_or_default(text_field_styles_, name);
}

const CheckBoxStyle& Skin::check_box_style(std::string_view name) const
{
    return style_or_default(check_box_styles_, name);
}

const SliderStyle& Skin::slider_style(std::string_view name) const
{
    return style_or_default(slider_styles_, name);
}

const WindowStyle& Skin::window_style(std::string_view name) const
{
    return style_or_default(window_styles_, name);
}

Size Widget::minimum_size(Painter&, const Skin&) const { return {}; }
Size Widget::preferred_size(Painter&) const { return {width(), height()}; }

Size Widget::maximum_size(Painter&, const Skin&) const
{
    const float infinity = std::numeric_limits<float>::infinity();
    return {infinity, infinity};
}

SizeHints Widget::size_hints(Painter& painter, const Skin& skin) const
{
    SizeHints hints{
        minimum_size(painter, skin),
        preferred_size(painter),
        maximum_size(painter, skin)
    };
    hints.preferred.width = clamp_dimension(
        hints.preferred.width, hints.minimum.width, hints.maximum.width
    );
    hints.preferred.height = clamp_dimension(
        hints.preferred.height, hints.minimum.height, hints.maximum.height
    );
    return hints;
}

void Widget::invalidate_layout() noexcept
{
    layout_valid_ = false;
    if (auto* widget = dynamic_cast<Widget*>(parent())) {
        widget->invalidate_layout();
    }
}

void Widget::validate_layout(Painter& painter, const Skin& skin)
{
    if (layout_valid_) return;
    layout(painter, skin);
    layout_valid_ = true;
}

void Widget::layout(Painter& painter, const Skin& skin)
{
    for (std::size_t index = 0; index < child_count(); ++index) {
        if (auto* child = dynamic_cast<Widget*>(child_at(index))) {
            child->validate_layout(painter, skin);
        }
    }
}

void Widget::paint(Painter&, const Skin&, float, float) const {}
bool Widget::pointer_event(const PointerEvent&) { return false; }
bool Widget::key_down(Key, KeyModifiers) { return false; }
bool Widget::text_input(std::string_view) { return false; }
bool Widget::text_editing(std::string_view, int, int) { return false; }
void Widget::focus_changed(bool) {}
bool Widget::focusable() const noexcept { return false; }

void Widget::input_event(scene2d::InputEvent& event)
{
    bool handled = false;
    switch (event.type) {
    case scene2d::InputType::pointer_move:
    case scene2d::InputType::pointer_down:
    case scene2d::InputType::pointer_up:
    case scene2d::InputType::pointer_cancel: {
        PointerAction action = PointerAction::move;
        if (event.type == scene2d::InputType::pointer_down) {
            action = PointerAction::down;
        } else if (event.type == scene2d::InputType::pointer_up) {
            action = PointerAction::up;
        } else if (event.type == scene2d::InputType::pointer_cancel) {
            action = PointerAction::cancel;
        }
        handled = pointer_event({
            action,
            event.pointer_id,
            event.local_x(),
            event.local_y(),
            event.button
        });
        break;
    }
    case scene2d::InputType::key_down: {
        Key key = Key::escape;
        switch (event.key) {
        case scene2d::InputKey::left: key = Key::left; break;
        case scene2d::InputKey::right: key = Key::right; break;
        case scene2d::InputKey::up: key = Key::up; break;
        case scene2d::InputKey::down: key = Key::down; break;
        case scene2d::InputKey::home: key = Key::home; break;
        case scene2d::InputKey::end: key = Key::end; break;
        case scene2d::InputKey::backspace: key = Key::backspace; break;
        case scene2d::InputKey::delete_key: key = Key::delete_key; break;
        case scene2d::InputKey::enter: key = Key::enter; break;
        case scene2d::InputKey::space: key = Key::space; break;
        case scene2d::InputKey::tab: key = Key::tab; break;
        case scene2d::InputKey::escape: key = Key::escape; break;
        case scene2d::InputKey::unknown: return;
        }
        handled = key_down(key, event.modifiers);
        break;
    }
    default:
        break;
    }
    if (handled) {
        event.handle();
        event.stop();
    }
}

Label::Label(std::string text) : text_(std::move(text))
{
    set_touchable(false);
}

void Label::set_text(std::string text)
{
    text_ = std::move(text);
    invalidate_layout();
}

Size Label::preferred_size(Painter& painter) const
{
    return painter.measure_text(text_);
}

void Label::paint(
    Painter& painter,
    const Skin& skin,
    float x,
    float y
) const
{
    painter.draw_text(text_, x, y, muted_ ? skin.muted_text : skin.text);
}

Image::Image(DrawablePtr drawable) : drawable_(std::move(drawable))
{
    set_touchable(false);
}

void Image::set_drawable(DrawablePtr drawable)
{
    drawable_ = std::move(drawable);
    invalidate_layout();
}

Size Image::preferred_size(Painter&) const
{
    return drawable_ ? drawable_->minimum_size() : Size{};
}

void Image::paint(Painter& painter, const Skin&, float x, float y) const
{
    if (drawable_) drawable_->draw(painter, bounds_of(*this, x, y));
}

Panel::Panel(std::string style) : style_(std::move(style)) {}

void Panel::set_style(std::string style)
{
    style_ = std::move(style);
    invalidate_layout();
}

void Panel::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const auto& style = skin.panel_style(style_);
    if (style.background) {
        style.background->draw(painter, bounds_of(*this, x, y));
    }
}

void Cell::changed() noexcept
{
    if (owner_) owner_->invalidate_layout();
}

Cell& Cell::column_span(std::size_t columns) noexcept
{
    column_span_ = std::max<std::size_t>(1, columns);
    changed();
    return *this;
}

Cell& Cell::grow() noexcept { return grow_x().grow_y(); }
Cell& Cell::grow_x() noexcept { grow_x_ = 1.0F; changed(); return *this; }
Cell& Cell::grow_y() noexcept { grow_y_ = 1.0F; changed(); return *this; }
Cell& Cell::fill() noexcept { return fill_x().fill_y(); }
Cell& Cell::fill_x() noexcept { fill_x_ = true; changed(); return *this; }
Cell& Cell::fill_y() noexcept { fill_y_ = true; changed(); return *this; }

Cell& Cell::pad(float value) noexcept
{
    return pad(Insets{value, value, value, value});
}

Cell& Cell::pad(Insets value) noexcept
{
    padding_ = {
        std::max(0.0F, value.left), std::max(0.0F, value.top),
        std::max(0.0F, value.right), std::max(0.0F, value.bottom)
    };
    changed();
    return *this;
}

Cell& Cell::align(Alignment horizontal, Alignment vertical) noexcept
{
    horizontal_ = horizontal;
    vertical_ = vertical;
    changed();
    return *this;
}

struct Table::GridMetrics {
    std::vector<float> columns;
    std::vector<float> rows;
    float width{0.0F};
    float height{0.0F};
};

Cell& Table::add(std::unique_ptr<Widget> child)
{
    if (!child) throw std::invalid_argument("GUI widget must not be null");
    for (const Cell& existing : cells_) {
        if (existing.row_ == current_row_) {
            current_column_ = std::max(
                current_column_, existing.column_ + existing.column_span_
            );
        }
    }
    Widget* reference = child.get();
    static_cast<void>(add_actor(std::move(child)));
    try {
        cells_.push_back({});
    } catch (...) {
        [[maybe_unused]] auto removed = remove_actor(*reference);
        throw;
    }
    Cell& cell = cells_.back();
    cell.owner_ = this;
    cell.widget_ = reference;
    cell.row_ = current_row_;
    cell.column_ = current_column_++;
    invalidate_layout();
    return cell;
}

Table& Table::row() noexcept
{
    if (current_column_ != 0 || !cells_.empty()) ++current_row_;
    current_column_ = 0;
    invalidate_layout();
    return *this;
}

void Table::set_padding(float padding) noexcept
{
    padding_ = std::max(0.0F, padding);
    invalidate_layout();
}

void Table::set_spacing(float spacing) noexcept
{
    spacing_ = std::max(0.0F, spacing);
    invalidate_layout();
}

Table::GridMetrics Table::measure(
    Painter& painter,
    const Skin* skin,
    bool minimum
) const
{
    const float outer = padding_ < 0.0F
        ? (skin ? skin->padding : default_padding) : padding_;
    const float gap = spacing_ < 0.0F
        ? (skin ? skin->spacing : default_spacing) : spacing_;
    std::size_t column_count = 0;
    std::size_t row_count = 0;
    for (const Cell& cell : cells_) {
        column_count = std::max(column_count, cell.column_ + cell.column_span_);
        row_count = std::max(row_count, cell.row_ + 1);
    }
    GridMetrics result;
    result.columns.assign(column_count, 0.0F);
    result.rows.assign(row_count, 0.0F);

    for (const Cell& cell : cells_) {
        const Size size = minimum
            ? cell.widget_->minimum_size(painter, *skin)
            : cell.widget_->preferred_size(painter);
        const float padded_width = size.width + cell.padding_.left + cell.padding_.right;
        const float padded_height = size.height + cell.padding_.top + cell.padding_.bottom;
        result.rows[cell.row_] = std::max(result.rows[cell.row_], padded_height);
        if (cell.column_span_ == 1) {
            result.columns[cell.column_] = std::max(
                result.columns[cell.column_], padded_width
            );
        }
    }
    for (const Cell& cell : cells_) {
        if (cell.column_span_ == 1) continue;
        const Size size = minimum
            ? cell.widget_->minimum_size(painter, *skin)
            : cell.widget_->preferred_size(painter);
        const float needed = size.width + cell.padding_.left + cell.padding_.right;
        float present = gap * static_cast<float>(cell.column_span_ - 1);
        for (std::size_t column = cell.column_;
             column < cell.column_ + cell.column_span_; ++column) {
            present += result.columns[column];
        }
        const float extra = std::max(0.0F, needed - present) /
            static_cast<float>(cell.column_span_);
        if (extra > 0.0F) {
            for (std::size_t column = cell.column_;
                 column < cell.column_ + cell.column_span_; ++column) {
                result.columns[column] += extra;
            }
        }
    }
    result.width = 2.0F * outer;
    result.height = 2.0F * outer;
    for (float value : result.columns) result.width += value;
    for (float value : result.rows) result.height += value;
    if (!result.columns.empty()) {
        result.width += gap * static_cast<float>(result.columns.size() - 1);
    }
    if (!result.rows.empty()) {
        result.height += gap * static_cast<float>(result.rows.size() - 1);
    }
    return result;
}

Size Table::minimum_size(Painter& painter, const Skin& skin) const
{
    const GridMetrics result = measure(painter, &skin, true);
    return {result.width, result.height};
}

Size Table::preferred_size(Painter& painter) const
{
    const GridMetrics result = measure(painter, nullptr, false);
    return {result.width, result.height};
}

void Table::layout(Painter& painter, const Skin& skin)
{
    GridMetrics metrics = measure(painter, &skin, false);
    const GridMetrics minimum = measure(painter, &skin, true);
    const float outer = padding_ < 0.0F ? skin.padding : padding_;
    const float gap = spacing_ < 0.0F ? skin.spacing : spacing_;
    std::vector<float> column_grow(metrics.columns.size(), 0.0F);
    std::vector<float> row_grow(metrics.rows.size(), 0.0F);
    for (const Cell& cell : cells_) {
        const float share = cell.grow_x_ / static_cast<float>(cell.column_span_);
        for (std::size_t column = cell.column_;
             column < cell.column_ + cell.column_span_; ++column) {
            column_grow[column] = std::max(column_grow[column], share);
        }
        row_grow[cell.row_] = std::max(row_grow[cell.row_], cell.grow_y_);
    }
    const auto fit = [](std::vector<float>& sizes,
                        const std::vector<float>& minimums,
                        const std::vector<float>& weights,
                        float target) {
        float used = 0.0F;
        for (float size : sizes) used += size;
        if (target < used) {
            const float shortage = used - target;
            float room = 0.0F;
            for (std::size_t index = 0; index < sizes.size(); ++index) {
                room += std::max(0.0F, sizes[index] - minimums[index]);
            }
            if (room <= 0.0F) return;
            for (std::size_t index = 0; index < sizes.size(); ++index) {
                const float available = std::max(
                    0.0F, sizes[index] - minimums[index]
                );
                sizes[index] -= std::min(
                    available, shortage * available / room
                );
            }
            return;
        }
        float total = 0.0F;
        for (float weight : weights) total += weight;
        const float extra = target - used;
        if (extra <= 0.0F || total <= 0.0F) return;
        for (std::size_t index = 0; index < sizes.size(); ++index) {
            sizes[index] += extra * weights[index] / total;
        }
    };
    const float column_gaps = metrics.columns.empty() ? 0.0F
        : gap * static_cast<float>(metrics.columns.size() - 1);
    const float row_gaps = metrics.rows.empty() ? 0.0F
        : gap * static_cast<float>(metrics.rows.size() - 1);
    fit(metrics.columns, minimum.columns, column_grow,
        std::max(0.0F, width() - 2.0F * outer - column_gaps));
    fit(metrics.rows, minimum.rows, row_grow,
        std::max(0.0F, height() - 2.0F * outer - row_gaps));

    std::vector<float> column_x(metrics.columns.size(), outer);
    std::vector<float> row_y(metrics.rows.size(), outer);
    for (std::size_t index = 1; index < column_x.size(); ++index) {
        column_x[index] = column_x[index - 1] + metrics.columns[index - 1] + gap;
    }
    for (std::size_t index = 1; index < row_y.size(); ++index) {
        row_y[index] = row_y[index - 1] + metrics.rows[index - 1] + gap;
    }
    for (Cell& cell : cells_) {
        float cell_width = 0.0F;
        for (std::size_t column = cell.column_;
             column < cell.column_ + cell.column_span_; ++column) {
            cell_width += metrics.columns[column];
        }
        cell_width += gap * static_cast<float>(cell.column_span_ - 1);
        const float available_width = std::max(
            0.0F, cell_width - cell.padding_.left - cell.padding_.right
        );
        const float available_height = std::max(
            0.0F, metrics.rows[cell.row_] - cell.padding_.top - cell.padding_.bottom
        );
        const SizeHints hints = cell.widget_->size_hints(painter, skin);
        const float child_width = cell.fill_x_ || cell.grow_x_ > 0.0F
            ? clamp_dimension(available_width, hints.minimum.width, hints.maximum.width)
            : std::min(available_width, hints.preferred.width);
        const float child_height = cell.fill_y_ || cell.grow_y_ > 0.0F
            ? clamp_dimension(available_height, hints.minimum.height, hints.maximum.height)
            : std::min(available_height, hints.preferred.height);
        const auto offset = [](float room, float size, Alignment alignment) {
            if (alignment == Alignment::end) return room - size;
            if (alignment == Alignment::center) return (room - size) * 0.5F;
            return 0.0F;
        };
        cell.widget_->set_bounds(
            column_x[cell.column_] + cell.padding_.left +
                offset(available_width, child_width, cell.horizontal_),
            row_y[cell.row_] + cell.padding_.top +
                offset(available_height, child_height, cell.vertical_),
            child_width,
            child_height
        );
        cell.widget_->invalidate_layout();
        cell.widget_->validate_layout(painter, skin);
    }
}

LinearLayout::LinearLayout(Direction direction) noexcept
    : direction_(direction)
{
}

Widget& LinearLayout::add(std::unique_ptr<Widget> child, float grow)
{
    if (!child) throw std::invalid_argument("GUI widget must not be null");
    Widget* reference = child.get();
    static_cast<void>(add_actor(std::move(child)));
    try {
        slots_.push_back({reference, std::max(0.0F, grow)});
    } catch (...) {
        [[maybe_unused]] auto removed = remove_actor(*reference);
        throw;
    }
    invalidate_layout();
    return *reference;
}

void LinearLayout::set_padding(float padding) noexcept
{
    padding_ = std::max(0.0F, padding);
    invalidate_layout();
}

void LinearLayout::set_spacing(float spacing) noexcept
{
    spacing_ = std::max(0.0F, spacing);
    invalidate_layout();
}

Size LinearLayout::measured_size(
    Painter& painter,
    const Skin* skin,
    bool minimum
) const
{
    const float padding = padding_ < 0.0F
        ? (skin ? skin->padding : default_padding) : padding_;
    const float spacing = spacing_ < 0.0F
        ? (skin ? skin->spacing : default_spacing) : spacing_;
    Size result{padding * 2.0F, padding * 2.0F};
    for (std::size_t index = 0; index < slots_.size(); ++index) {
        const Size child = minimum
            ? slots_[index].widget->minimum_size(painter, *skin)
            : slots_[index].widget->preferred_size(painter);
        if (direction_ == Direction::horizontal) {
            result.width += child.width;
            result.height = std::max(result.height, child.height + padding * 2.0F);
        } else {
            result.width = std::max(result.width, child.width + padding * 2.0F);
            result.height += child.height;
        }
        if (index != 0) {
            if (direction_ == Direction::horizontal) result.width += spacing;
            else result.height += spacing;
        }
    }
    return result;
}

Size LinearLayout::minimum_size(Painter& painter, const Skin& skin) const
{
    return measured_size(painter, &skin, true);
}

Size LinearLayout::preferred_size(Painter& painter) const
{
    return measured_size(painter, nullptr, false);
}

void LinearLayout::layout(Painter& painter, const Skin& skin)
{
    const float padding = padding_ < 0.0F ? skin.padding : padding_;
    const float spacing = spacing_ < 0.0F ? skin.spacing : spacing_;
    const float main_extent = direction_ == Direction::horizontal
        ? width() : height();
    const float cross_extent = direction_ == Direction::horizontal
        ? height() : width();
    const float gaps = slots_.empty()
        ? 0.0F : spacing * static_cast<float>(slots_.size() - 1);
    const float available = std::max(0.0F, main_extent - 2.0F * padding - gaps);

    std::vector<SizeHints> hints;
    std::vector<float> main_sizes;
    hints.reserve(slots_.size());
    main_sizes.reserve(slots_.size());
    float used = 0.0F;
    float total_grow = 0.0F;
    for (const Slot& slot : slots_) {
        hints.push_back(slot.widget->size_hints(painter, skin));
        const Size& preferred = hints.back().preferred;
        const float value = direction_ == Direction::horizontal
            ? preferred.width : preferred.height;
        main_sizes.push_back(value);
        used += value;
        total_grow += slot.grow;
    }

    if (used > available && used > 0.0F) {
        const float shortage = used - available;
        float shrink_room = 0.0F;
        for (std::size_t index = 0; index < slots_.size(); ++index) {
            const float minimum = direction_ == Direction::horizontal
                ? hints[index].minimum.width : hints[index].minimum.height;
            shrink_room += std::max(0.0F, main_sizes[index] - minimum);
        }
        for (std::size_t index = 0; index < slots_.size(); ++index) {
            const float minimum = direction_ == Direction::horizontal
                ? hints[index].minimum.width : hints[index].minimum.height;
            const float room = std::max(0.0F, main_sizes[index] - minimum);
            const float reduction = shrink_room > 0.0F
                ? std::min(room, shortage * room / shrink_room)
                : 0.0F;
            main_sizes[index] -= reduction;
        }
    } else if (available > used && total_grow > 0.0F) {
        const float extra = available - used;
        for (std::size_t index = 0; index < slots_.size(); ++index) {
            const float maximum = direction_ == Direction::horizontal
                ? hints[index].maximum.width : hints[index].maximum.height;
            main_sizes[index] = std::min(
                maximum,
                main_sizes[index] + extra * slots_[index].grow / total_grow
            );
        }
    }

    float cursor = padding;
    for (std::size_t index = 0; index < slots_.size(); ++index) {
        Widget& child = *slots_[index].widget;
        const float cross_minimum = direction_ == Direction::horizontal
            ? hints[index].minimum.height : hints[index].minimum.width;
        const float cross_maximum = direction_ == Direction::horizontal
            ? hints[index].maximum.height : hints[index].maximum.width;
        const float cross_size = clamp_dimension(
            std::max(0.0F, cross_extent - 2.0F * padding),
            cross_minimum,
            cross_maximum
        );
        if (direction_ == Direction::horizontal) {
            child.set_bounds(cursor, padding, main_sizes[index], cross_size);
        } else {
            child.set_bounds(padding, cursor, cross_size, main_sizes[index]);
        }
        child.invalidate_layout();
        child.validate_layout(painter, skin);
        cursor += main_sizes[index] + spacing;
    }
}

Widget& Stack::add(std::unique_ptr<Widget> child)
{
    if (!child) throw std::invalid_argument("GUI widget must not be null");
    Widget& result = static_cast<Widget&>(add_actor(std::move(child)));
    invalidate_layout();
    return result;
}

Size Stack::preferred_size(Painter& painter) const
{
    Size result{};
    for (std::size_t index = 0; index < child_count(); ++index) {
        if (const auto* child = dynamic_cast<const Widget*>(child_at(index))) {
            const Size size = child->preferred_size(painter);
            result.width = std::max(result.width, size.width);
            result.height = std::max(result.height, size.height);
        }
    }
    return result;
}

void Stack::layout(Painter& painter, const Skin& skin)
{
    for (std::size_t index = 0; index < child_count(); ++index) {
        if (auto* child = dynamic_cast<Widget*>(child_at(index))) {
            child->set_bounds(0.0F, 0.0F, width(), height());
            child->invalidate_layout();
            child->validate_layout(painter, skin);
        }
    }
}

MarginContainer::MarginContainer(Insets margin) noexcept : margin_(margin) {}

Widget& MarginContainer::set_content(std::unique_ptr<Widget> content)
{
    if (!content) throw std::invalid_argument("GUI content must not be null");
    clear();
    content_ = content.get();
    static_cast<void>(add_actor(std::move(content)));
    invalidate_layout();
    return *content_;
}

Size MarginContainer::preferred_size(Painter& painter) const
{
    const Size child = content_ ? content_->preferred_size(painter) : Size{};
    return {
        child.width + margin_.left + margin_.right,
        child.height + margin_.top + margin_.bottom
    };
}

void MarginContainer::layout(Painter& painter, const Skin& skin)
{
    if (!content_) return;
    content_->set_bounds(
        margin_.left,
        margin_.top,
        std::max(0.0F, width() - margin_.left - margin_.right),
        std::max(0.0F, height() - margin_.top - margin_.bottom)
    );
    content_->invalidate_layout();
    content_->validate_layout(painter, skin);
}

Widget& ScrollPane::set_content(std::unique_ptr<Widget> content)
{
    if (!content) throw std::invalid_argument("GUI content must not be null");
    clear();
    content_ = content.get();
    static_cast<void>(add_actor(std::move(content)));
    invalidate_layout();
    return *content_;
}

void ScrollPane::set_scroll_y(float scroll_y) noexcept
{
    scroll_y_ = std::max(0.0F, scroll_y);
    clamp_scroll();
    if (content_) content_->set_position(0.0F, -scroll_y_);
}

Size ScrollPane::preferred_size(Painter& painter) const
{
    return content_ ? content_->preferred_size(painter) : Size{};
}

void ScrollPane::layout(Painter& painter, const Skin& skin)
{
    if (!content_) return;
    const Size preferred = content_->preferred_size(painter);
    content_->set_bounds(
        0.0F,
        -scroll_y_,
        std::max(width(), preferred.width),
        std::max(height(), preferred.height)
    );
    clamp_scroll();
    content_->set_position(0.0F, -scroll_y_);
    content_->invalidate_layout();
    content_->validate_layout(painter, skin);
}

bool ScrollPane::pointer_event(const PointerEvent& event)
{
    if (!enabled()) return false;
    if (event.action == PointerAction::down) {
        drag_pointer_ = event.pointer_id;
        last_pointer_y_ = event.y;
        return true;
    }
    if (!drag_pointer_ || *drag_pointer_ != event.pointer_id) return false;
    if (event.action == PointerAction::move) {
        set_scroll_y(scroll_y_ + last_pointer_y_ - event.y);
        last_pointer_y_ = event.y;
        return true;
    }
    drag_pointer_.reset();
    return true;
}

void ScrollPane::clamp_scroll() noexcept
{
    const float maximum = content_
        ? std::max(0.0F, content_->height() - height()) : 0.0F;
    scroll_y_ = std::clamp(scroll_y_, 0.0F, maximum);
}

Button::Button(std::string text, Callback callback)
    : text_(std::move(text)), callback_(std::move(callback))
{
}

void Button::set_text(std::string text)
{
    text_ = std::move(text);
    invalidate_layout();
}

void Button::set_on_click(Callback callback)
{
    callback_ = std::move(callback);
}

void Button::set_style(std::string style)
{
    style_ = std::move(style);
    invalidate_layout();
}

Size Button::minimum_size(Painter& painter, const Skin& skin) const
{
    const auto& style = skin.button_style(style_);
    const Size text = painter.measure_text(text_);
    return {
        text.width + 2.0F * style.horizontal_padding,
        std::max(skin.minimum_touch_size, style.minimum_height)
    };
}

Size Button::preferred_size(Painter& painter) const
{
    const Size text = painter.measure_text(text_);
    return {text.width + 24.0F, default_touch_size};
}

void Button::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const auto& style = skin.button_style(style_);
    DrawablePtr background;
    if (!enabled()) background = style.disabled;
    else if (pressed_ || selected()) background = style.pressed;
    else if (hovered_) background = style.hovered;
    else background = style.normal;
    if (background) background->draw(painter, bounds_of(*this, x, y));
    if (focused_) {
        painter.stroke_rectangle(bounds_of(*this, x, y), skin.accent, 2.0F);
    }
    const Size text_size = painter.measure_text(text_);
    painter.draw_text(
        text_,
        x + std::max(0.0F, (width() - text_size.width) * 0.5F),
        y + std::max(0.0F, (height() - text_size.height) * 0.5F),
        enabled() ? style.text : style.disabled_text
    );
}

bool Button::pointer_event(const PointerEvent& event)
{
    if (!enabled()) return false;
    const bool inside = contains(event.x, event.y);
    if (event.action == PointerAction::move) {
        hovered_ = inside;
        return pressed_;
    }
    if (event.action == PointerAction::down) {
        pressed_ = inside;
        hovered_ = inside;
        return pressed_;
    }
    if (event.action == PointerAction::up) {
        const bool clicked = pressed_ && inside;
        pressed_ = false;
        hovered_ = inside;
        if (clicked) activate();
        return clicked;
    }
    pressed_ = false;
    hovered_ = false;
    return true;
}

bool Button::key_down(Key key, KeyModifiers)
{
    if (!enabled() || (key != Key::enter && key != Key::space)) return false;
    activate();
    return true;
}

bool Button::focusable() const noexcept { return enabled(); }
void Button::focus_changed(bool focused) { focused_ = focused; }
void Button::activate() { if (callback_) callback_(); }
bool Button::selected() const noexcept { return false; }

ToggleButton::ToggleButton(std::string text, bool checked)
    : Button(std::move(text)), checked_(checked)
{
}

void ToggleButton::set_checked(bool checked)
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

CheckBox::CheckBox(std::string text, bool checked)
    : ToggleButton(std::move(text), checked)
{
}

void CheckBox::set_check_style(std::string style)
{
    check_style_ = std::move(style);
    invalidate_layout();
}

Size CheckBox::minimum_size(Painter& painter, const Skin& skin) const
{
    const auto& style = skin.check_box_style(check_style_);
    const Size text_size = painter.measure_text(text());
    return {
        style.minimum_touch_size + style.spacing + text_size.width,
        std::max(style.minimum_touch_size, text_size.height)
    };
}

Size CheckBox::preferred_size(Painter& painter) const
{
    const Size text_size = painter.measure_text(text());
    return {default_touch_size + 8.0F + text_size.width, default_touch_size};
}

void CheckBox::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const auto& style = skin.check_box_style(check_style_);
    DrawablePtr mark = !enabled()
        ? style.disabled : (checked() ? style.checked : style.unchecked);
    const float size = std::min(height(), style.minimum_touch_size);
    if (mark) {
        mark->draw(painter, {x, y + (height() - size) * 0.5F, size, size});
    }
    const Size text_size = painter.measure_text(text());
    painter.draw_text(
        text(),
        x + size + style.spacing,
        y + std::max(0.0F, (height() - text_size.height) * 0.5F),
        style.text
    );
    if (focused()) {
        painter.stroke_rectangle(bounds_of(*this, x, y), skin.accent, 2.0F);
    }
}

TextField::TextField(std::string text) : text_(std::move(text))
{
    cursor_ = text_.size();
}

void TextField::set_text(std::string text)
{
    text_ = std::move(text);
    cursor_ = text_.size();
    invalidate_layout();
}

void TextField::set_style(std::string style)
{
    style_ = std::move(style);
    invalidate_layout();
}

Size TextField::minimum_size(Painter&, const Skin& skin) const
{
    const auto& style = skin.text_field_style(style_);
    return {80.0F, std::max(skin.minimum_touch_size, style.minimum_height)};
}

Size TextField::preferred_size(Painter& painter) const
{
    const Size text_size = painter.measure_text(text_.empty() ? "M" : text_);
    return {std::max(120.0F, text_size.width + 20.0F), default_touch_size};
}

void TextField::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const auto& style = skin.text_field_style(style_);
    const DrawablePtr background = focused_ ? style.focused : style.normal;
    if (background) background->draw(painter, bounds_of(*this, x, y));
    const float text_y = y + std::max(
        0.0F, (height() - painter.measure_text("M").height) * 0.5F
    );
    painter.draw_text(text_, x + style.horizontal_padding, text_y, style.text);
    if (focused_) {
        const Size prefix = painter.measure_text(
            std::string_view(text_).substr(0, cursor_)
        );
        if (!composition_.empty()) {
            const float composition_x = x + style.horizontal_padding + prefix.width;
            painter.draw_text(composition_, composition_x, text_y, style.cursor);
            const Size composition_size = painter.measure_text(composition_);
            painter.fill_rectangle(
                {composition_x, text_y + composition_size.height - 1.0F,
                 composition_size.width, 1.0F},
                style.cursor
            );
        }
        painter.fill_rectangle(
            {x + style.horizontal_padding + prefix.width,
             text_y, 1.0F, prefix.height},
            style.cursor
        );
    }
}

bool TextField::pointer_event(const PointerEvent& event)
{
    if (!enabled() || event.action != PointerAction::down) return false;
    cursor_ = text_.size();
    return contains(event.x, event.y);
}

bool TextField::key_down(Key key, KeyModifiers)
{
    if (!enabled()) return false;
    switch (key) {
    case Key::left: cursor_ = previous_codepoint(text_, cursor_); return true;
    case Key::right: cursor_ = next_codepoint(text_, cursor_); return true;
    case Key::home: cursor_ = 0; return true;
    case Key::end: cursor_ = text_.size(); return true;
    case Key::backspace:
        if (cursor_ > 0) {
            const std::size_t previous = previous_codepoint(text_, cursor_);
            text_.erase(previous, cursor_ - previous);
            cursor_ = previous;
        }
        invalidate_layout();
        return true;
    case Key::delete_key:
        if (cursor_ < text_.size()) {
            text_.erase(cursor_, next_codepoint(text_, cursor_) - cursor_);
        }
        invalidate_layout();
        return true;
    default: return false;
    }
}

bool TextField::text_input(std::string_view text)
{
    if (!enabled() || text.empty()) return false;
    composition_.clear();
    text_.insert(cursor_, text);
    cursor_ += text.size();
    invalidate_layout();
    return true;
}

bool TextField::text_editing(std::string_view text, int, int)
{
    if (!enabled()) return false;
    composition_.assign(text);
    return true;
}

void TextField::focus_changed(bool focused)
{
    focused_ = focused;
    if (!focused) composition_.clear();
}
bool TextField::focusable() const noexcept { return enabled(); }

Slider::Slider(float minimum, float maximum, float value)
{
    set_range(minimum, maximum);
    value_ = std::clamp(value, minimum_, maximum_);
}

void Slider::set_range(float minimum, float maximum) noexcept
{
    minimum_ = std::min(minimum, maximum);
    maximum_ = std::max(minimum, maximum);
    value_ = std::clamp(value_, minimum_, maximum_);
}

void Slider::set_value(float value)
{
    float adjusted = std::clamp(value, minimum_, maximum_);
    if (step_ > 0.0F) {
        adjusted = minimum_ + std::round((adjusted - minimum_) / step_) * step_;
        adjusted = std::clamp(adjusted, minimum_, maximum_);
    }
    if (adjusted == value_) return;
    value_ = adjusted;
    if (callback_) callback_(value_);
}

void Slider::set_step(float step) noexcept { step_ = std::max(0.0F, step); }
void Slider::set_on_change(ChangeCallback callback) { callback_ = std::move(callback); }
void Slider::set_style(std::string style) { style_ = std::move(style); }

Size Slider::minimum_size(Painter&, const Skin& skin) const
{
    const auto& style = skin.slider_style(style_);
    return {style.minimum_length, std::max(style.minimum_touch_size, skin.minimum_touch_size)};
}

Size Slider::preferred_size(Painter&) const
{
    return {120.0F, default_touch_size};
}

void Slider::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const auto& style = skin.slider_style(style_);
    const float ratio = maximum_ > minimum_
        ? (value_ - minimum_) / (maximum_ - minimum_) : 0.0F;
    const float track_height = std::min(8.0F, height());
    const Rectangle track{x, y + (height() - track_height) * 0.5F, width(), track_height};
    if (style.track) style.track->draw(painter, track);
    if (style.filled_track) {
        style.filled_track->draw(painter, {track.x, track.y, track.width * ratio, track.height});
    }
    const float knob_size = std::min(height(), style.minimum_touch_size * 0.6F);
    if (style.knob) {
        style.knob->draw(
            painter,
            {x + ratio * std::max(0.0F, width() - knob_size),
             y + (height() - knob_size) * 0.5F,
             knob_size,
             knob_size}
        );
    }
    if (focused_) {
        painter.stroke_rectangle(bounds_of(*this, x, y), skin.accent, 2.0F);
    }
}

bool Slider::pointer_event(const PointerEvent& event)
{
    if (!enabled()) return false;
    if (event.action == PointerAction::down && contains(event.x, event.y)) {
        drag_pointer_ = event.pointer_id;
        update_from_pointer(event.x);
        return true;
    }
    if (!drag_pointer_ || *drag_pointer_ != event.pointer_id) return false;
    if (event.action == PointerAction::move) {
        update_from_pointer(event.x);
        return true;
    }
    drag_pointer_.reset();
    if (event.action == PointerAction::up) update_from_pointer(event.x);
    return true;
}

bool Slider::key_down(Key key, KeyModifiers)
{
    if (!enabled() || (key != Key::left && key != Key::right)) return false;
    const float amount = step_ > 0.0F
        ? step_
        : std::max(0.01F, (maximum_ - minimum_) * 0.05F);
    set_value(value_ + (key == Key::left ? -amount : amount));
    return true;
}

bool Slider::focusable() const noexcept { return enabled(); }
void Slider::focus_changed(bool focused) { focused_ = focused; }

void Slider::update_from_pointer(float x)
{
    const float ratio = width() > 0.0F ? std::clamp(x / width(), 0.0F, 1.0F) : 0.0F;
    set_value(minimum_ + ratio * (maximum_ - minimum_));
}

Separator::Separator(Direction direction) noexcept : direction_(direction)
{
    set_touchable(false);
}

Size Separator::preferred_size(Painter&) const
{
    return direction_ == Direction::horizontal
        ? Size{0.0F, 1.0F} : Size{1.0F, 0.0F};
}

void Separator::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    painter.fill_rectangle(bounds_of(*this, x, y), skin.border);
}

Window::Window(std::string title, std::string style)
    : title_(std::move(title)), style_(std::move(style))
{
    auto content = std::make_unique<Table>();
    content_ = content.get();
    static_cast<void>(add_actor(std::move(content)));
}

void Window::set_title(std::string title)
{
    title_ = std::move(title);
    invalidate_layout();
}

void Window::set_style(std::string style)
{
    style_ = std::move(style);
    invalidate_layout();
}

void Window::set_closable(bool closable) noexcept
{
    if (closable_ == closable) return;
    closable_ = closable;
    invalidate_layout();
}

void Window::set_minimum_window_size(Size size) noexcept
{
    requested_minimum_ = {
        std::max(0.0F, size.width), std::max(0.0F, size.height)
    };
    invalidate_layout();
}

Size Window::minimum_size(Painter& painter, const Skin& skin) const
{
    const WindowStyle& style = skin.window_style(style_);
    const Size table = content_->minimum_size(painter, skin);
    const Size title = painter.measure_text(title_);
    const Size background = style.background
        ? style.background->minimum_size() : Size{};
    const float title_controls = closable_
        ? std::max(0.0F, style.close_size) + style.content_insets.right
        : 0.0F;
    return {
        std::max({
            table.width + style.content_insets.left + style.content_insets.right,
            title.width + style.content_insets.left + title_controls +
                style.content_insets.right,
            background.width,
            requested_minimum_.width
        }),
        std::max({
            table.height + style.title_height + style.content_insets.top +
                style.content_insets.bottom,
            background.height,
            requested_minimum_.height
        })
    };
}

Size Window::preferred_size(Painter& painter) const
{
    const Size table = content_->preferred_size(painter);
    const Size title = painter.measure_text(title_);
    const float title_controls = closable_ ? close_size_ + 8.0F : 0.0F;
    return {
        std::max({table.width + 16.0F, title.width + 24.0F + title_controls,
                  requested_minimum_.width}),
        std::max(table.height + title_height_ + 16.0F,
                 requested_minimum_.height)
    };
}

void Window::layout(Painter& painter, const Skin& skin)
{
    const WindowStyle& style = skin.window_style(style_);
    title_height_ = std::max(style.title_height, painter.measure_text(title_).height);
    close_size_ = std::min(title_height_, std::max(0.0F, style.close_size));
    resize_border_ = std::max(1.0F, style.resize_border);
    measured_minimum_ = minimum_size(painter, skin);
    if (width() < measured_minimum_.width || height() < measured_minimum_.height) {
        set_size(
            std::max(width(), measured_minimum_.width),
            std::max(height(), measured_minimum_.height)
        );
        constrain_to_parent();
    }
    content_->set_bounds(
        style.content_insets.left,
        title_height_ + style.content_insets.top,
        std::max(0.0F, width() - style.content_insets.left - style.content_insets.right),
        std::max(0.0F, height() - title_height_ - style.content_insets.top -
            style.content_insets.bottom)
    );
    content_->invalidate_layout();
    content_->validate_layout(painter, skin);
}

void Window::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const WindowStyle& style = skin.window_style(style_);
    if (style.background) style.background->draw(painter, bounds_of(*this, x, y));
    if (style.title_background) {
        style.title_background->draw(painter, {x, y, width(), title_height_});
    }
    const Size title_size = painter.measure_text(title_);
    painter.draw_text(
        title_, x + style.content_insets.left,
        y + std::max(0.0F, (title_height_ - title_size.height) * 0.5F),
        style.title_text
    );
    if (closable_) {
        const Rectangle bounds = close_bounds();
        DrawablePtr background = close_pressed_ ? style.close_pressed
            : (close_hovered_ ? style.close_hovered : style.close_normal);
        if (background) {
            background->draw(
                painter,
                {x + bounds.x, y + bounds.y, bounds.width, bounds.height}
            );
        }
        const Size close_text = painter.measure_text("x");
        painter.draw_text(
            "x",
            x + bounds.x + std::max(0.0F, (bounds.width - close_text.width) * 0.5F),
            y + bounds.y + std::max(0.0F, (bounds.height - close_text.height) * 0.5F),
            style.close_text
        );
    }
}

bool Window::pointer_event(const PointerEvent& event)
{
    if (!enabled()) return false;
    const bool inside = contains(event.x, event.y);
    if (event.action == PointerAction::down) {
        const Rectangle close = close_bounds();
        const bool over_close = closable_ &&
            event.x >= close.x && event.x <= close.x + close.width &&
            event.y >= close.y && event.y <= close.y + close.height;
        if (over_close) {
            interaction_pointer_ = event.pointer_id;
            close_pressed_ = true;
            close_hovered_ = true;
            return true;
        }
        const unsigned int edges = resizable_
            ? resize_edges(event.x, event.y) : resize_none;
        if (inside && edges != resize_none) {
            interaction_pointer_ = event.pointer_id;
            resize_edges_ = edges;
            resize_start_ = {x(), y(), width(), height()};
            pointer_start_x_ = x() + event.x;
            pointer_start_y_ = y() + event.y;
            return true;
        }
        if (movable_ && inside && event.y <= title_height_) {
            interaction_pointer_ = event.pointer_id;
            drag_offset_x_ = event.x;
            drag_offset_y_ = event.y;
            return true;
        }
        return inside || modal_;
    }
    if (interaction_pointer_ && *interaction_pointer_ == event.pointer_id) {
        const Rectangle close = close_bounds();
        const bool over_close = event.x >= close.x && event.x <= close.x + close.width &&
            event.y >= close.y && event.y <= close.y + close.height;
        if (close_pressed_) {
            close_hovered_ = over_close;
            if (event.action == PointerAction::up ||
                event.action == PointerAction::cancel) {
                const bool close_window = event.action == PointerAction::up && over_close;
                close_pressed_ = false;
                interaction_pointer_.reset();
                if (close_window) request_close();
            }
            return true;
        }
        if (resize_edges_ != resize_none) {
            if (event.action == PointerAction::move) {
                resize_from_pointer(x() + event.x, y() + event.y);
            } else {
                resize_edges_ = resize_none;
                interaction_pointer_.reset();
            }
            return true;
        }
        if (event.action == PointerAction::move) {
            set_position(
                x() + event.x - drag_offset_x_,
                y() + event.y - drag_offset_y_
            );
            constrain_to_parent();
        } else {
            interaction_pointer_.reset();
        }
        return true;
    }
    if (event.action == PointerAction::move && closable_) {
        const Rectangle close = close_bounds();
        close_hovered_ = event.x >= close.x && event.x <= close.x + close.width &&
            event.y >= close.y && event.y <= close.y + close.height;
    }
    return inside || modal_;
}

Rectangle Window::close_bounds() const noexcept
{
    const float size = std::min(title_height_, close_size_);
    return {
        std::max(0.0F, width() - size - 4.0F),
        std::max(0.0F, (title_height_ - size) * 0.5F),
        size,
        size
    };
}

unsigned int Window::resize_edges(float local_x, float local_y) const noexcept
{
    if (!contains(local_x, local_y)) return resize_none;
    unsigned int result = resize_none;
    if (local_x <= resize_border_) result |= resize_left;
    if (local_x >= width() - resize_border_) result |= resize_right;
    if (local_y <= resize_border_) result |= resize_top;
    if (local_y >= height() - resize_border_) result |= resize_bottom;
    return result;
}

void Window::constrain_to_parent() noexcept
{
    const scene2d::Group* owner = parent();
    if (!owner) return;
    const float minimum_width = std::max(
        requested_minimum_.width, measured_minimum_.width
    );
    const float minimum_height = std::max(
        requested_minimum_.height, measured_minimum_.height
    );
    const float constrained_width = owner->width() >= minimum_width
        ? std::min(width(), owner->width()) : width();
    const float constrained_height = owner->height() >= minimum_height
        ? std::min(height(), owner->height()) : height();
    if (constrained_width != width() || constrained_height != height()) {
        set_size(constrained_width, constrained_height);
        invalidate_layout();
    }
    set_position(
        std::clamp(x(), 0.0F, std::max(0.0F, owner->width() - width())),
        std::clamp(y(), 0.0F, std::max(0.0F, owner->height() - height()))
    );
}

void Window::resize_from_pointer(float stage_x, float stage_y) noexcept
{
    const float delta_x = stage_x - pointer_start_x_;
    const float delta_y = stage_y - pointer_start_y_;
    float left = resize_start_.x;
    float top = resize_start_.y;
    float right = resize_start_.x + resize_start_.width;
    float bottom = resize_start_.y + resize_start_.height;
    if ((resize_edges_ & resize_left) != 0U) left += delta_x;
    if ((resize_edges_ & resize_right) != 0U) right += delta_x;
    if ((resize_edges_ & resize_top) != 0U) top += delta_y;
    if ((resize_edges_ & resize_bottom) != 0U) bottom += delta_y;

    const float minimum_width = std::max(
        requested_minimum_.width, measured_minimum_.width
    );
    const float minimum_height = std::max(
        requested_minimum_.height, measured_minimum_.height
    );
    if (right - left < minimum_width) {
        if ((resize_edges_ & resize_left) != 0U) left = right - minimum_width;
        else right = left + minimum_width;
    }
    if (bottom - top < minimum_height) {
        if ((resize_edges_ & resize_top) != 0U) top = bottom - minimum_height;
        else bottom = top + minimum_height;
    }

    if (const scene2d::Group* owner = parent()) {
        if ((resize_edges_ & resize_left) != 0U && left < 0.0F) left = 0.0F;
        if ((resize_edges_ & resize_top) != 0U && top < 0.0F) top = 0.0F;
        if ((resize_edges_ & resize_right) != 0U && right > owner->width()) {
            right = owner->width();
        }
        if ((resize_edges_ & resize_bottom) != 0U && bottom > owner->height()) {
            bottom = owner->height();
        }
    }
    set_bounds(left, top, std::max(0.0F, right - left),
               std::max(0.0F, bottom - top));
    invalidate_layout();
}

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

Ui::Ui(float width, float height, Skin skin)
    : stage_(width, height), skin_(std::move(skin))
{
}

Widget& Ui::set_content(std::unique_ptr<Widget> content)
{
    if (!content) throw std::invalid_argument("GUI content must not be null");
    clear_focus();
    captures_.clear();
    overlays_.clear();
    stage_.root().clear();
    content_ = content.get();
    content_->set_bounds(0.0F, 0.0F, stage_.root().width(), stage_.root().height());
    static_cast<void>(stage_.add_actor(std::move(content)));
    return *content_;
}

Window& Ui::show_window(std::unique_ptr<Window> window, bool center)
{
    if (!window) throw std::invalid_argument("GUI window must not be null");
    Window* reference = window.get();
    overlays_.push_back({reference, focused_, center});
    try {
        static_cast<void>(stage_.add_actor(std::move(window)));
    } catch (...) {
        overlays_.pop_back();
        throw;
    }
    captures_.clear();
    if (reference->modal()) {
        clear_focus();
        static_cast<void>(focus_next(false));
    }
    return *reference;
}

Dialog& Ui::show_dialog(std::unique_ptr<Dialog> dialog, bool center)
{
    if (!dialog) throw std::invalid_argument("GUI dialog must not be null");
    Dialog* reference = dialog.get();
    static_cast<void>(show_window(std::move(dialog), center));
    return *reference;
}

void Ui::close_window(Window& window)
{
    const auto found = std::find_if(
        overlays_.begin(), overlays_.end(),
        [&window](const Overlay& overlay) { return overlay.window == &window; }
    );
    if (found == overlays_.end()) return;
    Widget* restore = found->previous_focus;
    for (Overlay& overlay : overlays_) {
        if (is_descendant_of(overlay.previous_focus, &window)) {
            overlay.previous_focus = restore;
        }
    }
    if (is_descendant_of(focused_, &window)) clear_focus();
    for (auto capture = captures_.begin(); capture != captures_.end();) {
        if (is_descendant_of(capture->second, &window)) capture = captures_.erase(capture);
        else ++capture;
    }
    [[maybe_unused]] auto removed = stage_.root().remove_actor(window);
    overlays_.erase(found);
    if (!focused_ && restore && restore->focusable()) set_focus(restore);
    if (!focused_ && top_modal()) static_cast<void>(focus_next(false));
}

void Ui::resize(float width, float height)
{
    stage_.resize(width, height);
    if (content_) {
        content_->set_bounds(0.0F, 0.0F, width, height);
        content_->invalidate_layout();
    }
    for (Overlay& overlay : overlays_) {
        overlay.window->constrain_to_parent();
    }
}

void Ui::update(double delta_seconds)
{
    stage_.act(delta_seconds);
    prune_closed_windows();
}

void Ui::layout(Painter& painter)
{
    if (content_) content_->validate_layout(painter, skin_);
    for (Overlay& overlay : overlays_) {
        Window& window = *overlay.window;
        if (window.width() <= 0.0F || window.height() <= 0.0F) {
            const Size size = window.size_hints(painter, skin_).preferred;
            window.set_size(size.width, size.height);
            window.invalidate_layout();
        }
        if (overlay.center_pending) {
            window.set_position(
                std::max(0.0F, (stage_.root().width() - window.width()) * 0.5F),
                std::max(0.0F, (stage_.root().height() - window.height()) * 0.5F)
            );
            overlay.center_pending = false;
        }
        window.constrain_to_parent();
        window.validate_layout(painter, skin_);
    }
    if (text_input_service_ && text_input_service_->active() &&
        dynamic_cast<TextField*>(focused_)) {
        float stage_x = 0.0F;
        float stage_y = 0.0F;
        for (const scene2d::Actor* current = focused_; current;
             current = current->parent()) {
            stage_x += current->x();
            stage_y += current->y();
        }
        text_input_service_->update_area(
            {stage_x, stage_y, focused_->width(), focused_->height()}
        );
    }
}

void Ui::paint(Painter& painter) const
{
    const Window* modal = top_modal();
    for (std::size_t index = 0; index < stage_.root().child_count(); ++index) {
        const scene2d::Actor* child = stage_.root().child_at(index);
        if (child == modal) {
            const WindowStyle& style = skin_.window_style(modal->style_);
            painter.fill_rectangle(
                {0.0F, 0.0F, stage_.root().width(), stage_.root().height()},
                style.modal_overlay
            );
        }
        paint_tree(*child, painter, skin_, 0.0F, 0.0F);
    }
}

bool Ui::event(const application::Event& event)
{
    switch (event.type) {
    case application::Event::Type::PointerDown:
        return pointer(PointerAction::down, event.x, event.y, 0, event.pointer_id);
    case application::Event::Type::PointerMove:
        return pointer(PointerAction::move, event.x, event.y, 0, event.pointer_id);
    case application::Event::Type::PointerUp:
        return pointer(PointerAction::up, event.x, event.y, 0, event.pointer_id);
    case application::Event::Type::Resize:
        resize(static_cast<float>(event.width), static_cast<float>(event.height));
        return true;
    case application::Event::Type::TextInput:
        return text_input(event.text);
    case application::Event::Type::TextEditing:
        return text_editing(
            event.text, event.editing_start, event.editing_length
        );
    case application::Event::Type::TextInputHidden:
        return text_editing({}, 0, 0);
    case application::Event::Type::KeyDown: {
        using AppKey = application::Event::Key;
        KeyModifiers modifiers{
            event.modifiers.contains(application::KeyModifier::shift),
            event.modifiers.contains(application::KeyModifier::control),
            event.modifiers.contains(application::KeyModifier::alt),
            event.modifiers.contains(application::KeyModifier::meta)
        };
        switch (event.key) {
        case AppKey::left: return key_down(Key::left, modifiers);
        case AppKey::right: return key_down(Key::right, modifiers);
        case AppKey::up: return key_down(Key::up, modifiers);
        case AppKey::down: return key_down(Key::down, modifiers);
        case AppKey::home: return key_down(Key::home, modifiers);
        case AppKey::end: return key_down(Key::end, modifiers);
        case AppKey::backspace: return key_down(Key::backspace, modifiers);
        case AppKey::delete_key: return key_down(Key::delete_key, modifiers);
        case AppKey::enter: return key_down(Key::enter, modifiers);
        case AppKey::space: return key_down(Key::space, modifiers);
        case AppKey::tab: return key_down(Key::tab, modifiers);
        case AppKey::escape: return key_down(Key::escape, modifiers);
        default: return false;
        }
    }
    case application::Event::Type::KeyUp: {
        using AppKey = application::Event::Key;
        KeyModifiers modifiers{
            event.modifiers.contains(application::KeyModifier::shift),
            event.modifiers.contains(application::KeyModifier::control),
            event.modifiers.contains(application::KeyModifier::alt),
            event.modifiers.contains(application::KeyModifier::meta)
        };
        switch (event.key) {
        case AppKey::left: return key_up(Key::left, modifiers);
        case AppKey::right: return key_up(Key::right, modifiers);
        case AppKey::up: return key_up(Key::up, modifiers);
        case AppKey::down: return key_up(Key::down, modifiers);
        case AppKey::home: return key_up(Key::home, modifiers);
        case AppKey::end: return key_up(Key::end, modifiers);
        case AppKey::backspace: return key_up(Key::backspace, modifiers);
        case AppKey::delete_key: return key_up(Key::delete_key, modifiers);
        case AppKey::enter: return key_up(Key::enter, modifiers);
        case AppKey::space: return key_up(Key::space, modifiers);
        case AppKey::tab: return key_up(Key::tab, modifiers);
        case AppKey::escape: return key_up(Key::escape, modifiers);
        default: return false;
        }
    }
    case application::Event::Type::NavigationInput:
        return navigation(event.navigation, event.input_device_id);
    default:
        return false;
    }
}

void Ui::set_text_input_service(
    application::TextInputService* service
) noexcept
{
    if (text_input_service_ && text_input_service_->active()) {
        text_input_service_->stop();
    }
    text_input_service_ = service;
}

bool Ui::pointer(
    PointerAction action,
    float x,
    float y,
    int button,
    std::int64_t pointer_id
)
{
    Widget* target = nullptr;
    const auto capture = captures_.find(pointer_id);
    if (capture != captures_.end()) target = capture->second;
    else target = widget_at(x, y);

    if (!target) {
        if (action == PointerAction::down) clear_focus();
        return false;
    }
    if (action == PointerAction::down) {
        Widget* focus = target;
        while (focus && !focus->focusable()) {
            focus = dynamic_cast<Widget*>(focus->parent());
        }
        if (focus || !top_modal()) set_focus(focus);
    }

    scene2d::InputEvent routed;
    routed.type = action == PointerAction::down
        ? scene2d::InputType::pointer_down
        : action == PointerAction::up
            ? scene2d::InputType::pointer_up
            : action == PointerAction::cancel
                ? scene2d::InputType::pointer_cancel
                : scene2d::InputType::pointer_move;
    routed.pointer_id = pointer_id;
    routed.stage_x = x;
    routed.stage_y = y;
    routed.button = button;
    const bool handled = stage_.dispatch_input(routed, target);
    auto* handler = dynamic_cast<Widget*>(routed.handled_by());
    if (action == PointerAction::down && handler) captures_[pointer_id] = handler;
    if (action == PointerAction::up || action == PointerAction::cancel) {
        captures_.erase(pointer_id);
    }
    prune_closed_windows();
    return handled;
}

bool Ui::key_down(Key key, KeyModifiers modifiers)
{
    scene2d::InputEvent routed;
    routed.type = scene2d::InputType::key_down;
    routed.modifiers = modifiers;
    switch (key) {
    case Key::left: routed.key = scene2d::InputKey::left; break;
    case Key::right: routed.key = scene2d::InputKey::right; break;
    case Key::up: routed.key = scene2d::InputKey::up; break;
    case Key::down: routed.key = scene2d::InputKey::down; break;
    case Key::home: routed.key = scene2d::InputKey::home; break;
    case Key::end: routed.key = scene2d::InputKey::end; break;
    case Key::backspace: routed.key = scene2d::InputKey::backspace; break;
    case Key::delete_key: routed.key = scene2d::InputKey::delete_key; break;
    case Key::enter: routed.key = scene2d::InputKey::enter; break;
    case Key::space: routed.key = scene2d::InputKey::space; break;
    case Key::tab: routed.key = scene2d::InputKey::tab; break;
    case Key::escape: routed.key = scene2d::InputKey::escape; break;
    }
    bool handled = focused_ && stage_.dispatch_input(routed, focused_);
    if (!handled && key == Key::tab) handled = focus_next(modifiers.shift);
    if (!handled && (key == Key::left || key == Key::right ||
                     key == Key::up || key == Key::down)) {
        handled = focus_direction(key);
    }
    if (!handled && key == Key::escape && !overlays_.empty()) {
        Window* window = overlays_.back().window;
        if (window->escape_closes()) {
            window->request_close();
            handled = true;
        }
    }
    prune_closed_windows();
    return handled;
}

bool Ui::key_up(Key key, KeyModifiers modifiers)
{
    if (!focused_) return false;
    scene2d::InputEvent routed;
    routed.type = scene2d::InputType::key_up;
    routed.modifiers = modifiers;
    switch (key) {
    case Key::left: routed.key = scene2d::InputKey::left; break;
    case Key::right: routed.key = scene2d::InputKey::right; break;
    case Key::up: routed.key = scene2d::InputKey::up; break;
    case Key::down: routed.key = scene2d::InputKey::down; break;
    case Key::home: routed.key = scene2d::InputKey::home; break;
    case Key::end: routed.key = scene2d::InputKey::end; break;
    case Key::backspace: routed.key = scene2d::InputKey::backspace; break;
    case Key::delete_key: routed.key = scene2d::InputKey::delete_key; break;
    case Key::enter: routed.key = scene2d::InputKey::enter; break;
    case Key::space: routed.key = scene2d::InputKey::space; break;
    case Key::tab: routed.key = scene2d::InputKey::tab; break;
    case Key::escape: routed.key = scene2d::InputKey::escape; break;
    }
    return stage_.dispatch_input(routed, focused_);
}

bool Ui::navigation(
    application::Event::Navigation navigation_value,
    std::int32_t input_device_id
)
{
    using Navigation = application::Event::Navigation;
    scene2d::InputEvent routed;
    routed.type = scene2d::InputType::navigation;
    routed.input_device_id = input_device_id;
    switch (navigation_value) {
    case Navigation::left:
        routed.navigation = scene2d::NavigationAction::left;
        break;
    case Navigation::right:
        routed.navigation = scene2d::NavigationAction::right;
        break;
    case Navigation::up:
        routed.navigation = scene2d::NavigationAction::up;
        break;
    case Navigation::down:
        routed.navigation = scene2d::NavigationAction::down;
        break;
    case Navigation::next:
        routed.navigation = scene2d::NavigationAction::next;
        break;
    case Navigation::previous:
        routed.navigation = scene2d::NavigationAction::previous;
        break;
    case Navigation::activate:
        routed.navigation = scene2d::NavigationAction::activate;
        break;
    case Navigation::cancel:
        routed.navigation = scene2d::NavigationAction::cancel;
        break;
    case Navigation::unknown:
        return false;
    }
    if (focused_ && stage_.dispatch_input(routed, focused_)) return true;

    switch (navigation_value) {
    case Navigation::left: return focus_direction(Key::left);
    case Navigation::right: return focus_direction(Key::right);
    case Navigation::up: return focus_direction(Key::up);
    case Navigation::down: return focus_direction(Key::down);
    case Navigation::next: return focus_next(false);
    case Navigation::previous: return focus_next(true);
    case Navigation::activate: return key_down(Key::enter);
    case Navigation::cancel: return key_down(Key::escape);
    case Navigation::unknown:
    default: return false;
    }
}

bool Ui::text_input(std::string_view text)
{
    return focused_ && focused_->text_input(text);
}

bool Ui::text_editing(std::string_view text, int start, int length)
{
    return focused_ && focused_->text_editing(text, start, length);
}

void Ui::clear_focus() { set_focus(nullptr); }

Widget* Ui::widget_at(float x, float y) noexcept
{
    scene2d::Actor* actor = stage_.hit(x, y, true);
    if (Window* modal = top_modal(); modal && !is_descendant_of(actor, modal)) {
        return modal;
    }
    while (actor) {
        if (auto* widget = dynamic_cast<Widget*>(actor)) return widget;
        actor = actor->parent();
    }
    return nullptr;
}

void Ui::paint_tree(
    const scene2d::Actor& actor,
    Painter& painter,
    const Skin& skin,
    float parent_x,
    float parent_y
)
{
    if (!actor.visible()) return;
    const float x = parent_x + actor.x();
    const float y = parent_y + actor.y();
    const auto* widget = dynamic_cast<const Widget*>(&actor);
    if (widget) {
        painter.push_clip({x, y, widget->width(), widget->height()});
        widget->paint(painter, skin, x, y);
    }
    if (const auto* group = dynamic_cast<const scene2d::Group*>(&actor)) {
        for (std::size_t index = 0; index < group->child_count(); ++index) {
            paint_tree(*group->child_at(index), painter, skin, x, y);
        }
    }
    if (widget) painter.pop_clip();
}

void Ui::set_focus(Widget* widget)
{
    if (Window* modal = top_modal();
        modal && widget && !is_descendant_of(widget, modal)) {
        widget = nullptr;
    }
    if (focused_ == widget) {
        if (text_input_service_ && !text_input_service_->active() &&
            dynamic_cast<TextField*>(focused_)) {
            float stage_x = 0.0F;
            float stage_y = 0.0F;
            for (const scene2d::Actor* current = focused_; current;
                 current = current->parent()) {
                stage_x += current->x();
                stage_y += current->y();
            }
            text_input_service_->start({
                .area = {stage_x, stage_y, focused_->width(), focused_->height()}
            });
        }
        return;
    }
    if (focused_) focused_->focus_changed(false);
    focused_ = widget;
    if (focused_) focused_->focus_changed(true);
    if (!text_input_service_) return;
    if (dynamic_cast<TextField*>(focused_)) {
        float stage_x = 0.0F;
        float stage_y = 0.0F;
        for (const scene2d::Actor* current = focused_; current;
             current = current->parent()) {
            stage_x += current->x();
            stage_y += current->y();
        }
        text_input_service_->start({
            .area = {stage_x, stage_y, focused_->width(), focused_->height()}
        });
    } else if (text_input_service_->active()) {
        text_input_service_->stop();
    }
}

bool Ui::focus_next(bool reverse)
{
    const std::vector<Widget*> candidates = focusable_widgets();
    if (candidates.empty()) {
        clear_focus();
        return false;
    }
    const auto current = std::find(candidates.begin(), candidates.end(), focused_);
    std::size_t index = 0;
    if (current != candidates.end()) {
        const auto position = static_cast<std::size_t>(
            std::distance(candidates.begin(), current)
        );
        index = reverse
            ? (position + candidates.size() - 1) % candidates.size()
            : (position + 1) % candidates.size();
    } else if (reverse) {
        index = candidates.size() - 1;
    }
    set_focus(candidates[index]);
    return true;
}

bool Ui::focus_direction(Key direction)
{
    if (!focused_) return focus_next(false);
    const std::vector<Widget*> candidates = focusable_widgets();
    float current_x = 0.0F;
    float current_y = 0.0F;
    stage_position(*focused_, current_x, current_y);
    current_x += focused_->width() * 0.5F;
    current_y += focused_->height() * 0.5F;

    Widget* best = nullptr;
    float best_score = std::numeric_limits<float>::max();
    for (Widget* candidate : candidates) {
        if (candidate == focused_) continue;
        float candidate_x = 0.0F;
        float candidate_y = 0.0F;
        stage_position(*candidate, candidate_x, candidate_y);
        candidate_x += candidate->width() * 0.5F;
        candidate_y += candidate->height() * 0.5F;
        const float dx = candidate_x - current_x;
        const float dy = candidate_y - current_y;
        float primary = 0.0F;
        float perpendicular = 0.0F;
        bool eligible = false;
        if (direction == Key::left && dx < 0.0F) {
            primary = -dx;
            perpendicular = std::abs(dy);
            eligible = true;
        } else if (direction == Key::right && dx > 0.0F) {
            primary = dx;
            perpendicular = std::abs(dy);
            eligible = true;
        } else if (direction == Key::up && dy < 0.0F) {
            primary = -dy;
            perpendicular = std::abs(dx);
            eligible = true;
        } else if (direction == Key::down && dy > 0.0F) {
            primary = dy;
            perpendicular = std::abs(dx);
            eligible = true;
        }
        if (!eligible) continue;
        const float score = primary + perpendicular * 2.0F;
        if (score < best_score) {
            best = candidate;
            best_score = score;
        }
    }
    if (!best) return false;
    set_focus(best);
    return true;
}

std::vector<Widget*> Ui::focusable_widgets()
{
    std::vector<Widget*> result;
    scene2d::Actor* scope = top_modal();
    if (!scope) scope = &stage_.root();
    collect_focusable(*scope, result);
    return result;
}

void Ui::collect_focusable(
    scene2d::Actor& actor,
    std::vector<Widget*>& result
)
{
    if (!actor.visible()) return;
    if (auto* widget = dynamic_cast<Widget*>(&actor);
        widget && widget->focusable()) {
        result.push_back(widget);
    }
    if (auto* group = dynamic_cast<scene2d::Group*>(&actor)) {
        for (std::size_t index = 0; index < group->child_count(); ++index) {
            collect_focusable(*group->child_at(index), result);
        }
    }
}

void Ui::stage_position(
    const scene2d::Actor& actor,
    float& x,
    float& y
) noexcept
{
    x = 0.0F;
    y = 0.0F;
    for (const scene2d::Actor* current = &actor; current;
         current = current->parent()) {
        x += current->x();
        y += current->y();
    }
}

void Ui::prune_closed_windows()
{
    for (std::size_t index = overlays_.size(); index > 0; --index) {
        Window* window = overlays_[index - 1].window;
        if (window->close_requested()) close_window(*window);
    }
}

Window* Ui::top_modal() noexcept
{
    for (auto found = overlays_.rbegin(); found != overlays_.rend(); ++found) {
        if (found->window->modal()) return found->window;
    }
    return nullptr;
}

const Window* Ui::top_modal() const noexcept
{
    for (auto found = overlays_.rbegin(); found != overlays_.rend(); ++found) {
        if (found->window->modal()) return found->window;
    }
    return nullptr;
}

bool Ui::is_descendant_of(
    const scene2d::Actor* actor,
    const scene2d::Actor* ancestor
) noexcept
{
    for (const scene2d::Actor* current = actor; current; current = current->parent()) {
        if (current == ancestor) return true;
    }
    return false;
}

} // namespace squared::gui
