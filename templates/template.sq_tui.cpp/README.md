# template.sq_tui.cpp

A character-cell program on squared-tui: a frame, a title and an @ that the
arrows (or h j k l) move, drawn by `sq::tui::Terminal` and tested with
`sq::tui::Test`. squared-core (sq.term, sq.keys) is the installed SDK's, or
a `squared/` folder of the workspace's own.

It is written against `package.squared-tui`, which is named with it: sqpg
adds no package that was not asked for (D-080), and the build says so if it
is missing.

```sh
sqpg new NAME --template template.sq_tui.cpp --package package.squared-tui
```

Everything under `tree/` is copied into the workspace; `{{name}}` is
substituted. This file is beside `tree/`, not in it.
