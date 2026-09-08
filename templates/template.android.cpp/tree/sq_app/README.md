# sq_app — your code

The **user working directory**. Everything in it is yours; the generator will
not write here again.

```text
sq_app/include/    your headers
sq_app/src/        your sources — every .cpp here is compiled
```

## The one rule

**No Android headers in this directory.**

No `<android/...>`, no `<EGL/...>`, no `<GLES3/...>`. The platform layer in
`sq_android/` owns all of that and calls into your `App` through the interface
in `app.hpp`.

The exception, and it is a deliberate one: `<squared/kit/gl.hpp>` guarded by
`__has_include`, and `<android/log.h>` for logging. Both are cheap to replace
if you ever port this elsewhere.

Keeping to this means the same `App` compiles under `template.termux.cpp` with
a different platform layer. It is easy to lose one include at a time, so it is
worth checking when you add code.

## Lifecycle

| Method | When |
|---|---|
| `start()` | once, before the first frame |
| `resume()` / `pause()` | focus gained and lost |
| `resize(w, h)` | before the first render, and on rotation |
| `render()` | each frame, only while a surface exists |
| `touch(phase, x, y)` | a touch; return true if handled |
| `stop()` | once, on the way out |

Called from the platform layer on the main thread. **None of them may block.**
Android kills an unresponsive process without warning.

`pause()` is the last call you are guaranteed — Android may destroy the process
afterwards without calling `stop()`. Save anything you cannot lose there.
