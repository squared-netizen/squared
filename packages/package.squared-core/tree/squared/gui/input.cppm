// Aggregate header for the input group of
// sq::gui. Include a single type's header
// directly to keep a translation unit narrow.
// The key, modifier and navigation types are sq::app's: the platform layer
// and the GUI share them.

export module sq.gui:input;

export import :src.input_event;
export import :src.input_listener;
export import :src.input_listener_id;
export import :src.input_phase;
export import :src.input_type;
import sq.app;
