# libc++'s std module, pinned

`import std;` (C++23) is built from these files: libc++'s own module
sources, `std.cppm` and `std/*.inc`, copied from a libc++ installation by
`tools/pin_libcxx_modules.sh` and committed, so a checkout builds without
that installation's copy.

- **Which libc++.** They must match the libc++ headers squared is compiled
  against: the std module names every standard type, and one the headers
  lack stops the build of the module. Pinned from the NDK (r29, libc++
  21.0.0), whose headers are the Android build's and, at the same version,
  Termux's. `PINNED` says where from, the version, and a sha256 for each
  file.
- **Checked.** `make headers` checks the sums; the build refuses headers of
  another libc++ major version than `PINNED` names, and says to pin again.
- **Built once per toolchain.** The module is compiled once for each
  compiler, flag set and libc++, into `~/sqsysroot/.sqpg/std/<key>/` (else
  the build's own `modules/`), and shared by every project
  (`docs/developer/modules.md`).
- **Pinning again**, when the NDK's libc++ moves:

      tools/pin_libcxx_modules.sh <ndk>/toolchains/llvm/prebuilt/<host>/share/libc++/v1

  then build, test, and commit `third_party/libcxx/` with `PINNED`.

License: Apache License v2.0 with LLVM Exceptions (`LICENSE.TXT`), the
LLVM Project's.
