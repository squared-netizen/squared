#pragma once

// Aggregate header for the input group of
// sq::gui. Include a single type's header
// directly to keep a translation unit narrow.
// The key, modifier and navigation types are sq::app's: the platform layer
// and the GUI share them.

#include <squared/app/src/key.hpp>
#include <squared/app/src/key_modifiers.hpp>
#include <squared/app/src/navigation.hpp>
#include <squared/gui/src/input_event.hpp>
#include <squared/gui/src/input_listener.hpp>
#include <squared/gui/src/input_listener_id.hpp>
#include <squared/gui/src/input_phase.hpp>
#include <squared/gui/src/input_type.hpp>
