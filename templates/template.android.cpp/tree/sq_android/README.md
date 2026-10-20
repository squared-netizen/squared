# sq_android — the platform layer

How this project meets Android. Everything here is **yours** (ownership class:
seeded) — the generator wrote it once and will not overwrite it.

This is the expert escape hatch. You should not normally need it.

```text
AndroidManifest.xml    package id, activity, launcher entry
entry.cpp              native_app_glue loop → your App
android_text.hpp/.cpp  the keyboard, what a key types, the clipboard (JNI)
res/values/            label and icon colour
res/drawable/          the icon, as a vector path
res/mipmap-*/          adaptive icon and legacy fallback
```

## `entry.cpp`

Runs the `android_native_app_glue` event loop, translates lifecycle and input
events into calls on your `App`, and brings up a rendering surface if a
rendering kit was applied.

The renderer include is guarded, so this file compiles with no kit at all.
That guard is what makes `kit.opengl` and a future `kit.sfml` alternatives
rather than one of them being baked in.

**What the device lacks is decided from its API level**, read once at start
from the configuration NativeActivity already holds
(`AConfiguration_getSdkVersion`) and logged with the process's name as
`device: API <n>, process <name>` (an app with a `:console` process logs
each separately); the GL
driver is logged once as `driver: <version>; <renderer>; <vendor>` when the
first context is current. Both reach logcat (`make logcat`, through adb), which is
how a device without Termux is read. Each such check costs a comparison,
cheaper than reading a cached answer from storage, so nothing is cached.

**The safe area** (the bars, the camera cut-out, the keyboard) is measured
from the window's insets from API 23. Below that, those methods do not
exist, and the window's visible frame (`getWindowVisibleDisplayFrame`, API 1)
answers instead: some older devices lay the window out under translucent
bars, and the Android 5.1 tablet does (found 2026-10-19: its clock and
battery drew over the Console's `[x]`).

**Typing.** NativeActivity gives native code key codes, never characters.
`android_text.cpp` turns a key press into the character it types through
Java's `KeyCharacterMap` (one JNI call per key press, never per frame), and
shows and hides the soft keyboard. `entry.cpp` hands your `App` a
`sq::app::TextInputService` (`set_text_input_service`): when a text field
takes focus the keyboard shows, and each key arrives as a `KeyDown` event
and, if it types something, a `TextInput` event. What does not arrive: an
input method's composition (accents built from two keys, CJK input, emoji),
which needs a Java `InputConnection`. The volume keys are left to Android.

**The Console** (`make CONSOLE=app`; see squared's `docs/programmer/console.md`).
A three-finger tap anywhere, or `` ` `` on a keyboard, opens and closes it;
the gesture is taken before your `App` sees it (fingers already down are
released far off screen, so nothing is clicked). While it is open, a gesture
that starts on it is its own: tap to type (the keyboard shows), drag to
scroll back, hold the input row for half a second to paste, `[x]` or Back
to close. `copy` puts lines on the clipboard. Every other touch, and every
key while it is closed, reaches your `App` as before.

`app stop` disposes your `App` and puts a fresh, uncreated one in its place;
`app start` creates it and hands it the surface again, as at launch;
`app restart` does both. The process, the window, the graphics context and
the Console stay, so this recovers from a stuck screen or bad state, not from
a crash. While stopped, no method of your `App` is called and the screen is
cleared under the Console. In a Console build a `create()` that fails no
longer leaves a black window: the Console opens and `app start` tries again.
`quit` finishes the activity, the same way your `quit_requested()` does.

**Crash reports.** In a Console build a fatal signal (a segfault, an abort)
writes a report to the app's private files, `files/crash.txt`: the trace,
each frame as a library and an offset for `addr2line`, then the last 40 lines
of the log. The next launch shows it in the Console, in red, once, and moves
it to `crash-previous.txt`; `crash` shows it again. The signal then goes on
to the dev log's handler (with `DEV_LOG`) and to Android's own crash report,
so nothing else is lost.

## `AndroidManifest.xml`

Two deliberate absences:

**No `<uses-sdk>`.** `minSdkVersion` and `targetSdkVersion` are passed to
`aapt2` by the build, so `make apk SQ_MIN_SDK=26` retargets without editing a
file. Declaring them here too would create two sources of truth, and aapt2
would take this one. `SQ_MIN_SDK` (default 22) is also what the code is
compiled and linked for, so the manifest cannot claim a level the library was
not built for.

**No `android:debuggable`.** The build sets it for `DEV_LOG` and `CONSOLE`
builds (`aapt2 link --debug-mode`, read back from the linked manifest to
check), so `run-as` reaches their private files; plain builds never carry it.
`SQ_DEBUGGABLE=` turns it off for one build, `SQ_DEBUGGABLE=1` on. Committing
it here would ship a debuggable release.

`android:hasCode="false"` and `android.app.NativeActivity` are what let this
project have no Java: the APK carries no `classes.dex`, and the build needs a
JDK only for signing.

## The icon

`res/drawable/ic_launcher_foreground.xml` is a vector path — text, reviewable
in a diff, editable without an image editor. `res/mipmap-hdpi/ic_launcher.png`
is the legacy fallback for API < 26 and draws the same shape.

Replace both when you have a real icon.
