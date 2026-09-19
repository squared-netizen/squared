#pragma once

#include <memory>

namespace sq::assets {

/**
 * @brief Shared immutable reference to one loaded asset.
 * @tparam T Asset value type.
 *
 * @note This is one of the few places squared keeps std::shared_ptr. The
 * ownership really is shared and really is dynamic: a texture outlives the
 * skin that asked for it when a widget still holds it, and the manager cannot
 * know which goes first. It is 16 bytes plus a control block per asset, not
 * per frame and not per draw, so the cost is bounded by the number of distinct
 * assets. See docs/developer/standard-library-deviations.md.
 */
template <typename T>
using AssetHandle = std::shared_ptr<const T>;

}  // namespace sq::assets
