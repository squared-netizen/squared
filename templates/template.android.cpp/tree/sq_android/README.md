# sq_android — the platform layer

How this project meets Android. Everything here is **yours** (ownership class:
seeded) — the generator wrote it once and will not overwrite it.

This is the expert escape hatch. You should not normally need it.

```text
AndroidManifest.xml    package id, activity, launcher entry
entry.cpp              native_app_glue loop → your App
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
first context is current. Both reach logcat (`adb logcat -s <tag>`), which is
how a device without Termux is read. Below API 23 the safe-area query is
skipped: the bars sit outside the window there, and the Java method it calls
does not exist. Each such check costs a comparison, cheaper than reading a
cached answer from storage, so nothing is cached.

## `AndroidManifest.xml`

Two deliberate absences:

**No `<uses-sdk>`.** `minSdkVersion` and `targetSdkVersion` are passed to
`aapt2` by the build, so `make apk SQ_MIN_SDK=26` retargets without editing a
file. Declaring them here too would create two sources of truth, and aapt2
would take this one. `SQ_MIN_SDK` (default 22) is also what the code is
compiled and linked for, so the manifest cannot claim a level the library was
not built for.

**No `android:debuggable`.** Set by the build for debug APKs. Committing it
would ship a debuggable release.

`android:hasCode="false"` and `android.app.NativeActivity` are what let this
project have no Java: the APK carries no `classes.dex`, and the build needs a
JDK only for signing.

## The icon

`res/drawable/ic_launcher_foreground.xml` is a vector path — text, reviewable
in a diff, editable without an image editor. `res/mipmap-hdpi/ic_launcher.png`
is the legacy fallback for API < 26 and draws the same shape.

Replace both when you have a real icon.
