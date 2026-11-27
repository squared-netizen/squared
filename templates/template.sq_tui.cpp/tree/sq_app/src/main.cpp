// {{project_name}}: the program's loop. It opens the terminal (raw mode and
// the alternate screen, both put back however the program ends), then draws,
// shows and waits for the next event until the board says to stop.
//
// What it draws and how a key moves it is board.cppm's; this file only runs
// it. Run it with `sqmake run`.

import board;
import sq.tui;
import std;

int main() {
    auto terminal = sq::tui::Terminal::open();
    if (!terminal) {
        std::println(std::cerr, "{{project_name}}: {}", sq::tui::describe(terminal.error()));
        return 1;
    }
    sq::tui::Screen screen(terminal->columns(), terminal->rows());
    board::State state;
    while (!state.quit) {
        board::draw(screen, state);
        terminal->show(screen);
        // Wait for something to happen (a negative timeout waits forever).
        const std::optional<sq::tui::Event> event = terminal->next(std::chrono::milliseconds{-1});
        if (!event) continue;
        if (const auto* key = std::get_if<sq::tui::Key>(&*event)) {
            board::step(state, *key, screen.columns(), screen.rows());
        } else if (const auto* size = std::get_if<sq::tui::Resize>(&*event)) {
            screen.resize(size->columns, size->rows);
        }
    }
}
