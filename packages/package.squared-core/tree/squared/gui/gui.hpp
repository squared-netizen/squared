#pragma once

// Every public type in sq::gui, and the sq::app key, modifier and navigation
// types its input uses. Prefer a narrower include in headers of your own.

#include <squared/app/src/key.hpp>
#include <squared/app/src/key_modifiers.hpp>
#include <squared/app/src/navigation.hpp>
#include <squared/gui/actor.hpp>
#include <squared/gui/batch_painter.hpp>
#include <squared/gui/button.hpp>
#include <squared/gui/button_group.hpp>
#include <squared/gui/cell.hpp>
#include <squared/gui/check_box.hpp>
#include <squared/gui/dialog.hpp>
#include <squared/gui/group.hpp>
#include <squared/gui/image.hpp>
#include <squared/gui/label.hpp>
#include <squared/gui/linear_layout.hpp>
#include <squared/gui/margin_container.hpp>
#include <squared/gui/panel.hpp>
#include <squared/gui/progress_bar.hpp>
#include <squared/gui/radio_button.hpp>
#include <squared/gui/scroll_pane.hpp>
#include <squared/gui/separator.hpp>
#include <squared/gui/skin.hpp>
// skin_loader.hpp includes this file back: it is an aggregate kept for source
// compatibility. #pragma once makes the cycle harmless; Sprint 3 settles the
// aggregates (docs/developer/lint.md).
// NOLINTNEXTLINE(misc-header-include-cycle)
#include <squared/gui/skin_loader.hpp>
#include <squared/gui/slider.hpp>
#include <squared/gui/src/actor_interface_id.hpp>
#include <squared/gui/src/alignment.hpp>
#include <squared/gui/src/button_style.hpp>
#include <squared/gui/src/check_box_style.hpp>
#include <squared/gui/src/color_drawable.hpp>
#include <squared/gui/src/direction.hpp>
#include <squared/gui/src/drawable.hpp>
#include <squared/gui/src/drawable_ptr.hpp>
#include <squared/gui/src/font_ptr.hpp>
#include <squared/gui/src/font_resource.hpp>
#include <squared/gui/src/input_event.hpp>
#include <squared/gui/src/input_listener.hpp>
#include <squared/gui/src/input_listener_id.hpp>
#include <squared/gui/src/input_phase.hpp>
#include <squared/gui/src/input_type.hpp>
#include <squared/gui/src/insets.hpp>
#include <squared/gui/src/label_style.hpp>
#include <squared/gui/src/nine_patch_drawable.hpp>
#include <squared/gui/src/nine_patch_splits.hpp>
#include <squared/gui/src/painter.hpp>
#include <squared/gui/src/panel_style.hpp>
#include <squared/gui/src/pointer_action.hpp>
#include <squared/gui/src/pointer_event.hpp>
#include <squared/gui/src/progress_bar_style.hpp>
#include <squared/gui/src/rectangle.hpp>
#include <squared/gui/src/region_drawable.hpp>
#include <squared/gui/src/size.hpp>
#include <squared/gui/src/size_hints.hpp>
#include <squared/gui/src/skin_drawable_resolver.hpp>
#include <squared/gui/src/skin_font_resolver.hpp>
#include <squared/gui/src/skin_load_issue.hpp>
#include <squared/gui/src/skin_load_limits.hpp>
#include <squared/gui/src/skin_load_report.hpp>
#include <squared/gui/src/skin_load_severity.hpp>
#include <squared/gui/src/slider_style.hpp>
#include <squared/gui/src/text_field_style.hpp>
#include <squared/gui/src/tooltip_config.hpp>
#include <squared/gui/src/window_style.hpp>
#include <squared/gui/stack.hpp>
#include <squared/gui/stage.hpp>
#include <squared/gui/table.hpp>
#include <squared/gui/text_field.hpp>
#include <squared/gui/toggle_button.hpp>
#include <squared/gui/ui.hpp>
#include <squared/gui/widget.hpp>
#include <squared/gui/window.hpp>
