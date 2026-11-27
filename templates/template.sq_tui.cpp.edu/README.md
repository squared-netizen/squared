# template.sq_tui.cpp.edu

A teaching workspace on squared-tui's `Simple`: the second step after
`template.term.edu.cpp`'s printed lines. A learner draws in cells and reads
keys with no event plumbing (an @ moved with h j k l), and `TRY.md` has
things to change, one at a time.

It is written against `package.squared-tui`, which is named with it: sqpg
adds no package that was not asked for (D-080), and the build says so if it
is missing.

```sh
sqpg new NAME --template template.sq_tui.cpp.edu --package package.squared-tui
```

Everything under `tree/` is copied into the workspace; `{{name}}` is
substituted. This file is beside `tree/`, not in it.
