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

A shim that would need a branch, a loop or a field is the wrong shim: its logic moves into C++ behind a native method. Sprint 9b's service will need a third kind, a static initializer that calls `System.loadLibrary` (three instructions); it is added with its first user, not before.

## The shims

| Class | Extends | Methods | For |
|---|---|---|---|
| `dev.squared.ConsoleActivity` | `android.app.NativeActivity` | the constructor | the monitor process, `CONSOLE=monitor` (4.7). `NativeActivity` reads `android.app.lib_name` from the activity's own manifest entry, so a second entry with its own `android:process` and library needs no code |

The names are fixed and do not depend on the app's package, so the dex is the same for every app: generated once, committed as `tools/dexgen/shims.dex`.

## The file

`tools/dexgen/shims.dex`: 432 bytes, dex version 035, which every Android release from 5.0 reads. sha256 `48dc3e6068141e0ac8980cd527bd0b3547cf9d0b0a64d4718b50bb7e759c74b9`.

| Section | Contents |
|---|---|
| header | 0x70 bytes: magic `dex\n035\0`, Adler-32 of everything after it, SHA-1 of everything after the signature, sizes and offsets |
| string ids | 4: `<init>`, `Landroid/app/NativeActivity;`, `Ldev/squared/ConsoleActivity;`, `V` |
| type ids | 3: the two classes and `V` |
| proto ids | 1: `()V`, shorty `V` |
| method ids | 2: the parent's `<init>` and the shim's |
| class defs | 1: public, no interfaces, no source file, no annotations |
| data | one code item (registers 1, ins 1, outs 1), the string data, the class data, the map |

## How `dexgen` writes it

`tools/dexgen/dexgen.cpp`, one file, written from the published format (source.android.com, "Dalvik executable format"). A host tool: the standard containers are used freely, as the framework rules allow outside resident code.

1. **Collect** every string, type, prototype and method reference the shim table names.
2. **Sort** each table the way the format requires and Android's verifier checks: strings by code point (all ASCII here, so byte order), types by string index, prototypes by return type then parameters, methods by class, name, prototype; class definitions by descriptor (a shim's parent is a framework class, so no definition order is forced).
3. **Lay out**: header, then the id tables in their fixed order, so the data section's offset is known before any of it is written; then code items (4-aligned), type lists (4-aligned), string data, class data, and the map last.
4. **Sign**: SHA-1 over everything after the signature field, then Adler-32 over everything after the checksum field, signature included.

`dexgen --dump` reads any dex back: it verifies the checksum, signature, size and endian tag, and prints the strings, methods, classes, flags, register counts and every instruction (`invoke-direct`, `invoke-static` and `return-void`; anything else is named by opcode).

## How it is checked

- **Byte for byte.** `tools/dexgen.sh` regenerates the dex and compares it with the committed file. Identical from clang 18 and g++ 13 here, and expected from clang 21 on the phone: nothing in it depends on the compiler.
- **Independently.** The checksum and signature were recomputed with Python's `zlib` and `hashlib`, and the file was parsed by androguard 3.3.5's `DalvikVMFormat`, a dex reader that shares no code with `dexgen`: one class, the right parent, flags `0x10001` (public constructor), registers 1, `invoke-direct` then `return-void`. androguard is not part of the tree; it was the sandbox's check.
- **The paths no shipped shim uses yet.** `tools/dexgen.sh --self-test` builds `dexgen` with a test service (three native methods, parameter lists, a second class), writes it and reads it back; androguard parsed that file too.
- **On the device** (4.0, pending): `tools/shim_spike.sh` builds the project's APK with `ConsoleActivity` added in its own process. Android's verifier is the last word on the file.

## What it costs

Nothing in a build without a shim: `CONSOLE=0` ships no dex and keeps `android:hasCode="false"`. With one: 432 bytes in the APK; ART compiles the dex on install (a few KB); the APK gets a class loader, as every Java app has. The C++ is untouched: Android does nothing more to machine code.

## Adding a shim

1. Add it to `shims()` in `tools/dexgen/dexgen.cpp`, as a constructor plus native methods.
2. `tools/dexgen.sh --write`, then `tools/dexgen.sh --dump` to read what you wrote.
3. Commit the dex with the change. The byte-for-byte check then guards it.
4. Register its natives from C++ (`RegisterNatives`), on the thread that loads the library.
