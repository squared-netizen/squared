#include "detail/catalog.hpp"

// Weak: sqgen's definitions, in an app with layout files or names.json, take
// their place. Alone in their file, so a static archive brings them in only
// when nothing else defines layouts::all() and layouts::screens().
[[gnu::weak]] std::span<const sq::gui::Layout *const> layouts::all() noexcept
{
    return {};
}

[[gnu::weak]] std::span<const std::string_view> layouts::screens() noexcept
{
    return {};
}
