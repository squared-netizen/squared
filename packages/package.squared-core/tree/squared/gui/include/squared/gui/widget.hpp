#pragma once

#include <squared/gui/key.hpp>
#include <squared/gui/key_modifiers.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/size_hints.hpp>
#include <squared/scene2d/group.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace sq::scene2d {
class InputEvent;
} // namespace sq::scene2d

namespace sq::gui {

class Painter;
class Skin;
struct PointerEvent;

/**
 * @brief Base class for every GUI node. Widgets may own other widgets.
 *
 * Widget inherits the Scene2D composite ownership model: children owned via
 * Group. Widgets carry layout, painting, and input behavior.
 */
class Widget : public scene2d::Group {
public:
    /** @brief Factory creating one fresh tooltip widget subtree on demand. */
    using TooltipFactory = std::function<std::unique_ptr<Widget>()>;

    /** @brief Identifier published for scene2d::actor_cast<Widget>(). */
    static constexpr scene2d::ActorInterfaceId actor_interface_id =
        0x57'44'47'54U;

    ~Widget() override = default;

    /**
     * @brief Publish the Widget interface to scene2d::actor_cast().
     * @param id Identifier of the requested interface.
     * @return This widget for the Widget identifier, otherwise the Group
     * answer, so actor_cast<scene2d::Group>() still resolves.
     */
    [[nodiscard]] void* actor_interface(
        scene2d::ActorInterfaceId id
    ) noexcept override
    {
        return id == actor_interface_id
            ? this
            : scene2d::Group::actor_interface(id);
    }

    /**
     * @brief Publish the Widget interface to scene2d::actor_cast(), read-only.
     * @param id Identifier of the requested interface.
     * @return This widget for the Widget identifier, otherwise the Group
     * answer.
     */
    [[nodiscard]] const void* actor_interface(
        scene2d::ActorInterfaceId id
    ) const noexcept override
    {
        return id == actor_interface_id
            ? this
            : scene2d::Group::actor_interface(id);
    }

    /**
     * @brief Report the smallest acceptable size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing style and spacing values.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] virtual Size minimum_size(Painter& painter, const Skin& skin) const;

    /**
     * @brief Report the preferred size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing style, font, and spacing values.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] virtual Size preferred_size(
        Painter& painter,
        const Skin& skin
    ) const;

    /**
     * @brief Report the largest acceptable size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing style and spacing values.
     * @return Maximum extent in logical units.
     */
    [[nodiscard]] virtual Size maximum_size(Painter& painter, const Skin& skin) const;

    /**
     * @brief Compute the full size-hint set.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing style and spacing values.
     * @return Minimum, preferred, and maximum sizes.
     */
    [[nodiscard]] SizeHints size_hints(Painter& painter, const Skin& skin) const;

    /**
     * @brief Invalidate cached layout so it is recomputed next pass.
     */
    void invalidate_layout() noexcept;

    /**
     * @brief Recompute layout if currently invalid.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing style and spacing values.
     */
    void validate_layout(Painter& painter, const Skin& skin);

    /**
     * @brief Check whether cached layout is current.
     * @return true while the layout is valid.
     */
    [[nodiscard]] bool layout_valid() const noexcept { return layout_valid_; }

    /**
     * @brief Position and size this widget, then lay out children.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing style and spacing values.
     */
    virtual void layout(Painter& painter, const Skin& skin);

    /**
     * @brief Paint this widget and, typically, its children.
     * @param painter Active painter; valid for the call.
     * @param skin Skin providing style resources.
     * @param stage_x Stage-space origin associated with this widget.
     * @param stage_y Stage-space origin associated with this widget.
     */
    virtual void paint(
        Painter& painter,
        const Skin& skin,
        float stage_x,
        float stage_y
    ) const;

    /**
     * @brief Handle a pointer event.
     * @param event Pointer payload in widget-local coordinates.
     * @return true when the event was handled and should stop propagating.
     */
    virtual bool pointer_event(const PointerEvent& event);

    /**
     * @brief Handle a key-down event.
     * @param key Portable key name.
     * @param modifiers Modifier state at the time of the event.
     * @return true when the key was handled.
     */
    virtual bool key_down(Key key, KeyModifiers modifiers = {});

    /**
     * @brief Handle committed UTF-8 text input.
     * @param text Committed text to insert.
     * @return true when the text was handled.
     */
    virtual bool text_input(std::string_view text);

    /**
     * @brief Handle composition-edit updates.
     * @note Arguments are the current composition text, its start offset, and
     * its length, matching the virtual signature.
     * @return true when the composition was handled.
     */
    virtual bool text_editing(std::string_view, int, int);

    /**
     * @brief Notify the widget of a focus change.
     * @param focused true when the widget gained focus.
     */
    virtual void focus_changed(bool focused);

    /**
     * @brief Report whether this widget can take keyboard focus.
     * @return true when the widget is focusable.
     */
    [[nodiscard]] virtual bool focusable() const noexcept;

    /**
     * @brief Report whether focusing this widget should raise the platform
     * text-input service.
     * @return false by default; text-entry widgets override it.
     * @note The Ui starts, moves, and stops the platform soft keyboard from
     * this answer. A custom text-entry widget opts in by overriding it, which
     * the previous TextField-specific check did not allow.
     */
    [[nodiscard]] virtual bool wants_text_input() const noexcept;

    /**
     * @brief Control the enabled state.
     * @param enabled false disables interaction and dims rendering.
     */
    void set_enabled(bool enabled) noexcept { enabled_ = enabled; }

    /**
     * @brief Read the enabled state.
     * @return true while the widget is enabled.
     */
    [[nodiscard]] bool enabled() const noexcept { return enabled_; }

    /**
     * @brief Attach a plain-text tooltip composed from standard GUI widgets.
     * @param text UTF-8 tooltip text. An empty string clears the tooltip.
     * @post A fresh Stack/Panel/MarginContainer/Label subtree is created each
     * time the tooltip is shown.
     */
    void set_tooltip(std::string text);

    /**
     * @brief Attach a custom tooltip widget factory.
     * @param factory Copyable callback returning a fresh unparented widget
     * subtree. An empty callback clears the tooltip.
     * @note The Ui temporarily owns each returned subtree while it is shown.
     */
    void set_tooltip_factory(TooltipFactory factory);

    /** @brief Remove the tooltip declaration from this widget. */
    void clear_tooltip() noexcept;

    /** @brief Return true when this widget declares tooltip content. */
    [[nodiscard]] bool has_tooltip() const noexcept
    {
        return static_cast<bool>(tooltip_factory_);
    }

private:
    friend class Ui;
    bool layout_valid_{false};
    bool enabled_{true};
    TooltipFactory tooltip_factory_;

protected:
    /**
     * @brief Bridge Scene2D dispatch into widget input handlers.
     * @param event Scene2D input event to adapt.
     */
    void input_event(scene2d::InputEvent& event) override;
};

} // namespace sq::gui
