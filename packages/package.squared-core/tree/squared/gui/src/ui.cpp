#include <squared/gui/ui.hpp>

#include "detail/gui_detail.hpp"
#include <squared/app/event.hpp>
#include <squared/app/text_input.hpp>
#include <squared/gui/dialog.hpp>
#include <squared/gui/key.hpp>
#include <squared/gui/key_modifiers.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/pointer_action.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/tooltip_config.hpp>
#include <squared/gui/widget.hpp>
#include <squared/gui/window.hpp>
#include <squared/gui/window_style.hpp>
#include <squared/scene2d/actor.hpp>
#include <squared/scene2d/group.hpp>
#include <squared/scene2d/input_event.hpp>
#include <squared/scene2d/input_key.hpp>
#include <squared/scene2d/input_type.hpp>
#include <squared/scene2d/navigation_action.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Ui::Ui(float width, float height, Skin skin)
    : stage_(width, height), skin_(std::move(skin))
{
}

Widget& Ui::set_content(std::unique_ptr<Widget> content)
{
    if (!content) throw std::invalid_argument("GUI content must not be null");
    hide_tooltip();
    reset_tooltip_candidates();
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
    hide_tooltip();
    reset_tooltip_candidates();
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
    if (is_descendant_of(tooltip_owner_, &window)) hide_tooltip();
    if (is_descendant_of(hover_tooltip_owner_, &window) ||
        is_descendant_of(press_tooltip_owner_, &window) ||
        is_descendant_of(focus_tooltip_owner_, &window)) {
        reset_tooltip_candidates();
    }
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
    const double elapsed = std::isfinite(delta_seconds)
        ? std::max(0.0, delta_seconds) : 0.0;
    stage_.act(elapsed);
    prune_closed_windows();

    if (tooltip_owner_ && !tooltip_owner_allowed(tooltip_owner_)) {
        hide_tooltip();
    }
    if (press_tooltip_active_ && press_tooltip_owner_) {
        if (!tooltip_owner_allowed(press_tooltip_owner_)) {
            press_tooltip_active_ = false;
            press_tooltip_owner_ = nullptr;
        } else if (tooltip_owner_ != press_tooltip_owner_) {
            press_tooltip_elapsed_ += elapsed;
            if (press_tooltip_elapsed_ >= tooltip_config_.long_press_delay) {
                show_tooltip(*press_tooltip_owner_, true);
                if (tooltip_owner_ == press_tooltip_owner_) {
                    const auto capture = captures_.find(
                        press_tooltip_pointer_id_
                    );
                    if (capture != captures_.end()) {
                        scene2d::InputEvent cancelled;
                        cancelled.type = scene2d::InputType::pointer_cancel;
                        cancelled.pointer_id = press_tooltip_pointer_id_;
                        cancelled.stage_x = tooltip_anchor_x_;
                        cancelled.stage_y = tooltip_anchor_y_;
                        static_cast<void>(
                            stage_.dispatch_input(cancelled, capture->second)
                        );
                        captures_.erase(capture);
                    }
                }
                press_tooltip_active_ = false;
            }
        }
        return;
    }
    if (hover_tooltip_owner_) {
        if (!tooltip_owner_allowed(hover_tooltip_owner_)) {
            hover_tooltip_owner_ = nullptr;
            hover_tooltip_elapsed_ = 0.0;
        } else if (tooltip_owner_ != hover_tooltip_owner_) {
            hover_tooltip_elapsed_ += elapsed;
            if (hover_tooltip_elapsed_ >= tooltip_config_.hover_delay) {
                show_tooltip(*hover_tooltip_owner_, true);
            }
        }
        return;
    }
    if (focus_tooltip_owner_) {
        if (!tooltip_owner_allowed(focus_tooltip_owner_) ||
            !is_descendant_of(focused_, focus_tooltip_owner_)) {
            focus_tooltip_owner_ = nullptr;
            focus_tooltip_elapsed_ = 0.0;
        } else if (tooltip_owner_ != focus_tooltip_owner_) {
            focus_tooltip_elapsed_ += elapsed;
            if (focus_tooltip_elapsed_ >= tooltip_config_.focus_delay) {
                show_tooltip(*focus_tooltip_owner_, false);
            }
        }
    }
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
        focused_ && focused_->wants_text_input()) {
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
    layout_tooltip(painter);
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

bool Ui::event(const app::Event& event)
{
    switch (event.type) {
    case app::Event::Type::PointerDown:
        return pointer(PointerAction::down, event.x, event.y, 0, event.pointer_id);
    case app::Event::Type::PointerMove:
        return pointer(PointerAction::move, event.x, event.y, 0, event.pointer_id);
    case app::Event::Type::PointerUp:
        return pointer(PointerAction::up, event.x, event.y, 0, event.pointer_id);
    case app::Event::Type::Resize:
        resize(static_cast<float>(event.width), static_cast<float>(event.height));
        return true;
    case app::Event::Type::TextInput:
        return text_input(event.text);
    case app::Event::Type::TextEditing:
        return text_editing(
            event.text, event.editing_start, event.editing_length
        );
    case app::Event::Type::TextInputHidden:
        return text_editing({}, 0, 0);
    case app::Event::Type::KeyDown: {
        using AppKey = app::Event::Key;
        KeyModifiers modifiers{
            event.modifiers.contains(app::KeyModifier::shift),
            event.modifiers.contains(app::KeyModifier::control),
            event.modifiers.contains(app::KeyModifier::alt),
            event.modifiers.contains(app::KeyModifier::meta)
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
    case app::Event::Type::KeyUp: {
        using AppKey = app::Event::Key;
        KeyModifiers modifiers{
            event.modifiers.contains(app::KeyModifier::shift),
            event.modifiers.contains(app::KeyModifier::control),
            event.modifiers.contains(app::KeyModifier::alt),
            event.modifiers.contains(app::KeyModifier::meta)
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
    case app::Event::Type::NavigationInput:
        return navigation(event.navigation, event.input_device_id);
    default:
        return false;
    }
}

void Ui::set_text_input_service(
    app::TextInputService* service
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
    tooltip_anchor_x_ = x;
    tooltip_anchor_y_ = y;
    const bool tooltip_was_visible = tooltip_widget_ != nullptr;
    if (tooltip_widget_) hide_tooltip();

    scene2d::Actor* tooltip_hit = stage_.hit(x, y, false);
    if (Window* modal = top_modal();
        modal && !is_descendant_of(tooltip_hit, modal)) {
        tooltip_hit = modal;
    }
    Widget* tooltip_target = nullptr;
    for (scene2d::Actor* actor = tooltip_hit; actor; actor = actor->parent()) {
        if (auto* widget = scene2d::actor_cast<Widget>(actor)) {
            tooltip_target = widget;
            break;
        }
    }
    Widget* candidate = tooltip_owner_for(tooltip_target);

    if (action == PointerAction::move) {
        focus_tooltip_owner_ = nullptr;
        focus_tooltip_elapsed_ = 0.0;
        if (press_tooltip_active_ && pointer_id == press_tooltip_pointer_id_) {
            const float dx = x - press_start_x_;
            const float dy = y - press_start_y_;
            const float tolerance = tooltip_config_.movement_tolerance;
            if (dx * dx + dy * dy > tolerance * tolerance) {
                press_tooltip_active_ = false;
                press_tooltip_owner_ = nullptr;
                press_tooltip_elapsed_ = 0.0;
            }
        } else if (candidate != hover_tooltip_owner_ || tooltip_was_visible) {
            hover_tooltip_owner_ = candidate;
            hover_tooltip_elapsed_ = 0.0;
        }
    } else if (action == PointerAction::down) {
        hover_tooltip_owner_ = nullptr;
        hover_tooltip_elapsed_ = 0.0;
        focus_tooltip_owner_ = nullptr;
        focus_tooltip_elapsed_ = 0.0;
        press_tooltip_owner_ = button == 0 ? candidate : nullptr;
        press_tooltip_elapsed_ = 0.0;
        press_tooltip_pointer_id_ = pointer_id;
        press_start_x_ = x;
        press_start_y_ = y;
        press_tooltip_active_ = press_tooltip_owner_ != nullptr;
    } else if (action == PointerAction::up || action == PointerAction::cancel) {
        if (pointer_id == press_tooltip_pointer_id_) {
            press_tooltip_active_ = false;
            press_tooltip_owner_ = nullptr;
            press_tooltip_elapsed_ = 0.0;
        }
    }

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
            focus = scene2d::actor_cast<Widget>(focus->parent());
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
    auto* handler = scene2d::actor_cast<Widget>(routed.handled_by());
    if (action == PointerAction::down && handler) captures_[pointer_id] = handler;
    if (action == PointerAction::up || action == PointerAction::cancel) {
        captures_.erase(pointer_id);
    }
    prune_closed_windows();
    return handled;
}

bool Ui::key_down(Key key, KeyModifiers modifiers)
{
    hide_tooltip();
    focus_tooltip_owner_ = nullptr;
    focus_tooltip_elapsed_ = 0.0;
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
    app::Event::Navigation navigation_value,
    std::int32_t input_device_id
)
{
    hide_tooltip();
    focus_tooltip_owner_ = nullptr;
    focus_tooltip_elapsed_ = 0.0;
    using Navigation = app::Event::Navigation;
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
    hide_tooltip();
    focus_tooltip_owner_ = nullptr;
    focus_tooltip_elapsed_ = 0.0;
    return focused_ && focused_->text_input(text);
}

bool Ui::text_editing(std::string_view text, int start, int length)
{
    hide_tooltip();
    focus_tooltip_owner_ = nullptr;
    focus_tooltip_elapsed_ = 0.0;
    return focused_ && focused_->text_editing(text, start, length);
}

void Ui::clear_focus() { set_focus(nullptr); }

void Ui::set_tooltip_config(TooltipConfig config)
{
    const bool valid =
        std::isfinite(config.hover_delay) && config.hover_delay >= 0.0 &&
        std::isfinite(config.long_press_delay) &&
            config.long_press_delay >= 0.0 &&
        std::isfinite(config.focus_delay) && config.focus_delay >= 0.0 &&
        std::isfinite(config.movement_tolerance) &&
            config.movement_tolerance >= 0.0F &&
        std::isfinite(config.viewport_margin) &&
            config.viewport_margin >= 0.0F &&
        std::isfinite(config.owner_gap) && config.owner_gap >= 0.0F &&
        std::isfinite(config.pointer_offset) &&
            config.pointer_offset >= 0.0F;
    if (!valid) {
        throw std::invalid_argument(
            "Tooltip configuration values must be finite and non-negative"
        );
    }
    tooltip_config_ = config;
    hide_tooltip();
    reset_tooltip_candidates();
}

Widget* Ui::widget_at(float x, float y) noexcept
{
    scene2d::Actor* actor = stage_.hit(x, y, true);
    if (Window* modal = top_modal(); modal && !is_descendant_of(actor, modal)) {
        return modal;
    }
    while (actor) {
        if (auto* widget = scene2d::actor_cast<Widget>(actor)) return widget;
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
    const auto* widget = scene2d::actor_cast<Widget>(&actor);
    if (widget) {
        painter.push_clip({x, y, widget->width(), widget->height()});
        widget->paint(painter, skin, x, y);
    }
    if (const auto* group = scene2d::actor_cast<scene2d::Group>(&actor)) {
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
            focused_ && focused_->wants_text_input()) {
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
    if (tooltip_widget_ && !tooltip_pointer_anchor_) hide_tooltip();
    focus_tooltip_owner_ = nullptr;
    focus_tooltip_elapsed_ = 0.0;
    if (focused_) focused_->focus_changed(false);
    focused_ = widget;
    if (focused_) focused_->focus_changed(true);
    if (!text_input_service_) return;
    if (focused_ && focused_->wants_text_input()) {
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
    arm_focus_tooltip(candidates[index]);
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
    arm_focus_tooltip(best);
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
    if (auto* widget = scene2d::actor_cast<Widget>(&actor);
        widget && widget->focusable()) {
        result.push_back(widget);
    }
    if (auto* group = scene2d::actor_cast<scene2d::Group>(&actor)) {
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

Widget* Ui::tooltip_owner_for(Widget* target) const noexcept
{
    for (Widget* current = target; current;
         current = scene2d::actor_cast<Widget>(current->parent())) {
        if (current->has_tooltip() && tooltip_owner_allowed(current)) {
            return current;
        }
    }
    return nullptr;
}

bool Ui::tooltip_owner_allowed(const Widget* owner) const noexcept
{
    if (!owner || !owner->has_tooltip() ||
        !is_descendant_of(owner, &stage_.root())) {
        return false;
    }
    for (const scene2d::Actor* current = owner; current;
         current = current->parent()) {
        if (!current->visible()) return false;
    }
    const Window* modal = top_modal();
    return !modal || is_descendant_of(owner, modal);
}

void Ui::arm_focus_tooltip(Widget* owner) noexcept
{
    focus_tooltip_owner_ = tooltip_owner_for(owner);
    focus_tooltip_elapsed_ = 0.0;
    hover_tooltip_owner_ = nullptr;
    hover_tooltip_elapsed_ = 0.0;
}

void Ui::show_tooltip(Widget& owner, bool pointer_anchor)
{
    if (!tooltip_owner_allowed(&owner) || tooltip_owner_ == &owner) return;
    std::unique_ptr<Widget> tooltip = owner.tooltip_factory_();
    if (!tooltip) return;
    if (tooltip->parent()) {
        throw std::invalid_argument("Tooltip factory returned a parented widget");
    }
    make_subtree_untouchable(*tooltip);
    hide_tooltip();
    tooltip_widget_ = tooltip.get();
    tooltip_owner_ = &owner;
    tooltip_pointer_anchor_ = pointer_anchor;
    try {
        static_cast<void>(stage_.add_actor(std::move(tooltip)));
    } catch (...) {
        tooltip_widget_ = nullptr;
        tooltip_owner_ = nullptr;
        throw;
    }
}

void Ui::hide_tooltip() noexcept
{
    if (tooltip_widget_ && tooltip_widget_->parent() == &stage_.root()) {
        [[maybe_unused]] auto removed =
            stage_.root().remove_actor(*tooltip_widget_);
    }
    tooltip_widget_ = nullptr;
    tooltip_owner_ = nullptr;
}

void Ui::reset_tooltip_candidates() noexcept
{
    hover_tooltip_owner_ = nullptr;
    hover_tooltip_elapsed_ = 0.0;
    press_tooltip_owner_ = nullptr;
    press_tooltip_elapsed_ = 0.0;
    press_tooltip_active_ = false;
    focus_tooltip_owner_ = nullptr;
    focus_tooltip_elapsed_ = 0.0;
}

void Ui::layout_tooltip(Painter& painter)
{
    if (!tooltip_widget_ || !tooltip_owner_) return;
    if (!tooltip_owner_allowed(tooltip_owner_)) {
        hide_tooltip();
        return;
    }

    const float viewport_width = stage_.root().width();
    const float viewport_height = stage_.root().height();
    const float margin = std::min(
        tooltip_config_.viewport_margin,
        std::max(0.0F, std::min(viewport_width, viewport_height) * 0.5F)
    );
    const Size preferred = tooltip_widget_->size_hints(painter, skin_).preferred;
    const float width = std::min(
        preferred.width, std::max(0.0F, viewport_width - 2.0F * margin)
    );
    const float height = std::min(
        preferred.height, std::max(0.0F, viewport_height - 2.0F * margin)
    );

    float anchor_left = tooltip_anchor_x_;
    float anchor_top = tooltip_anchor_y_;
    float anchor_bottom = tooltip_anchor_y_;
    float x = 0.0F;
    float y = 0.0F;
    float gap = tooltip_config_.pointer_offset;
    if (tooltip_pointer_anchor_) {
        x = tooltip_anchor_x_ + gap;
        y = tooltip_anchor_y_ + gap;
    } else {
        stage_position(*tooltip_owner_, anchor_left, anchor_top);
        anchor_bottom = anchor_top + tooltip_owner_->height();
        gap = tooltip_config_.owner_gap;
        x = anchor_left + (tooltip_owner_->width() - width) * 0.5F;
        y = anchor_bottom + gap;
    }
    if (y + height > viewport_height - margin) {
        y = anchor_top - gap - height;
    }
    const float maximum_x = std::max(margin, viewport_width - margin - width);
    const float maximum_y = std::max(margin, viewport_height - margin - height);
    x = std::clamp(x, margin, maximum_x);
    y = std::clamp(y, margin, maximum_y);

    tooltip_widget_->set_bounds(x, y, width, height);
    tooltip_widget_->invalidate_layout();
    tooltip_widget_->validate_layout(painter, skin_);
}

void Ui::make_subtree_untouchable(scene2d::Actor& actor) noexcept
{
    actor.set_touchable(false);
    if (auto* group = scene2d::actor_cast<scene2d::Group>(&actor)) {
        for (std::size_t index = 0; index < group->child_count(); ++index) {
            make_subtree_untouchable(*group->child_at(index));
        }
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

} // namespace sq::gui
