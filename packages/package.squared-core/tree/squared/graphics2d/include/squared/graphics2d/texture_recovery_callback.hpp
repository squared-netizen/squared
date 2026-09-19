#pragma once

namespace sq::graphics2d {

class TextureRecoveryTarget;

using TextureRecoveryCallback = bool (*)(
    void* user_data,
    TextureRecoveryTarget& target
) noexcept;

} // namespace sq::graphics2d
