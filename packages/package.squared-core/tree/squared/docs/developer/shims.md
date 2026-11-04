# Shims: generated classes Android calls into

> [!important] The rule for every shim
> All behaviour stays in C++. The dex only tells Android "these classes exist, and their methods live in that library."

Programmer counterpart: [../programmer/tools.md](../programmer/tools.md#dexgensh) (the tools only; shims have no API of their own).

## Why they exist

JNI lets C++ call any Android API. It cannot give Android a class of ours to call into: a second activity, a service, a receiver. Those must be classes in a dex. squared generates them instead of compiling Java: no javac, no d8, no JDK (Sprint 4, decided 2026-10-15; see the app-model spec, "Generated shims").

A shim method is one of two kinds, and `dexgen` can express nothing else:

| Kind | Bytecode | Use |
|---|---|---|
| constructor | `invoke-direct {v0}, Parent.<init>()V` then `return-void`: 4 code units | every shim |
| native | none; bound by `RegisterNatives` from C++ | every behaviour a shim has |

A shim that would need a branch, a loop or a field is the wrong shim: its logic moves into C++ behind a native method. The service was expected to need a third kind, a static initializer that calls `System.loadLibrary`; it does not (12.6): its natives are registered from C++ as the activity is made, through the activity's class loader (the app's; `FindClass` there would ask NativeActivity's, the system's). The dex then names no app's library, and stays one dex for every app. The cost: a service Android started by itself, in a process where no activity has registered them, would find no natives; so it is never sticky (`START_NOT_STICKY`), and only the app starts it.

## The shims

| Class | Extends | Methods | For |
|---|---|---|---|
| `dev.squared.ConsoleActivity` | `android.app.NativeActivity` | the constructor | the monitor process, `CONSOLE=monitor` (4.7). `NativeActivity` reads `android.app.lib_name` from the activity's own manifest entry, so a second entry with its own `android:process` and library needs no code |
| `dev.squared.ModelService` (12.6) | `android.app.Service` | the constructor; native `onStartCommand(Intent, int, int)`, `onBind(Intent)`, `onDestroy()` | running in the background (`app.keep_running`): the template's `sq_android/background.cpp` registers the three and does everything (the notification, the wake lock, Exit). Packed by `BACKGROUND=1`, with the manifest entry |

The names are fixed and do not depend on the app's package, so the dex is the same for every app: generated once, committed as `tools/dexgen/shims.dex`.

## The file

`tools/dexgen/shims.dex`: 812 bytes since 12.6 (432 with the Console's activity alone), dex version 035, which every Android release from 5.0 reads. sha256 `079d5dae1775a6335813898224e8926bb94930fce2b08cd300b6380ef79113cd`.

| Section | Contents |
|---|---|
| header | 0x70 bytes: magic `dex\n035\0`, Adler-32 of everything after it, SHA-1 of everything after the signature, sizes and offsets |
| string ids | the method names, the class and parameter descriptors, the shorties |
| type ids | the classes, their parents, the parameter and return types |
| proto ids | `()V`, `(Intent)IBinder`, `(Intent, int, int)int`, ordered as the format requires |
| method ids | the parents' `<init>` and the shims' methods |
| class defs | 2: public, no interfaces, no source file, no annotations |
| data | two code items (the constructors: registers 1, ins 1, outs 1), a type list, the string data, the class data (the natives have no code), the map |

## How `dexgen` writes it

`tools/dexgen/dexgen.cpp`, one file, written from the published format (source.android.com, "Dalvik executable format"). A host tool: the standard containers are used freely, as the framework rules allow outside resident code.

1. **Collect** every string, type, prototype and method reference the shim table names.
2. **Sort** each table the way the format requires and Android's verifier checks: strings by code point (all ASCII here, so byte order), types by string index, prototypes by return type then parameters, methods by class, name, prototype; class definitions by descriptor (a shim's parent is a framework class, so no definition order is forced).
3. **Lay out**: header, then the id tables in their fixed order, so the data section's offset is known before any of it is written; then code items (4-aligned), type lists (4-aligned), string data, class data, and the map last.
4. **Sign**: SHA-1 over everything after the signature field, then Adler-32 over everything after the checksum field, signature included.

`dexgen --dump` reads any dex back: it verifies the checksum, signature, size and endian tag, and prints the strings, methods, classes, flags, register counts and every instruction (`invoke-direct`, `invoke-static` and `return-void`; anything else is named by opcode).

## How it is checked

- **Byte for byte.** `tools/dexgen.sh` regenerates the dex and compares it with the committed file. Identical from clang 18 and g++ 13 here, and expected from clang 21 on the phone: nothing in it depends on the compiler. Since C++23 `dexgen.cpp` is `import std;`: the script builds it through the framework's make (`make dexgen`, as the tests are), which gives it the std module; until 12.6 it compiled it alone, and failed.
- **Independently.** The checksum and signature were recomputed with Python's `zlib` and `hashlib`, and the file was parsed by androguard 3.3.5's `DalvikVMFormat`, a dex reader that shares no code with `dexgen`: one class, the right parent, flags `0x10001` (public constructor), registers 1, `invoke-direct` then `return-void`. androguard is not part of the tree; it was the sandbox's check. The 12.6 dex was read back by a second small reader written for it in Python (no network for androguard that day): the checksums, sorted string and method tables, both classes and their parents, the constructors' code items, the three natives with `0x101` (public native) and no code.
- **The paths no shipped shim uses yet.** `tools/dexgen.sh --self-test` builds `dexgen` with a test service (three native methods, parameter lists, a second class), writes it and reads it back; androguard parsed that file too.
- **On the device**: Android's verifier is the last word on the file. 12.6 is the first time a dex reaches one (sqterm, `BACKGROUND=1`): the 4.0 spike with `ConsoleActivity` was never run, the Console having moved into the app's own process first.

## What it costs

Nothing in a build without a shim: a build without `BACKGROUND=1` ships no dex and keeps `android:hasCode="false"`. With it: 812 bytes in the APK; ART compiles the dex on install (a few KB); the APK gets a class loader, as every Java app has. The C++ is untouched: Android does nothing more to machine code.

## Adding a shim

1. Add it to `shims()` in `tools/dexgen/dexgen.cpp`, as a constructor plus native methods.
2. `tools/dexgen.sh --write`, then `tools/dexgen.sh --dump` to read what you wrote.
3. Commit the dex with the change. The byte-for-byte check then guards it.
4. Register its natives from C++ (`RegisterNatives`) before Android can call them: through the activity's class loader, as `sq_android/background.cpp` does for `ModelService`.
