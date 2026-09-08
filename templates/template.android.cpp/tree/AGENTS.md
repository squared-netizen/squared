# AGENTS.md — {{project_name}}

Conventions for this workspace. Seeded by squared-pg; yours to change.

## Ground rules

- **Write code in `sq_app/`.** That is the user working directory.
- **Keep Android out of `sq_app/`.** No `<android/...>`, no EGL, no GLES. The
  platform layer in `sq_android/` calls into your `App`. That separation is
  what lets the same class compile under other templates, and it is easy to
  lose one include at a time.
- **`sq_android/` is yours too**, but it is the platform layer, not where
  application code goes. Change it when you need to change how this project
  meets Android — a new lifecycle hook, a manifest permission, a different
  entry point.
- **Never edit `mk/squared_generated.mk`, `mk/squared_android_package.mk`, or
  anything under `sq_kit/`.** Generator-owned and rewritten on update. Build
  settings go in the `Makefile`, which is yours; every generated variable uses
  `?=` so overriding one works.
- **The build globs.** Every `.cpp` under `sq_app/src/` and `sq_android/` is
  compiled.

## Language

C++20. `-Wall -Wextra -Wpedantic`; keep the build warning-free.

Nothing on the main thread may block. Android kills an unresponsive process
without warning.

## Rendering

Guard the kit include with `__has_include`, as `app.cpp` does. It costs two
lines and means adding or swapping a rendering kit later does not require
editing code you have already written.

`sq::gl::Context::present()` returns false when the surface is lost — normal
every time the app is backgrounded. Do not ignore it; stop drawing and wait for
a new window.

## Before committing

```sh
make            # warning-free
make apk        # packages cleanly
```

Never commit a keystore. `.gitignore` covers `*.keystore` and `*.jks`.
