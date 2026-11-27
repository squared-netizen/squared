// {{project_name}}: what the program shows and how a key changes it, a module
// of its own so a test can draw it with no terminal (sq_app/test/board_test.cpp)
// and main.cpp only runs the loop.

export module board;

import sq.tui;
import std;

export namespace board {

/// Everything the program knows: where the @ is, the last key, whether to stop.
struct State {
    int x = 2;
    int y = 2;
    std::string last = "none yet";
    bool quit = false;
};

/// A key as a person would name it: "a", "Ctrl+c", "Up".
[[nodiscard]] std::string describe(const sq::tui::Key& key) {
    using sq::tui::KeyName;
    std::string out;
    if (key.modifiers.control) out += "Ctrl+";
    if (key.modifiers.alt) out += "Alt+";
    if (key.modifiers.shift && !key.is_character()) out += "Shift+";
    if (key.is_character()) {
        if (key.character < 0x80) {
            out += static_cast<char>(key.character);
        } else {
            out += std::format("U+{:04X}", static_cast<std::uint32_t>(key.character));
        }
        return out;
    }
    switch (key.name) {
        case KeyName::up:
            return out + "Up";
        case KeyName::down:
            return out + "Down";
        case KeyName::left:
            return out + "Left";
        case KeyName::right:
            return out + "Right";
        case KeyName::enter:
            return out + "Enter";
        case KeyName::escape:
            return out + "Escape";
        case KeyName::tab:
            return out + "Tab";
        case KeyName::backspace:
            return out + "Backspace";
        case KeyName::space:
            return out + "Space";
        default:
            return out + "another key";
    }
}

/// One key: the arrows or h j k l move the @ inside the frame; q or Escape quits.
void step(State& state, const sq::tui::Key& key, int columns, int rows) {
    using sq::tui::KeyName;
    state.last = describe(key);
    if (key == U'q' || key.name == KeyName::escape) {
        state.quit = true;
        return;
    }
    if (key == U'h' || key.name == KeyName::left) --state.x;
    if (key == U'l' || key.name == KeyName::right) ++state.x;
    if (key == U'k' || key.name == KeyName::up) --state.y;
    if (key == U'j' || key.name == KeyName::down) ++state.y;
    // Inside the frame, above the status line.
    state.x = std::clamp(state.x, 1, std::max(1, columns - 2));
    state.y = std::clamp(state.y, 1, std::max(1, rows - 3));
}

/// The whole screen, drawn again from the state: a frame, a title, the @ and
/// a status line. The terminal is sent only what changed since the last.
void draw(sq::tui::Screen& screen, const State& state) {
    // A colour (24-bit, 0xRRGGBB) on the terminal's own background.
    const auto ink = [](std::uint32_t hex, bool bold = false) {
        sq::tui::Style style;
        style.foreground = sq::tui::rgb(hex);
        style.attributes.bold = bold;
        return style;
    };
    const sq::tui::Style frame = ink(0x5a4fcf);
    const sq::tui::Style title = ink(0x00e5ff, true);
    const sq::tui::Style player = ink(0xff2a6d, true);
    const sq::tui::Style status = ink(0x9aa0a6);

    screen.clear();
    screen.box(screen.area(), frame);
    screen.put(2, 0, " {{project_name}} ", title);
    const int bottom = screen.rows() - 2;
    if (bottom > 0) {
        screen.put(2, static_cast<std::uint16_t>(bottom),
                   std::format("key: {}   (arrows or hjkl move, q quits)", state.last), status);
    }
    screen.put(static_cast<std::uint16_t>(state.x), static_cast<std::uint16_t>(state.y), U'@',
               player);
}

}  // namespace board
