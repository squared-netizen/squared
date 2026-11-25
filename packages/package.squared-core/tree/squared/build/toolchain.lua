-- build/toolchain.lua: the SDK's toolchain, as data ("One C++23 toolchain",
-- 2026-11-19). sqmake reads it with nothing in reach (values only: squared's
-- docs/developer/sdk-tools.md) and finds each part itself, with no shell:
-- `sqmake toolchain` says what it found and where. The only description of
-- it since 2026-11-25 (build/toolchain.sh, the same in bash for make and
-- sqterm's packages, went with them).
--
-- clang 23, libc++ 23 and bionic: from sqterm's own packages (llvm, runtimes,
-- sysroot) on the phone, from apt.llvm.org's clang 23 with the same runtimes
-- and sysroot on CI. A compiled module (a BMI) is tied to the compiler, its
-- library and the flags that change the language or the target, so nothing
-- here is a project's or a target's to choose.

clang = 23

-- The Android API level, unless $SQ_API says.
api = 28
api_env = "SQ_API"

-- The target: this, then the API level (aarch64-unknown-linux-android28).
triple = "aarch64-unknown-linux-android"

-- On every compile and link, after --target (what changes a BMI):
--   -rtlib=compiler-rt -unwindlib=libunwind
--       not link flags only: they decide target features (outline atomics,
--       function multiversioning), and a BMI made without them is refused
--       by a compile with them (sqterm #21)
--   -pthread
--       clang refuses a BMI made with threads by a compile without, and the
--       other way
--   -D__BIONIC_CTYPE_INLINE=inline
--       the std module exports bionic's ctype functions, which must be plain
--       inline
flags = {"-rtlib=compiler-rt", "-unwindlib=libunwind", "-pthread", "-D__BIONIC_CTYPE_INLINE=inline"}

-- On every C++ compile; libc++'s headers come after these, its
-- __config_site's folder first.
cxx_flags = {"-std=c++23", "-stdlib=libc++", "-nostdinc++"}

-- On every link.
link_flags = {"-fuse-ld=lld", "-static-libstdc++", "-Wl,-z,max-page-size=16384"}

-- Where each part is: its variable, else under $PREFIX (sqterm's), else the
-- default. clang's bin/ must hold clang and clang++ (clang 23); the sysroot
-- usr/include; the runtimes lib/<triple>/libc++.a, include/c++/v1,
-- share/libc++/v*/std.cppm and lib/clang/*/lib.
llvm_bin = {env = "SQ_LLVM_BIN", prefix = "bin", otherwise = "/usr/lib/llvm-23/bin", install = "pkg install llvm"}
sysroot = {env = "SQ_SYSROOT", prefix = "share/sysroot", install = "pkg install sysroot"}
runtimes = {env = "SQ_RUNTIMES", prefix = "", install = "pkg install runtimes"}
