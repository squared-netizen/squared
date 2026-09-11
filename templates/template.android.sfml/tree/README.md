# {{project_name}}

{{description}}

An Android application in C++20, with SFML as the platform layer. No Java, no
Gradle, no Android Studio. `make apk` produces a signed, installable APK using
clang, aapt2 and apksigner.

## Layout

| Directory | Yours? | What it is |
|---|---|---|
| `sq_app/` | yes | your application. No Android header, no SFML header. |
| `sq_android/` | yes | the platform layer: window, event loop, lifecycle. |
| `sq_kit/` | generated | SFML headers and archives, from kit.sfml. |
| `mk/` | generated | build settings the generator rewrites on update. |
| `Makefile` | yes | yours to edit; it includes `mk/`. |

Write code in `sq_app/`. You will rarely need to open `sq_android/`, but it is
plain C++ when you do.

## Build

```sh
make                 # build lib{{project_name}}.so
make apk             # package and sign
make install         # hand it to the package installer
make android-status  # what the build found on this host
make android-help    # every target and override
```

## Why SFML owns the entry point

SFML's Android backend defines `ANativeActivity_onCreate` itself, installs its
own activity callbacks, and runs your `main()` on a thread it owns. There is one
such entry point per process, so SFML and `android_native_app_glue` cannot both
be present — which is why this is a template rather than a kit applied to
`template.android.cpp`.

The practical consequence is in `sq_android/main.cpp`, and it is worth knowing:
your code does not run on Android's UI thread. One thread throughout, so `App`
needs no locking, but it is SFML's thread, not the platform's.

## Portability

`sq_app/` compiles unchanged under `template.android.cpp`, which reaches the
same GLES 3.0 through NativeActivity and `kit.opengl`. That is a property worth
keeping: it means the decision about how your project meets Android is
reversible. Including an SFML header in `sq_app/` gives it up.

## Requires

`kit.sfml`, which carries SFML's headers and its static archives built for this
ABI. The template will not generate without it.
