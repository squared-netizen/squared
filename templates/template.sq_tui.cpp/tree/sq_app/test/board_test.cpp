// The board, drawn and stepped with no terminal: sq::tui::Test renders the
// screen into squared's own terminal emulator (sqterm's), and the test reads
// back what a person would see. `sqmake test` runs it.

#include <cassert>

import board;
import sq.tui;
import std;

int main() {
    sq::tui::Test t(40, 8);
    board::State state;

    board::draw(t.screen(), state);
    t.show();
    assert(t.text(2, 0).starts_with(" {{project_name}} "));
    assert(t.text(2, 2).starts_with("@"));
    assert(t.text(2, 6).find("key: none yet") != std::string::npos);

    // Keys typed as a terminal sends them: "l" moves right, the down arrow down.
    for (const sq::tui::Event& event : t.type("l\x1b[B")) {
        board::step(state, std::get<sq::tui::Key>(event), 40, 8);
    }
    board::draw(t.screen(), state);
    t.show();
    assert(state.x == 3 && state.y == 3);
    assert(t.text(3, 3).starts_with("@"));
    assert(t.text(2, 6).find("key: Down") != std::string::npos);

    // The frame holds it: far to the left stops at the border.
    for (int i = 0; i < 10; ++i) board::step(state, sq::tui::Key{.character = U'h'}, 40, 8);
    assert(state.x == 1);

    board::step(state, sq::tui::Key{.character = U'q'}, 40, 8);
    assert(state.quit);

    std::println("board: all assertions passed");
}
