#pragma once

#include <squared/gui/layout.hpp>

#include <span>

// The app's layouts, as sqgen writes them into build/gen/layouts.cpp; the
// declaration is the one sqgen's layouts.hpp makes. An app without layout
// files has no such definition, and the framework's own, empty and weak
// (catalog.cpp), links in its place.
namespace layouts
{

/** @brief Every layout of the app, sorted by name. */
[[nodiscard]] std::span<const sq::gui::Layout *const> all() noexcept;

} // namespace layouts
