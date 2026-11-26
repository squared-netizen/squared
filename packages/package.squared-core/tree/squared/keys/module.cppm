// sq.keys: the key vocabulary, Key and KeyModifiers, on their own.
//
// sq.app re-exports it (its events carry them), and sq.term and sq.tui
// import it alone, so a terminal program does not bring sq.app's graphics.
// The types keep their namespace, sq::app, so nothing that names them
// changes.

export module sq.keys;

export import :key;
export import :key_modifiers;
