#pragma once

#include <squared/application/event.hpp>
#include <squared/graphics/color.hpp>
#include <squared/graphics2d/texture_region.hpp>
#include <squared/scene2d/group.hpp>
#include <squared/scene2d/stage.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace squared::gui {

struct Size {
    float width{0.0F};
    float height{0.0F};
};

struct Rectangle {
    float x{0.0F};
    float y{0.0F};
    float width{0.0F};
    float height{0.0F};
};

struct Insets {
    float left{0.0F};
    float top{0.0F};
    float right{0.0F};
    float bottom{0.0F};
};

struct SizeHints {
    Size minimum{};
    Size preferred{};
    Size maximum{
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity()
    };
};

/** Portable drawing boundary implemented over Squared Graphics2D. */
class Painter {
public:
    virtual ~Painter() = default;

    [[nodiscard]] virtual Size measure_text(std::string_view text) = 0;
    virtual void fill_rectangle(
        const Rectangle& rectangle,
        graphics::Color color
    ) = 0;
    virtual void stroke_rectangle(
        const Rectangle& rectangle,
        graphics::Color color,
        float thickness
    ) = 0;
    virtual void draw_region(
        const graphics2d::TextureRegion& region,
        const Rectangle& rectangle,
        graphics::Color color = graphics::Color::white()
    ) = 0;
    virtual void draw_text(
        std::string_view text,
        float x,
        float y,
        graphics::Color color
    ) = 0;
    virtual void push_clip(const Rectangle& rectangle) = 0;
    virtual void pop_clip() = 0;
};

/** Skin image abstraction; it never exposes SDL or another backend type. */
class Drawable {
public:
    virtual ~Drawable() = default;
    [[nodiscard]] virtual Size minimum_size() const noexcept = 0;
    [[nodiscard]] virtual Insets content_insets() const noexcept = 0;
    virtual void draw(
        Painter& painter,
        const Rectangle& rectangle,
        graphics::Color tint = graphics::Color::white()
    ) const = 0;
};

using DrawablePtr = std::shared_ptr<const Drawable>;

class ColorDrawable final : public Drawable {
public:
    explicit ColorDrawable(
        graphics::Color color,
        Size minimum = {},
        Insets insets = {}
    ) noexcept;

    [[nodiscard]] Size minimum_size() const noexcept override;
    [[nodiscard]] Insets content_insets() const noexcept override;
    void draw(
        Painter& painter,
        const Rectangle& rectangle,
        graphics::Color tint = graphics::Color::white()
    ) const override;

private:
    graphics::Color color_;
    Size minimum_;
    Insets insets_;
};

class RegionDrawable final : public Drawable {
public:
    explicit RegionDrawable(
        const graphics2d::TextureRegion& region,
        Insets insets = {}
    ) noexcept;

    [[nodiscard]] Size minimum_size() const noexcept override;
    [[nodiscard]] Insets content_insets() const noexcept override;
    void draw(
        Painter& painter,
        const Rectangle& rectangle,
        graphics::Color tint = graphics::Color::white()
    ) const override;

private:
    const graphics2d::TextureRegion* region_;
    Insets insets_;
};

struct PanelStyle {
    DrawablePtr background;
};

struct ButtonStyle {
    DrawablePtr normal;
    DrawablePtr hovered;
    DrawablePtr pressed;
    DrawablePtr disabled;
    graphics::Color text{graphics::Color::from_rgba8(238, 241, 247)};
    graphics::Color disabled_text{graphics::Color::from_rgba8(174, 181, 194)};
    float minimum_height{44.0F};
    float horizontal_padding{12.0F};
};

struct TextFieldStyle {
    DrawablePtr normal;
    DrawablePtr focused;
    graphics::Color text{graphics::Color::from_rgba8(238, 241, 247)};
    graphics::Color cursor{graphics::Color::from_rgba8(79, 137, 255)};
    float minimum_height{44.0F};
    float horizontal_padding{10.0F};
};

struct CheckBoxStyle {
    DrawablePtr unchecked;
    DrawablePtr checked;
    DrawablePtr disabled;
    graphics::Color text{graphics::Color::from_rgba8(238, 241, 247)};
    float spacing{8.0F};
    float minimum_touch_size{44.0F};
};

struct SliderStyle {
    DrawablePtr track;
    DrawablePtr filled_track;
    DrawablePtr knob;
    float minimum_length{120.0F};
    float minimum_touch_size{44.0F};
};

/** Named, reusable skin resources and widget styles. */
class Skin {
public:
    Skin();

    graphics::Color surface{graphics::Color::from_rgba8(28, 31, 38)};
    graphics::Color control{graphics::Color::from_rgba8(52, 57, 68)};
    graphics::Color control_hover{graphics::Color::from_rgba8(66, 73, 87)};
    graphics::Color accent{graphics::Color::from_rgba8(79, 137, 255)};
    graphics::Color text{graphics::Color::from_rgba8(238, 241, 247)};
    graphics::Color muted_text{graphics::Color::from_rgba8(174, 181, 194)};
    graphics::Color border{graphics::Color::from_rgba8(91, 99, 116)};
    float padding{8.0F};
    float spacing{6.0F};
    float border_width{1.0F};
    float minimum_touch_size{44.0F};

    void add_drawable(std::string name, DrawablePtr drawable);
    void add_region_drawable(
        std::string name,
        const graphics2d::TextureRegion& region,
        Insets insets = {}
    );
    [[nodiscard]] DrawablePtr drawable(std::string_view name) const noexcept;

    void add_panel_style(std::string name, PanelStyle style);
    void add_button_style(std::string name, ButtonStyle style);
    void add_text_field_style(std::string name, TextFieldStyle style);
    void add_check_box_style(std::string name, CheckBoxStyle style);
    void add_slider_style(std::string name, SliderStyle style);

    [[nodiscard]] const PanelStyle& panel_style(std::string_view name) const;
    [[nodiscard]] const ButtonStyle& button_style(std::string_view name) const;
    [[nodiscard]] const TextFieldStyle& text_field_style(std::string_view name) const;
    [[nodiscard]] const CheckBoxStyle& check_box_style(std::string_view name) const;
    [[nodiscard]] const SliderStyle& slider_style(std::string_view name) const;

private:
    std::unordered_map<std::string, DrawablePtr> drawables_;
    std::unordered_map<std::string, PanelStyle> panel_styles_;
    std::unordered_map<std::string, ButtonStyle> button_styles_;
    std::unordered_map<std::string, TextFieldStyle> text_field_styles_;
    std::unordered_map<std::string, CheckBoxStyle> check_box_styles_;
    std::unordered_map<std::string, SliderStyle> slider_styles_;
};

enum class PointerAction { move, down, up, cancel };

struct PointerEvent {
    PointerAction action{PointerAction::move};
    std::int64_t pointer_id{0};
    float x{0.0F};
    float y{0.0F};
    int button{0};
};

enum class Key {
    left,
    right,
    home,
    end,
    backspace,
    delete_key,
    enter,
    tab
};

/** Base class for every GUI node. Widgets may own other widgets. */
class Widget : public scene2d::Group {
public:
    ~Widget() override = default;

    [[nodiscard]] virtual Size minimum_size(Painter& painter, const Skin& skin) const;
    [[nodiscard]] virtual Size preferred_size(Painter& painter) const;
    [[nodiscard]] virtual Size maximum_size(Painter& painter, const Skin& skin) const;
    [[nodiscard]] SizeHints size_hints(Painter& painter, const Skin& skin) const;

    void invalidate_layout() noexcept;
    void validate_layout(Painter& painter, const Skin& skin);
    [[nodiscard]] bool layout_valid() const noexcept { return layout_valid_; }

    virtual void layout(Painter& painter, const Skin& skin);
    virtual void paint(
        Painter& painter,
        const Skin& skin,
        float stage_x,
        float stage_y
    ) const;

    virtual bool pointer_event(const PointerEvent& event);
    virtual bool key_down(Key key);
    virtual bool text_input(std::string_view text);
    virtual void focus_changed(bool focused);
    [[nodiscard]] virtual bool focusable() const noexcept;

    void set_enabled(bool enabled) noexcept { enabled_ = enabled; }
    [[nodiscard]] bool enabled() const noexcept { return enabled_; }

private:
    bool layout_valid_{false};
    bool enabled_{true};
};

class Label final : public Widget {
public:
    explicit Label(std::string text = {});
    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    void set_muted(bool muted) noexcept { muted_ = muted; }
    [[nodiscard]] Size preferred_size(Painter& painter) const override;
    void paint(Painter&, const Skin&, float, float) const override;

private:
    std::string text_;
    bool muted_{false};
};

class Image final : public Widget {
public:
    explicit Image(DrawablePtr drawable = {});
    void set_drawable(DrawablePtr drawable);
    [[nodiscard]] Size preferred_size(Painter& painter) const override;
    void paint(Painter&, const Skin&, float, float) const override;

private:
    DrawablePtr drawable_;
};

class Panel : public Widget {
public:
    explicit Panel(std::string style = "default");
    void set_style(std::string style);
    void paint(Painter&, const Skin&, float, float) const override;

private:
    std::string style_;
};

enum class Direction { horizontal, vertical };

class LinearLayout final : public Widget {
public:
    explicit LinearLayout(Direction direction = Direction::vertical) noexcept;
    Widget& add(std::unique_ptr<Widget> child, float grow = 0.0F);
    void set_padding(float padding) noexcept;
    void set_spacing(float spacing) noexcept;
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;
    [[nodiscard]] Size preferred_size(Painter&) const override;
    void layout(Painter&, const Skin&) override;

private:
    struct Slot { Widget* widget{nullptr}; float grow{0.0F}; };
    [[nodiscard]] Size measured_size(Painter&, const Skin*, bool minimum) const;
    Direction direction_;
    float padding_{-1.0F};
    float spacing_{-1.0F};
    std::vector<Slot> slots_;
};

class Stack final : public Widget {
public:
    Widget& add(std::unique_ptr<Widget> child);
    [[nodiscard]] Size preferred_size(Painter&) const override;
    void layout(Painter&, const Skin&) override;
};

class MarginContainer final : public Widget {
public:
    explicit MarginContainer(Insets margin = {}) noexcept;
    Widget& set_content(std::unique_ptr<Widget> content);
    [[nodiscard]] Size preferred_size(Painter&) const override;
    void layout(Painter&, const Skin&) override;

private:
    Insets margin_;
    Widget* content_{nullptr};
};

class ScrollPane final : public Widget {
public:
    Widget& set_content(std::unique_ptr<Widget> content);
    void set_scroll_y(float scroll_y) noexcept;
    [[nodiscard]] float scroll_y() const noexcept { return scroll_y_; }
    [[nodiscard]] Size preferred_size(Painter&) const override;
    void layout(Painter&, const Skin&) override;
    bool pointer_event(const PointerEvent&) override;

private:
    void clamp_scroll() noexcept;
    Widget* content_{nullptr};
    float scroll_y_{0.0F};
    float last_pointer_y_{0.0F};
    std::optional<std::int64_t> drag_pointer_;
};

class Button : public Widget {
public:
    using Callback = std::function<void()>;
    explicit Button(std::string text = {}, Callback callback = {});
    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    void set_on_click(Callback callback);
    void set_style(std::string style);
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;
    [[nodiscard]] Size preferred_size(Painter&) const override;
    void paint(Painter&, const Skin&, float, float) const override;
    bool pointer_event(const PointerEvent&) override;
    bool key_down(Key) override;
    [[nodiscard]] bool focusable() const noexcept override;

protected:
    virtual void activate();
    [[nodiscard]] virtual bool selected() const noexcept;
    [[nodiscard]] bool pressed() const noexcept { return pressed_; }
    [[nodiscard]] bool hovered() const noexcept { return hovered_; }

private:
    std::string text_;
    std::string style_{"default"};
    Callback callback_;
    bool pressed_{false};
    bool hovered_{false};
};

class ToggleButton : public Button {
public:
    using ChangeCallback = std::function<void(bool)>;
    explicit ToggleButton(std::string text = {}, bool checked = false);
    void set_checked(bool checked);
    [[nodiscard]] bool checked() const noexcept { return checked_; }
    void set_on_change(ChangeCallback callback);

protected:
    void activate() override;
    [[nodiscard]] bool selected() const noexcept override;

private:
    bool checked_{false};
    ChangeCallback change_callback_;
};

class CheckBox final : public ToggleButton {
public:
    explicit CheckBox(std::string text = {}, bool checked = false);
    void set_check_style(std::string style);
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;
    [[nodiscard]] Size preferred_size(Painter&) const override;
    void paint(Painter&, const Skin&, float, float) const override;

private:
    std::string check_style_{"default"};
};

class TextField final : public Widget {
public:
    explicit TextField(std::string text = {});
    void set_text(std::string text);
    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    [[nodiscard]] std::size_t cursor() const noexcept { return cursor_; }
    void set_style(std::string style);
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;
    [[nodiscard]] Size preferred_size(Painter&) const override;
    void paint(Painter&, const Skin&, float, float) const override;
    bool pointer_event(const PointerEvent&) override;
    bool key_down(Key) override;
    bool text_input(std::string_view) override;
    void focus_changed(bool) override;
    [[nodiscard]] bool focusable() const noexcept override;

private:
    std::string text_;
    std::string style_{"default"};
    std::size_t cursor_{0};
    bool focused_{false};
};

class Slider final : public Widget {
public:
    using ChangeCallback = std::function<void(float)>;
    Slider(float minimum = 0.0F, float maximum = 1.0F, float value = 0.0F);
    void set_range(float minimum, float maximum) noexcept;
    void set_value(float value);
    [[nodiscard]] float value() const noexcept { return value_; }
    void set_step(float step) noexcept;
    void set_on_change(ChangeCallback callback);
    void set_style(std::string style);
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;
    [[nodiscard]] Size preferred_size(Painter&) const override;
    void paint(Painter&, const Skin&, float, float) const override;
    bool pointer_event(const PointerEvent&) override;

private:
    void update_from_pointer(float x);
    float minimum_{0.0F};
    float maximum_{1.0F};
    float value_{0.0F};
    float step_{0.0F};
    std::string style_{"default"};
    ChangeCallback callback_;
    std::optional<std::int64_t> drag_pointer_;
};

class Separator final : public Widget {
public:
    explicit Separator(Direction direction = Direction::horizontal) noexcept;
    [[nodiscard]] Size preferred_size(Painter&) const override;
    void paint(Painter&, const Skin&, float, float) const override;

private:
    Direction direction_;
};

/** Owns one widget tree and consumes the framework application event type. */
class Ui {
public:
    Ui(float width, float height, Skin skin = {});
    Widget& set_content(std::unique_ptr<Widget> content);
    [[nodiscard]] Widget* content() noexcept { return content_; }
    [[nodiscard]] const Widget* content() const noexcept { return content_; }
    void resize(float width, float height);
    void update(double delta_seconds);
    void layout(Painter& painter);
    void paint(Painter& painter) const;

    bool event(const application::Event& event);
    bool pointer(
        PointerAction action,
        float x,
        float y,
        int button = 0,
        std::int64_t pointer_id = 0
    );
    bool key_down(Key key);
    bool text_input(std::string_view text);
    void clear_focus();
    [[nodiscard]] Widget* focused() noexcept { return focused_; }
    [[nodiscard]] const Skin& skin() const noexcept { return skin_; }
    [[nodiscard]] Skin& skin() noexcept { return skin_; }

private:
    [[nodiscard]] Widget* widget_at(float x, float y) noexcept;
    static void local_position(
        const Widget&, float, float, float&, float&
    ) noexcept;
    static void paint_tree(
        const scene2d::Actor&, Painter&, const Skin&, float, float
    );
    void set_focus(Widget* widget);

    scene2d::Stage stage_;
    Skin skin_;
    Widget* content_{nullptr};
    Widget* focused_{nullptr};
    std::unordered_map<std::int64_t, Widget*> captures_;
};

} // namespace squared::gui
