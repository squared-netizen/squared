# Squared generated build settings — Android.
#
# GENERATED FILE — DO NOT EDIT.
#
# The generator owns this path entirely and rewrites it on update (ownership
# class: generated). Anything you write here is lost. Your settings go in the
# Makefile, which is yours and which the generator never touches; every
# variable below uses ?= so overriding one from the command line or the
# Makefile works.
#
#   make SQ_MIN_SDK=21 apk
#
# Why NDK detection lives here and not in a kit: NativeActivity is this
# template's architecture, not a renderer's. A project with no rendering kit
# still needs the NDK headers and the native-app-glue, so putting detection in
# kit.opengl would mean the project could not build without a kit — which is
# exactly what this template promises it can do.

SQ_PROJECT           := {{project_name}}
SQ_PACKAGE           := {{package_name}}
SQ_GENERATOR         := squared-pg
SQ_GENERATOR_VERSION := {{generator_version}}
SQ_TEMPLATE          := {{template_id}}@{{template_version}}
SQ_FRAMEWORK_VERSION := {{framework_version}}
SQ_WORKING_DIR       := {{working_directory}}

# ---------------------------------------------------------------------------
# Target
#
# SDK levels are build settings, not generation parameters. Baking them at
# generation time would mean regenerating the project to retarget it, so they
# live here and reach the APK through aapt2's flags rather than through
# AndroidManifest.xml — which deliberately carries no <uses-sdk>.
# ---------------------------------------------------------------------------

# SQ_MIN_SDK is what the build compiles and links for, not only what the
# manifest claims: the compiler targets that level and links against that
# level's system libraries (see "Compiler and link flags" below), so a call to
# anything newer fails here, at the link, not on the device. 22 is Android 5.1:
# one APK runs on that and on everything after it. Anything newer than the
# minimum is reached at run time, through dlsym, and done without when absent.
SQ_MIN_SDK    ?= 22
SQ_TARGET_SDK ?= 34

# One ABI: the host's. Termux's clang targets aarch64 only, and building four
# ABIs on a phone is not the use case. A desktop NDK can do more -- see
# `make android-help`.
SQ_ABI ?= $(shell getprop ro.product.cpu.abi 2>/dev/null || echo arm64-v8a)

SQ_TRIPLE_arm64-v8a   := aarch64-linux-android
SQ_TRIPLE_armeabi-v7a := armv7a-linux-androideabi
SQ_TRIPLE_x86_64      := x86_64-linux-android
SQ_TRIPLE_x86         := i686-linux-android
SQ_TRIPLE             := $(SQ_TRIPLE_$(SQ_ABI))

SQ_LIB_TRIPLE_arm64-v8a   := aarch64-linux-android
SQ_LIB_TRIPLE_armeabi-v7a := arm-linux-androideabi
SQ_LIB_TRIPLE_x86_64      := x86_64-linux-android
SQ_LIB_TRIPLE_x86         := i686-linux-android
SQ_LIB_TRIPLE             := $(SQ_LIB_TRIPLE_$(SQ_ABI))

# The architecture directory of the NDK clang's own runtime libraries.
SQ_LIB_ARCH_arm64-v8a   := aarch64
SQ_LIB_ARCH_armeabi-v7a := arm
SQ_LIB_ARCH_x86_64      := x86_64
SQ_LIB_ARCH_x86         := i386
SQ_LIB_ARCH             := $(SQ_LIB_ARCH_$(SQ_ABI))

# ---------------------------------------------------------------------------
# NDK
#
# Newest first. ANDROID_NDK_HOME wins when set, because someone who set it
# meant it.
# ---------------------------------------------------------------------------

SQ_NDK ?= $(firstword $(strip $(ANDROID_NDK_HOME)) \
                      $(shell ls -d "$(PREFIX)/opt/android-sdk/ndk"/* \
                                    "$(ANDROID_HOME)/ndk"/* \
                                    "$(ANDROID_SDK_ROOT)/ndk"/* 2>/dev/null | sort -Vr))

SQ_NDK_HOST := $(notdir $(firstword $(wildcard $(SQ_NDK)/toolchains/llvm/prebuilt/*)))
SQ_SYSROOT  := $(SQ_NDK)/toolchains/llvm/prebuilt/$(SQ_NDK_HOST)/sysroot
SQ_NDK_INC  := $(SQ_SYSROOT)/usr/include

# Two library directories, and the distinction matters. The API-level one holds
# the platform stubs (libEGL, libandroid, liblog), one set per Android release;
# the C++ runtime's archives sit one level up, in the ABI directory.
SQ_NDK_LIB     := $(SQ_SYSROOT)/usr/lib/$(SQ_LIB_TRIPLE)/$(SQ_MIN_SDK)
SQ_NDK_LIB_ABI := $(SQ_SYSROOT)/usr/lib/$(SQ_LIB_TRIPLE)

# The C++ runtime, linked into the application's library rather than shipped
# beside it.
#
# An application here is one .so, and for one .so the NDK's advice is the
# static runtime: only the parts of libc++ the program uses are linked, there
# is one library fewer to load and relocate at every start, and nothing on the
# device has to agree with the headers the code was compiled against. That
# last one was a real failure: the shared copy came from wherever the compiler
# found it, an NDK copy once lacked symbols newer headers called
# (std::__hash_memory), and the APK installed and then failed to load with
# "cannot locate symbol".
#
# Named by path, like the GLES libraries in pkg_squared_core.mk. Termux's clang
# always asks for -lc++_shared, which -nostdlib++ (in SQ_LDFLAGS) stops, and
# looks for its unwinder beside itself, which --sysroot hides; the unwinder
# taken here is the NDK clang's own, built for the oldest level the NDK
# supports.
SQ_NDK_UNWIND  := $(firstword $(wildcard $(SQ_NDK)/toolchains/llvm/prebuilt/$(SQ_NDK_HOST)/lib/clang/*/lib/linux/$(SQ_LIB_ARCH)/libunwind.a))
SQ_CXX_RUNTIME := $(SQ_NDK_LIB_ABI)/libc++_static.a $(SQ_NDK_LIB_ABI)/libc++abi.a $(SQ_NDK_UNWIND)

SQ_GLUE_DIR := $(SQ_NDK)/sources/android/native_app_glue
SQ_GLUE_SRC := $(SQ_GLUE_DIR)/android_native_app_glue.c

# With no NDK, every path above degrades into something like
# "/sources/android/native_app_glue" and the compiler reports a missing header
# five levels deep in an include chain. `all` depends on this target so the
# build says what is actually wrong, once, at the top.
.PHONY: require-ndk
require-ndk:
	@if [ -z "$(strip $(SQ_NDK))" ] || [ ! -d "$(SQ_NDK)" ]; then 	  echo "" >&2; 	  echo "No Android NDK found." >&2; 	  echo "  Searched: \$$ANDROID_NDK_HOME, \$$PREFIX/opt/android-sdk/ndk/*," >&2; 	  echo "            \$$ANDROID_HOME/ndk/*, \$$ANDROID_SDK_ROOT/ndk/*" >&2; 	  echo "  Termux:   pkg install ndk-sysroot   (or set SQ_NDK=/path/to/ndk)" >&2; 	  echo "  See 'make android-status' for everything the build looked for." >&2; 	  exit 1; 	fi
	@if [ ! -f "$(SQ_GLUE_SRC)" ]; then 	  echo "" >&2; 	  echo "The NDK at $(SQ_NDK) has no native-app-glue source." >&2; 	  echo "  Expected: $(SQ_GLUE_SRC)" >&2; 	  echo "  This project needs it; it is not vendored, so that a stale copy" >&2; 	  echo "  cannot drift from the NDK it was taken from." >&2; 	  exit 1; 	fi
	@if [ ! -d "$(SQ_NDK_LIB)" ]; then \
	  echo "" >&2; \
	  echo "The NDK at $(SQ_NDK) has no system libraries for API $(SQ_MIN_SDK)." >&2; \
	  echo "  It has: $$(ls '$(SQ_NDK_LIB_ABI)' 2>/dev/null | grep -E '^[0-9]+$$' | sort -n | tr '\n' ' ')" >&2; \
	  echo "  Build for one of those: make SQ_MIN_SDK=<level>" >&2; \
	  exit 1; \
	fi
	@for f in $(SQ_NDK_LIB_ABI)/libc++_static.a $(SQ_NDK_LIB_ABI)/libc++abi.a; do \
	  if [ ! -f "$$f" ]; then \
	    echo "" >&2; \
	    echo "The NDK has no $$f; the C++ runtime is linked from it." >&2; \
	    exit 1; \
	  fi; \
	done
	@if [ -z "$(SQ_NDK_UNWIND)" ]; then \
	  echo "" >&2; \
	  echo "No libunwind.a for $(SQ_LIB_ARCH) under $(SQ_NDK)/toolchains/llvm/prebuilt/$(SQ_NDK_HOST)/lib/clang/" >&2; \
	  echo "  The C++ runtime needs it, and it ships with the NDK's clang." >&2; \
	  exit 1; \
	fi

# ---------------------------------------------------------------------------
# Compiler and link flags
#
# This host's clang, building against the NDK's sysroot. Left to itself,
# Termux's clang builds for Termux: API 24, against Termux's own headers and
# libraries, whatever SQ_MIN_SDK said. Such a library calls functions an older
# Android lacks, and carries only the DT_GNU_HASH symbol table, which the
# loader before Android 6.0 cannot read ("empty/missing DT_HASH").
#
#   --target      the ABI and the API level. The headers hide every function
#                 newer than SQ_MIN_SDK and the stubs define none, so a newer
#                 call fails at the link; clang also picks the code that level
#                 needs (emulated TLS below 29, no RELR relocations below 28).
#   --sysroot     the NDK's headers, startup objects and system libraries, in
#                 place of Termux's. A replacement, not an -I: the NDK's C
#                 headers ahead of libc++'s wrappers was what broke the build
#                 with "tried including <ctype.h> but didn't find libc++'s".
#   -stdlib++-isystem   the NDK's libc++ headers, which are the ones the
#                 runtime above was built from. C++ only, so it goes in
#                 CPPFLAGS, which the C glue's compile does not use.
#   -L            the API level's stubs, first, so nothing else answers.
#   --hash-style=both   DT_HASH for Android before 6.0, DT_GNU_HASH for the
#                 faster lookup everywhere after.
#   --gc-sections the framework is compiled with one section per function;
#                 this drops what nothing calls.
#
# __ANDROID_API__ is not defined: the target triple defines it, and redefining
# it is a warning and a chance to disagree with the compiler.
#
# Only the native-app-glue directory is added to the include path, and it
# holds one header.

SQ_TARGET   := --target=$(SQ_TRIPLE)$(SQ_MIN_SDK) --sysroot=$(SQ_SYSROOT)
SQ_CXX_INC  := -stdlib++-isystem $(SQ_SYSROOT)/usr/include/c++/v1
CPPFLAGS    += $(SQ_CXX_INC)

SQ_CPPFLAGS := $(SQ_TARGET) -I$(SQ_WORKING_DIR)/include -I$(SQ_GLUE_DIR) -DANDROID
SQ_LDFLAGS  := $(SQ_TARGET) -L$(SQ_NDK_LIB) -nostdlib++ --unwindlib=none \
               -Wl,--hash-style=both -Wl,--gc-sections
SQ_LDLIBS   := -landroid -llog

# Accumulated by the kit fragments; declared here so the Makefile can reference
# them whether or not any kit was applied.
SQ_KIT_CPPFLAGS ?=
SQ_KIT_LDFLAGS  ?=
SQ_KIT_LDLIBS   ?=
SQ_KIT_SRC      ?=
SQ_KITS_PRESENT ?=

# ---------------------------------------------------------------------------
# Packaging
# ---------------------------------------------------------------------------

SQ_ANDROID_JAR ?= $(firstword $(shell ls -d \
    "$(PREFIX)/opt/android-sdk/platforms"/android-*/android.jar \
    "$(ANDROID_HOME)/platforms"/android-*/android.jar \
    "$(ANDROID_SDK_ROOT)/platforms"/android-*/android.jar 2>/dev/null | sort -Vr))

SQ_AAPT2     ?= aapt2
SQ_APKSIGNER ?= apksigner
SQ_KEYTOOL   ?= keytool
SQ_STRIP     ?= llvm-strip

SQ_KEYSTORE  ?= $(HOME)/.android/debug.keystore
SQ_KEY_ALIAS ?= androiddebugkey
SQ_KEY_PASS  ?= android

SQ_RES_DIR := sq_android/res
SQ_MANIFEST := sq_android/AndroidManifest.xml

.PHONY: squared-info
squared-info:
	@echo "project    $(SQ_PROJECT)"
	@echo "package    $(SQ_PACKAGE)"
	@echo "generator  $(SQ_GENERATOR) $(SQ_GENERATOR_VERSION)"
	@echo "template   $(SQ_TEMPLATE)"
	@echo "abi        $(SQ_ABI)  (min sdk $(SQ_MIN_SDK), target $(SQ_TARGET_SDK); built for API $(SQ_MIN_SDK))"
	@echo "kits       $(SQ_KITS_PRESENT)"
