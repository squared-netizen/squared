#pragma once

#include <cstdint>

namespace sq::scene2d {

/**
 * @brief Stable identifier for one layer's Actor-derived interface.
 *
 * Scene2D is a public layer: sq::gui derives Widget from Group, and other
 * layers may do the same. Recovering a derived interface from an Actor& needs
 * a downcast that does not depend on RTTI, so each derived interface publishes
 * a constant of this type and answers Actor::actor_interface() for it.
 *
 * Identifiers are four-character constants chosen by the layer that owns the
 * interface. Collisions are the owning layer's responsibility; the framework
 * reserves values whose high byte is `0x00`.
 */
using ActorInterfaceId = std::uint32_t;

} // namespace sq::scene2d
