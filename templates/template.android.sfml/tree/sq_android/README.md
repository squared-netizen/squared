# sq_android — the platform layer

Yours (ownership class: seeded). The generator wrote it once and will not
overwrite it.

`main.cpp` is the whole platform layer: it opens the window, pumps SFML events,
maps them onto `App`, and drives the frame loop. `AndroidManifest.xml` and
`res/` are the APK's metadata and resources.

## What is not here

No `android_native_app_glue`, and no `android_main`. SFML's `sfml-main` defines
`ANativeActivity_onCreate` and starts your `main()` on a thread it owns. Under
`template.android.cpp` the opposite is true — that template owns `android_main`
and SFML is absent. The two cannot be combined.

## The thread

Android delivers lifecycle and window callbacks on the UI thread. SFML records
them and runs `main()` on a detached thread. So:

- `App` sees one consistent thread and needs no synchronisation.
- Blocking here will not trigger an ANR the way blocking the UI thread would,
  but it still stops the frame loop and the event queue.
- JNI calls from here need an attached environment. SFML attaches its own; if
  you add your own JNI, attach and detach it yourself.

## AndroidManifest.xml

`android:hasCode="false"` and `android.app.NativeActivity` — there is no Java in
this project and no `classes.dex` in the APK. The `android.app.lib_name`
meta-data must match the library name the build produces.

`<uses-sdk>` is deliberately absent: SDK levels reach aapt2 from
`mk/squared_generated.mk`, so `make apk SQ_MIN_SDK=21` retargets without editing
a file.
