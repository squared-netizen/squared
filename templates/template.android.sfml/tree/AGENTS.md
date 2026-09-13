# Working in this project

Instructions for AI assistants, and a summary for humans.

## The shape

- `sq_app/` is the application. It draws through SFML — `sf::Texture`,
  `sf::Font`, `sf::Text`, `sf::Sound` and the rest are all available and all
  intended to be used here.
- `sq_android/main.cpp` is the platform layer: it defines `main()`, opens the
  `sf::RenderWindow`, pumps events and calls into `App`. Everything about how
  this project meets Android is here.
- `sq_android/assets/` is packaged into the APK. SFML reads it through
  Android's AAssetManager; nothing else reaches a running app.
- `mk/` is generated. Edits there are lost on update. Build settings go in
  `Makefile`, which is yours.

## Rules that are not style preferences

**`sq_app/` may use SFML. It may not use `<android/...>`.** An earlier version
of this template forbade SFML here too, for portability with
`template.android.cpp`. That was withdrawn: it made the modules this template
exists to provide unreachable from the only place application code lives.
Android headers are still worth avoiding, because SFML already abstracts the
platform and reaching past it gives up what makes the code portable between
devices.

**`App::render` takes an `sf::RenderTarget&`, not an `sf::RenderWindow&`.** The
same drawing code then works against an `sf::RenderTexture` — which is how you
screenshot, or render at a resolution you do not present at.

**Do not add `android_native_app_glue`.** SFML supplies the activity entry
point. Linking both puts two definitions in one slot.

**Do not remove `-Wl,-u,ANativeActivity_onCreate` from the link.** Nothing
references that symbol — Android resolves it by name at load time — so without
`-u` the linker drops it, everything still builds, and the app dies at launch
with nothing in the log. `make` checks for it.

**`App` methods must not block.** They run on SFML's thread, one at a time. A
blocked call stops drawing and stops draining events.

## Assets

Put files in `sq_android/assets/`. `make apk` packages them; `make
android-status` reports how many were found.

Open them by **bare name, relative to the assets root**:

```
sq_android/assets/font.ttf        ->  "font.ttf"
sq_android/assets/ui/panel.png    ->  "ui/panel.png"
```

Subdirectories survive; no prefix is wanted. SFML's Android `FileInputStream`
passes the name unchanged to `AAssetManager_open`, which addresses into the
APK's `assets/` tree (`src/SFML/System/Android/ResourceStream.cpp`).

Assets are read out of the APK in place — nothing is unpacked to storage.

One trap worth knowing: that Android path is taken only when an activity
exists. Without one, `FileInputStream` falls through to an ordinary `fopen`
and silently reads the filesystem instead.

## Diagnosing on a device

`logcat` is not dependable — on some devices `logcat -d` returns nothing to a
Termux process. **Make failures visible on screen.** A project that draws
nothing when one asset is missing tells you nothing; draw the scene
unconditionally and report each failure as a line you can read.
