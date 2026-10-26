#pragma once

#include <squared/files/embedded_file_system.hpp>

#include <span>

namespace sq::detail
{

/**
 * @brief The default skin's four files (`uiskin.atlas`, its page
 * `uiskin.png`, `uiskin.json`, `default.fnt`), compiled in under
 * `skins/default/skin/`: `default_skin.cpp`, which `tools/embed_files.sh`
 * writes from `stage/skin/default/skin/`.
 */
[[nodiscard]] std::span<const files::EmbeddedFile> default_skin() noexcept;

} // namespace sq::detail
