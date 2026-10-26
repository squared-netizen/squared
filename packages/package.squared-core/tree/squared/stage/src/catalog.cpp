#include "detail/catalog.hpp"

// Weak: sqgen's definition, in an app with layout files, takes its place.
// Alone in its file, so a static archive brings it in only when nothing else
// defines layouts::all().
[[gnu::weak]] std::span<const sq::gui::Layout *const> layouts::all() noexcept
{
    return {};
}
