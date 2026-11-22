// Every public type in sq::tui: character-cell programs (squared-tui's first
// layers, docs/design/squared-tui.md). A screen of sq::term cells drawn into
// and rendered as the bytes that show what changed (Screen, View, Renderer);
// the terminal a program runs in, in raw mode, put back however the program
// ends, and its input as events (Terminal, InputDecoder); and a test bench
// that reads a screen back through sqterm's own emulator (Test). Simple is
// the terminal for a first program: draw, show, wait for a key.

export module sq.tui;

export import :event;
export import :input_decoder;
export import :key;
export import :rect;
export import :renderer;
export import :screen;
export import :simple;
export import :style;
export import :terminal;
export import :test_bench;
