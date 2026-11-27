// Your first screen program.
//
// Run it with:  sqmake run       (press q to stop)
//
// A screen program does not print lines one after another. It owns the
// whole terminal: a grid of character cells, like squares on graph paper.
// Column x counts from the left, row y from the top, both starting at 0.
//
// Every program like this repeats three steps, over and over:
//   1. draw:  say what goes in which cell
//   2. show:  put it on the screen
//   3. wait:  for the next key, then change something
//
// Everything after `//` on a line is a comment: the computer ignores it.

import sq.tui; // squared-tui: the screen and the keyboard
import std;    // C++'s standard library

int main() {
    sq::tui::Simple screen;  // takes over the terminal; gives it back when the program ends

    int x = 10;  // where the @ is: column...
    int y = 5;   // ...and row

    for (;;) {  // forever (until `return` below)
        // 1. Draw.
        screen.clear();
        screen.put(2, 1, "Move the @ with h j k l. Press q to stop.");
        screen.put(x, y, "@");

        // 2. Show.
        screen.show();

        // 3. Wait for one key, then act on it.
        const sq::tui::Key key = screen.key();
        if (key == 'q') return 0;  // q: stop
        if (key == 'h') --x;       // left
        if (key == 'l') ++x;       // right
        if (key == 'k') --y;       // up
        if (key == 'j') ++y;       // down
    }
}
