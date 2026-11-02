// The app's layouts, as sqgen writes them into build/gen/layouts.cpp; the
// declaration is the one sqgen's layouts.hpp makes. An app without layout
// files has no such definition, and the framework's own, empty and weak
// (catalog.cpp), links in its place.

export module sq.stage:src.detail.catalog;

import sq.gui;
import std;

export extern "C++"
{
    namespace layouts
    {

    /** @brief Every layout of the app, sorted by name. */
    [[nodiscard]] std::span<const sq::gui::Layout *const> all() noexcept;

    /** @brief The C++ screens names.json lists, sorted. */
    [[nodiscard]] std::span<const std::string_view> screens() noexcept;

    } // namespace layouts
} // extern "C++"
