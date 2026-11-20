# native_app_glue

The NDK's `sources/android/native_app_glue` (`android_native_app_glue.c` and
`.h`), Apache-2.0 (`NOTICE`), as NDK r27c (27.3.13750724) ships it, copied
unchanged on 2026-11-20. It has changed little since 2010.

An Android project built with the SDK's own toolchain (`SQ_TOOLCHAIN=1`:
clang 23 with sqterm's `sysroot` and `runtimes` packages) compiles this copy,
so that build needs no NDK ("One C++23 toolchain", phase 4). An NDK build
(`SQ_TOOLCHAIN=0`, the template's default for now) uses the NDK's own copy.
