#pragma once

namespace sq::graphics2d {

/**
 * @brief CPU-side recipe used to recover a texture after context loss.
 *
 * Policies are backend-neutral. They describe what source data Squared keeps
 * or asks the application to regenerate; they do not expose the selected
 * windowing or graphics API.
 */
enum class TextureRecoveryPolicy {
    ReloadFromAsset,
    RetainPixels,
    Regenerate,
    Discard
};

} // namespace sq::graphics2d
