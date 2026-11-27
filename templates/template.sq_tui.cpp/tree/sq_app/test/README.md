# sq_app/test

Drop a `.cpp` here with its own `main` and `sqmake test` builds and runs it,
linked with your modules and sq.tui (never your main). `sq::tui::Test` draws
a screen with no terminal and reads it back: `board_test.cpp` shows how.
