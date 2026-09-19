#pragma once

#include <squared/app/event.hpp>
#include <squared/app/text_input.hpp>
#include <squared/gui/dialog.hpp>
#include <squared/gui/key.hpp>
#include <squared/gui/key_modifiers.hpp>
#include <squared/gui/pointer_action.hpp>
#include <squared/gui/rectangle.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/tooltip_config.hpp>
#include <squared/gui/widget.hpp>
#include <squared/gui/window.hpp>
#include <squared/scene2d/stage.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sq::scene2d {
class Actor;
} // namespace sq::scene2d

namespace sq::gui {

class Painter;

/**
 * @brief Owns one widget tree and consumes the framework application event type.
 *
 * The Ui owns the stage, one content widget, and the set of open windows. All
 * input enters through event(), pointer(), and the key/text entry points. Not
 * thread-safe; one thread must drive it.
 */
class Ui {
public:
    /**
     * @brief Construct a user interface.
     * @param width Initial virtual width in logical pixels.
     * @param height Initial virtual height in logical pixels.
     * @param skin Skin styling every widget; copied into the Ui.
     */
    Ui(float width, float height, Skin skin = {});

    /**
     * @brief Replace the full-screen content widget.
     * @param content Widget to adopt; ownership transfers to the Ui.
     * @return Reference to the adopted content widget.
     */
    Widget& set_content(std::unique_ptr<Widget> content);

    /**
     * @brief Show a floating window.
     * @param window Window to adopt; ownership transfers to the Ui.
     * @param center true centers the window over the current viewport.
     * @return Reference to the shown window.
     */
    Window& show_window(std::unique_ptr<Window> window, bool center = true);

    /**
     * @brief Show a modal dialog.
     * @param dialog Dialog to adopt; ownership transfers to the Ui.
     * @param center true centers the dialog over the current viewport.
     * @return Reference to the shown dialog.
     */
    Dialog& show_dialog(std::unique_ptr<Dialog> dialog, bool center = true);

    /**
     * @brief Mark a window closed and remove it on the next update.
     * @param window Window currently shown by this Ui.
     */
    void close_window(Window& window);

    /**
     * @brief Count currently shown windows and dialogs.
     * @return Number of open overlays.
     */
    [[nodiscard]] std::size_t window_count() const noexcept
    {
        return overlays_.size();
    }

    /**
     * @brief Access the content widget.
     * @return Pointer to the content widget, or null when unset.
     */
    [[nodiscard]] Widget* content() noexcept { return content_; }

    /**
     * @brief Access the content widget, read-only.
     * @return Pointer to the content widget, or null when unset.
     */
    [[nodiscard]] const Widget* content() const noexcept { return content_; }

    /**
     * @brief Resize the virtual viewport.
     * @param width New width in logical pixels.
     * @param height New height in logical pixels.
     */
    void resize(float width, float height);

    /**
     * @brief Advance time-based state and prune close-requested windows.
     * @param delta_seconds Elapsed time in seconds; non-negative.
     * @throws Any exception raised by a tooltip factory, or
     * std::invalid_argument when a factory returns a parented widget.
     */
    void update(double delta_seconds);

    /**
     * @brief Validate layout of every visible widget.
     * @param painter Painter used for measurement.
     */
    void layout(Painter& painter);

    /**
     * @brief Paint the content and every open overlay.
     * @param painter Painter receiving all drawing.
     */
    void paint(Painter& painter) const;

    /**
     * @brief Consume one framework application event.
     * @param event Event to translate and dispatch.
     * @return true when the Ui handled the event.
     */
    bool event(const app::Event& event);

    /**
     * @brief Inject a pointer input.
     * @param action Pointer action being reported.
     * @param x Horizontal position in logical pixels.
     * @param y Vertical position in logical pixels.
     * @param button Button index; zero for the primary button.
     * @param pointer_id Stable identifier for the pointer contact.
     * @return true when some widget handled the event.
     */
    bool pointer(
        PointerAction action,
        float x,
        float y,
        int button = 0,
        std::int64_t pointer_id = 0
    );

    /**
     * @brief Dispatch a key-down event.
     * @param key Portable key name.
     * @param modifiers Modifier state at the time of the event.
     * @return true when the focused widget handled the key.
     */
    bool key_down(Key key, KeyModifiers modifiers = {});

    /**
     * @brief Dispatch a key-up event.
     * @param key Portable key name.
     * @param modifiers Modifier state at the time of the event.
     * @return true when the focused widget handled the key.
     */
    bool key_up(Key key, KeyModifiers modifiers = {});

    /**
     * @brief Dispatch a navigation action, including focus moves.
     * @param navigation Semantic navigation action.
     * @param input_device_id Source device identifier.
     * @return true when the navigation was handled.
     */
    bool navigation(
        app::Event::Navigation navigation,
        std::int32_t input_device_id = 0
    );

    /**
     * @brief Forward committed text to the focused widget.
     * @param text Committed UTF-8 text.
     * @return true when the focused widget handled the text.
     */
    bool text_input(std::string_view text);

    /**
     * @brief Forward composition updates to the focused widget.
     * @param text Current composition text.
     * @param start Start offset of the composition span.
     * @param length Length of the composition span.
     * @return true when the focused widget handled the composition.
     */
    bool text_editing(std::string_view text, int start, int length);

    /**
     * @brief Install the platform text-input boundary.
     * @param service Non-owning service; may be null to disable. Must outlive
     * the Ui while set.
     */
    void set_text_input_service(app::TextInputService* service) noexcept;

    /**
     * @brief Clear keyboard focus to no widget.
     */
    void clear_focus();

    /**
     * @brief Read the focused widget.
     * @return Focused widget, or null when none is focused.
     */
    [[nodiscard]] Widget* focused() noexcept { return focused_; }

    /**
     * @brief Replace tooltip timing and placement policy.
     * @param config Finite, non-negative delays and distances.
     * @throws std::invalid_argument when any value is negative or non-finite.
     */
    void set_tooltip_config(TooltipConfig config);

    /** @brief Return the active tooltip policy.
     * @return Reference valid for the lifetime of this Ui.
     */
    [[nodiscard]] const TooltipConfig& tooltip_config() const noexcept
    {
        return tooltip_config_;
    }

    /** @brief Return true while a tooltip subtree is attached to the Stage.
     * @return true between materialization and dismissal.
     */
    [[nodiscard]] bool tooltip_visible() const noexcept
    {
        return tooltip_widget_ != nullptr;
    }

    /** @brief Return the widget whose tooltip is visible, or null.
     * @return Non-owning pointer invalidated when its owning tree is removed.
     */
    [[nodiscard]] const Widget* tooltip_owner() const noexcept
    {
        return tooltip_owner_;
    }

    /**
     * @brief Return the visible tooltip rectangle in Stage coordinates.
     * @return Bounds after layout, or no value when no tooltip is attached.
     */
    [[nodiscard]] std::optional<Rectangle> tooltip_bounds() const noexcept
    {
        if (!tooltip_widget_) return std::nullopt;
        return Rectangle{
            tooltip_widget_->x(), tooltip_widget_->y(),
            tooltip_widget_->width(), tooltip_widget_->height()
        };
    }

    /**
     * @brief Access the skin, read-only.
     * @return Reference to the Ui's skin.
     */
    [[nodiscard]] const Skin& skin() const noexcept { return skin_; }

    /**
     * @brief Access the skin for mutation.
     * @return Reference to the Ui's skin.
     */
    [[nodiscard]] Skin& skin() noexcept { return skin_; }

private:
    [[nodiscard]] Widget* widget_at(float x, float y) noexcept;
    static void paint_tree(
        const scene2d::Actor&, Painter&, const Skin&, float, float
    );
    void set_focus(Widget* widget);
    bool focus_next(bool reverse);
    bool focus_direction(Key direction);
    [[nodiscard]] std::vector<Widget*> focusable_widgets();
    static void collect_focusable(
        scene2d::Actor& actor,
        std::vector<Widget*>& result
    );
    static void stage_position(
        const scene2d::Actor& actor,
        float& x,
        float& y
    ) noexcept;
    [[nodiscard]] Widget* tooltip_owner_for(Widget* target) const noexcept;
    [[nodiscard]] bool tooltip_owner_allowed(const Widget* owner) const noexcept;
    void arm_focus_tooltip(Widget* owner) noexcept;
    void show_tooltip(Widget& owner, bool pointer_anchor);
    void hide_tooltip() noexcept;
    void reset_tooltip_candidates() noexcept;
    void layout_tooltip(Painter& painter);
    static void make_subtree_untouchable(scene2d::Actor& actor) noexcept;
    void prune_closed_windows();
    [[nodiscard]] Window* top_modal() noexcept;
    [[nodiscard]] const Window* top_modal() const noexcept;
    [[nodiscard]] static bool is_descendant_of(
        const scene2d::Actor* actor, const scene2d::Actor* ancestor
    ) noexcept;

    scene2d::Stage stage_;
    Skin skin_;
    Widget* content_{nullptr};
    Widget* focused_{nullptr};
    std::unordered_map<std::int64_t, Widget*> captures_;
    app::TextInputService* text_input_service_{nullptr};
    struct Overlay { Window* window; Widget* previous_focus; bool center_pending; };
    std::vector<Overlay> overlays_;
    TooltipConfig tooltip_config_;
    Widget* hover_tooltip_owner_{nullptr};
    double hover_tooltip_elapsed_{0.0};
    Widget* press_tooltip_owner_{nullptr};
    double press_tooltip_elapsed_{0.0};
    std::int64_t press_tooltip_pointer_id_{0};
    float press_start_x_{0.0F};
    float press_start_y_{0.0F};
    bool press_tooltip_active_{false};
    Widget* focus_tooltip_owner_{nullptr};
    double focus_tooltip_elapsed_{0.0};
    float tooltip_anchor_x_{0.0F};
    float tooltip_anchor_y_{0.0F};
    bool tooltip_pointer_anchor_{false};
    Widget* tooltip_owner_{nullptr};
    Widget* tooltip_widget_{nullptr};
};

} // namespace sq::gui
