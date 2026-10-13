#pragma once

#include <squared/graphics/src/texture_recovery_target.hpp>

namespace sq::graphics {

using TextureRecoveryCallback = bool (*)(
    void* user_data,
    TextureRecoveryTarget& target
) noexcept;

} // namespace sq::graphics
