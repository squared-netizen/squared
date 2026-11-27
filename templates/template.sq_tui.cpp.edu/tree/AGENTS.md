# AGENTS.md

This workspace is for someone learning to program by drawing on a terminal
screen with `sq::tui::Simple`. If you are an AI assistant working in it:

- Explain before you write. Say what a line does and why, in plain words.
- Prefer the smallest change that answers the question.
- Keep to the standard library and `Simple` (`clear`, `put`, `show`,
  `key()`, `width()`, `height()`); reach for `Terminal` and `Screen` only
  when the learner asks for more than `Simple` does.
- Code goes in `sq_app/`. `sq_pkg/` is a package's; `sq/build/` is the
  generator's.
- Keep the build warning-free (`-Wall -Wextra -Wpedantic`) and explain any
  warning rather than silencing it.
