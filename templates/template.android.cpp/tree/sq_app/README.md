# sq_app — your code

The **user working directory**. Everything in it is yours; the generator will
not write here again.

```text
sq_app/src/        your sources — every .cpp here is compiled
sq_app/layouts/    your screens, one JSON file each
sq_app/names.json  your model's addresses, when you have a model
```

## Where it starts

`src/main.cpp` defines `squared_main`, which the framework calls once, when
the first window is up:

```cpp
void squared_main(sq::App& app)
{
    app.run<Prefs>();                    // your model, on its own thread
    app.stage.push(layouts::landing);    // the first screen
}
```

## Screens are layout files

`layouts/landing.json` is the screen `layouts::landing`. The build compiles
every file in `layouts/` (and `names.json`) into C++ before your sources, and
a mistake in one stops the build with the file and the place:
`landing.json: rows[1][0].tap: not a tap`. The format, every key, and the
taps: `squared/docs/programmer/layouts.md`.

A button's `tap` is one line:

| Tap | Does |
|---|---|
| `push settings` | the screen `settings`, over this one |
| `replace settings` | the screen `settings`, instead of this one |
| `back` | back to the screen beneath; at the first screen, leave |
| `quit` | quit: your model saves and stops |
| `set settings.sound false`, `do file.open "a.txt"` | a telegram, to your model |

## The one rule

**No Android headers in this directory.**

No `<android/...>`, no `<EGL/...>`, no `<GLES3/...>`. The platform layer in
`sq_android/` owns all of that and runs your app through `sq::App`. Keeping
Android out of `sq_app/` is what lets the same code build under another
platform layer, and it is easy to lose one include at a time.

## Lifecycle

You write `squared_main` and your model; the App does the rest:

| Android | What happens |
|---|---|
| the first window | `squared_main`, once a process |
| leaving the screen | your model's `background()`: save here, it may never come back |
| coming back | your model's `foreground(elapsed)` |
| short of memory | your model's `low_memory()` |
| back at the first screen, or `quit` | your model saves and stops; the app closes |

Your model runs on its own thread, so nothing in it blocks the screen.
`squared/docs/programmer/stage.md` has the App and the Stage.
