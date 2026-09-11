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
# Why NDK detection lives here and not in kit.sfml: the NDK sysroot, the
# platform stub libraries and libc++_shared are this template's concern. They
# are what make the output an Android project at all, and they would be needed
# whatever library drew the pixels. kit.sfml consumes SQ_NDK_LIB; it does not
# locate it.
#
# Absent here, and deliberately: android_native_app_glue. SFML supplies
# ANativeActivity_onCreate from sfml-main, so the glue is not merely unused --
# linking both would put two definitions in the one entry-point slot.

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

SQ_MIN_SDK    ?= 24
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
# the platform stubs (libEGL, libandroid, liblog); libc++_shared.so sits one
# level up, in the ABI directory.
SQ_NDK_LIB     := $(SQ_SYSROOT)/usr/lib/$(SQ_LIB_TRIPLE)/$(SQ_MIN_SDK)
SQ_NDK_LIB_ABI := $(SQ_SYSROOT)/usr/lib/$(SQ_LIB_TRIPLE)
SQ_LIBCXX      := $(SQ_NDK_LIB_ABI)/libc++_shared.so

# With no NDK, every path above degrades into something like
# "/sources/android/native_app_glue" and the compiler reports a missing header
# five levels deep in an include chain. `all` depends on this target so the
# build says what is actually wrong, once, at the top.
.PHONY: require-ndk
require-ndk:
	@if [ -z "$(strip $(SQ_NDK))" ] || [ ! -d "$(SQ_NDK)" ]; then 	  echo "" >&2; 	  echo "No Android NDK found." >&2; 	  echo "  Searched: \$$ANDROID_NDK_HOME, \$$PREFIX/opt/android-sdk/ndk/*," >&2; 	  echo "            \$$ANDROID_HOME/ndk/*, \$$ANDROID_SDK_ROOT/ndk/*" >&2; 	  echo "  Termux:   pkg install ndk-sysroot   (or set SQ_NDK=/path/to/ndk)" >&2; 	  echo "  See 'make android-status' for everything the build looked for." >&2; 	  exit 1; 	fi

# ---------------------------------------------------------------------------
# Compiler and link flags
#
# The NDK's include directory is deliberately NOT on the search path here.
#
# Termux's clang already targets Android and ships every android/* header in
# its own sysroot, so the NDK is needed only for the Khronos headers Termux
# lacks (EGL/, GLES3/, KHR/) -- which is kit.opengl's business, not the
# template's.
#
# Putting the NDK sysroot on -I breaks the build outright: it places a second
# complete set of C headers ahead of libc++, and <cctype> then finds the NDK's
# ctype.h instead of libc++'s wrapper. libc++ detects this and stops with
# "tried including <ctype.h> but didn't find libc++'s". kit.opengl uses
# -idirafter for the same reason: it appends to the *end* of the search path,
# so the NDK supplies only headers nothing else provides.
#
# No NDK include directory is added here at all. kit.sfml brings SFML's own
# headers, and SFML reaches EGL and GLES through paths the kit passes.
#
# __ANDROID_API__ is not defined either: the toolchain already defines it from
# its target triple, and redefining it is both a warning and a chance to
# disagree with the compiler about what it is building for.

SQ_CPPFLAGS := -I$(SQ_WORKING_DIR)/include -DANDROID

# -u ANativeActivity_onCreate is load-bearing, and its absence is invisible.
#
# Android's runtime looks that symbol up by name when it loads the library.
# Nothing in this project references it, and it lives inside libsfml-main.a, so
# a static linker drops the archive member that defines it. The link then
# succeeds, the .so is well formed, the APK installs -- and the process dies at
# launch before any of your code runs, with nothing in the log naming the
# cause.
#
# -u forces the symbol to be treated as undefined, which pulls the member in.
# SFML's own build does exactly this; see src/SFML/Main/CMakeLists.txt.
SQ_LDFLAGS  := -Wl,-u,ANativeActivity_onCreate
SQ_LDLIBS   := -landroid -llog

# Accumulated by the kit fragments; declared here so the Makefile can reference
# them whether or not any kit was applied.
SQ_KIT_CPPFLAGS ?=
SQ_KIT_LDFLAGS  ?=
SQ_KIT_LDLIBS   ?=
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
	@echo "abi        $(SQ_ABI)  (min sdk $(SQ_MIN_SDK), target $(SQ_TARGET_SDK))"
	@echo "kits       $(SQ_KITS_PRESENT)"
