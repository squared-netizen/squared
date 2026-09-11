# Working in this project

Instructions for AI assistants, and a summary for humans.

## The shape

- `sq_app/` is the application. It includes **no Android header and no SFML
  header**. It draws through GLES 3.0 and nothing else.
- `sq_android/main.cpp` is the platform layer: it defines `main()`, opens the
  `sf::Window`, pumps events and calls into `App`. Everything platform-specific
  belongs here.
- `mk/` is generated. Edits there are lost on update. Build settings go in
  `Makefile`, which is yours.

## Rules that are not style preferences

**Do not include `<SFML/...>` from `sq_app/`.** The application layer is
portable between this template and `template.android.cpp`; an SFML include
ends that. If you need SFML, the code belongs in `sq_android/main.cpp`.

**Do not add `android_native_app_glue`.** SFML supplies the activity entry
point. Linking both puts two definitions in one slot.

**Do not remove `-Wl,-u,ANativeActivity_onCreate` from the link.** Nothing
references that symbol, so the linker drops it and the APK dies at launch with
nothing in the log to say why. `make` checks for it and fails the build if it
is missing.

**`App` methods must not block.** They run on SFML's thread, one at a time. A
blocked call stops drawing and stops draining events.

## Adding a dependency

Kits contribute build flags through `mk/kit_*.mk`, which the Makefile picks up
by wildcard. Adding a library by hand means editing `Makefile`, not `mk/`.
