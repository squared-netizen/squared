# Things to try

Change `src/main.cpp`, then `sqmake run`. One at a time; run after each.

1. **Start somewhere else.** Make the @ start in the top-left corner.
2. **A different player.** Draw `#` instead of `@`, or a word.
3. **Walls.** The @ can walk off the screen. Stop it: before `--x`, check
   that `x > 0`. `screen.width()` and `screen.height()` say how big the
   screen is.
4. **The arrow keys.** A named key is not a letter: compare its name.
   ```cpp
   if (key.name == sq::tui::KeyName::left) --x;
   ```
5. **Leave a trail.** Draw a `.` everywhere the @ has been. (You need to
   remember the places: a `std::vector` of points, or a grid of `bool`s.)
6. **Colour.** `put` takes a style:
   ```cpp
   sq::tui::Style red;
   red.foreground = sq::tui::rgb(0xff2a6d);
   screen.put(x, y, "@", red);
   ```
7. **A treasure.** Put a `$` somewhere. When the @ reaches it, say so on the
   screen and move the `$` somewhere else. Count how many were found.

When `Simple` starts to feel small, `template.sq_tui.cpp` shows the same
program written with the types underneath it (`Terminal`, `Screen`, events
and a test that checks the screen with no terminal).
