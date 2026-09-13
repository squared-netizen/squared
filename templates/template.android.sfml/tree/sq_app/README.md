# sq_app — your application

Yours (ownership class: seeded). The generator wrote these files once and will
not overwrite them.

## What you get

An `sf::RenderTarget` per frame, a live GLES 3.0 context, and the five SFML
modules kit.sfml ships: System, Window, Graphics, Audio, and the activity entry
point.

`App` has seven methods the platform layer calls:

| Method | When |
|---|---|
| `start()` | once, after the window exists |
| `stop()` | once, at shutdown |
| `resume()` / `pause()` | foreground / background |
| `resize(w, h)` | at startup and on every change |
| `render(target)` | every frame |
| `touch(phase, x, y)` | on touch |

## Use SFML here

That is what this layer is for. `#include <SFML/Graphics.hpp>` and
`<SFML/Audio.hpp>` in your `.cpp` files.

`app.hpp` forward-declares `sf::RenderTarget` rather than including the SFML
headers, because `<SFML/Graphics.hpp>` is large and every file including
`app.hpp` would pay for it. Keep that pattern as you add members: the private
`State` struct lives in the `.cpp`, so adding to it rebuilds one file.

## Do not use `<android/...>` here

Except `<android/log.h>`, which the demo uses and which is hard to regret. SFML
already abstracts the platform; reaching past it gives up the portability that
makes this code worth keeping.

Anything that genuinely needs the Android API belongs in `sq_android/main.cpp`,
which has `sf::getNativeActivity()` and the whole NDK available.

## Assets

`sq_android/assets/` is packaged into the APK. Load through SFML —
`sf::Texture::loadFromFile`, `sf::Font::openFromFile`, `sf::Music::openFromFile`
— which reaches Android's AAssetManager.

Open by bare name relative to the assets root — `sq_android/assets/font.ttf`
is `"font.ttf"`, `sq_android/assets/ui/panel.png` is `"ui/panel.png"`.
Subdirectories survive; no prefix is wanted.
