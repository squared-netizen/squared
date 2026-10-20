# {{project_name}} — build.
#
# This file is YOURS (ownership class: seeded). The generator wrote it once and
# will not overwrite it. Edit it freely.
#
# The parts the generator owns live in mk/, are included below, and are
# rewritten on update. That split is deliberate: you get a build file you can
# change without fear, and the generator gets files it can regenerate without
# destroying your work.
#
#   make                 build lib{{project_name}}.so
#   make apk             package and sign an installable APK
#   make install         install it: with adb on this phone, no prompt; else the
#                        package installer
#   make run             install, then start it (needs adb)
#   make logcat          its log, and Android's crash lines (needs adb)
#   make devices         what adb reaches, and how to connect it to this phone
#   make status          what the build found on this host
#   make squared-core    build the vendored framework
#   make android-help    every target and override
#   make clean
#
#   make DEV_LOG=1 apk   a development build that sends its log lines, and a
#                        crash report, to UDP port 5555 on this device
#
# DEV_LOG and CONSOLE builds are debuggable, so `run-as` reaches their private
# files; plain builds never are (SQ_DEBUGGABLE in mk/squared_android_package.mk).
#
# Overridable, e.g. `make SQ_MIN_SDK=24 OPT=-O3 apk`. SQ_MIN_SDK is what the
# library is compiled and linked for (default 22, Android 5.1), not only what
# the manifest says.

CXX      ?= clang++
CC       ?= clang
CXXSTD   ?= c++20
OPT      ?= -O2
WARN     ?= -Wall -Wextra -Wpedantic

BUILD    ?= build

# `make` with no target builds. Stated explicitly because the included
# fragments define targets of their own, and make's default goal is the first
# target it *sees*.
.DEFAULT_GOAL := all

# Your own settings, kept out of git (.gitignore lists it): what you would
# otherwise type on every make. For example, to make every build a Console
# build with the dev log until you delete the lines:
#
#   CONSOLE := app
#   DEV_LOG := 1
#
# Read before everything else, so a value given on the command line still wins.
-include local.mk

include mk/squared_generated.mk
-include $(wildcard mk/pkg_*.mk)
-include $(wildcard mk/kit_*.mk)

# Development log sink. Off unless asked for: `make DEV_LOG=1 apk`.
#
# The app sends each log line, and a backtrace if it crashes, to
# 127.0.0.1:$(DEV_LOG_PORT). Read them on the same device, in Termux:
#
#   nc -klu 5555
#
# Android refuses sockets without the INTERNET permission, so a DEV_LOG build
# uses a copy of the manifest with that one line added. Your manifest is not
# edited, and a normal build never carries the permission. Resolve a crash
# address with: addr2line -f -C -e build/lib$(SQ_PROJECT).so <offset>
ifdef DEV_LOG
DEV_LOG_PORT ?= 5555
SQ_CPPFLAGS  += -DSQUARED_DEV_LOG=$(DEV_LOG_PORT)
SQ_MANIFEST  := $(BUILD)/dev/AndroidManifest.xml

$(SQ_MANIFEST): sq_android/AndroidManifest.xml
	@mkdir -p $(dir $@)
	@sed 's|^\([[:space:]]*\)<application\([[:space:]]*\)$$|\1<uses-permission android:name="android.permission.INTERNET" />\n\1<application\2|' \
	    $< > $@
	@grep -q 'permission.INTERNET' $@ || { \
	  echo "DEV_LOG: could not add the INTERNET permission: no '<application' line" >&2; \
	  rm -f $@; exit 1; }
endif

SQ_SO := $(BUILD)/lib$(SQ_PROJECT).so

# Your code, plus the platform layer, plus the NDK's native-app-glue. The build
# globs; there is no source list to keep in sync.
APP_SRC  := $(shell find sq_app/src -name '*.cpp' 2>/dev/null | sort)
PLAT_SRC := $(shell find sq_android -maxdepth 1 -name '*.cpp' 2>/dev/null | sort)
CXX_SRC  := $(APP_SRC) $(PLAT_SRC) $(SQ_KIT_SRC)

CXX_OBJ  := $(patsubst %.cpp,$(BUILD)/%.o,$(CXX_SRC))
GLUE_OBJ := $(BUILD)/glue/android_native_app_glue.o
OBJ      := $(CXX_OBJ) $(GLUE_OBJ)
DEPS     := $(CXX_OBJ:.o=.d)

CPPFLAGS += $(SQ_CPPFLAGS) $(SQ_KIT_CPPFLAGS)
CXXFLAGS += -std=$(CXXSTD) $(OPT) $(WARN) -fPIC -fvisibility=hidden
# An unresolved symbol is a link error here, not a library that installs and then
# fails to load on the device with "cannot locate symbol". (Switch it off with
# `make SQ_NO_UNDEFINED= apk` if a library of yours resolves symbols at load.)
SQ_NO_UNDEFINED ?= -Wl,--no-undefined
LDFLAGS  += $(SQ_LDFLAGS) $(SQ_KIT_LDFLAGS) $(SQ_NO_UNDEFINED)
LDLIBS   += $(SQ_LDLIBS) $(SQ_KIT_LDLIBS)

include mk/squared_android_package.mk

# What the objects were built with.
#
# make compares timestamps, not flags. Without this, objects compiled for a
# DEV_LOG=1 build were reused by the next plain build, which then linked
# without the dev library and failed on a symbol that was only ever meant to
# exist in the other kind of build. The stamp is rewritten only when the flags
# change; every object, and the packaged resources (whose manifest differs
# between the two kinds of build), depend on it.
SQ_FLAGS_STAMP := $(BUILD)/.flags
$(shell mkdir -p '$(BUILD)'; [ "$$(cat '$(SQ_FLAGS_STAMP)' 2>/dev/null)" = '$(CPPFLAGS) $(CXXFLAGS)' ] || printf '%s' '$(CPPFLAGS) $(CXXFLAGS)' > '$(SQ_FLAGS_STAMP)')
$(SQ_APK_DIR)/base.apk: $(SQ_FLAGS_STAMP)

.PHONY: all clean help
all: require-ndk squared-core $(SQ_SO)

# The C++ runtime goes last: everything before it, the framework's archives
# included, may call into it.
$(SQ_SO): $(OBJ) $(SQ_SQUARED_LIBS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -shared $(OBJ) $(LDFLAGS) $(LDLIBS) $(SQ_CXX_RUNTIME) -o $@
	@$(MAKE) --no-print-directory verify-link

# Report what the linker actually bound, and refuse a library the oldest
# device it is built for could not load.
#
# Three checks, each for a failure that once got as far as the device:
#
#   GLES       below.
#   runtime    libc++_shared.so among the needed libraries means the C++
#              runtime was not linked in (see SQ_CXX_RUNTIME), and the APK
#              would carry none: it fails to load on every device.
#   DT_HASH    the loader before Android 6.0 reads only this symbol table. A
#              library without it fails there with "empty/missing DT_HASH";
#              the CDDA build on the Android 5.1 tablet did exactly that.
#
# A call to a function newer than SQ_MIN_SDK needs no check here: it fails the
# link itself, against that level's stubs, under -Wl,--no-undefined.
#
# On Termux, $PREFIX/lib/libEGL.so belongs to libglvnd — the X11 EGL, not
# Android's — and a plain -lEGL picks it over the NDK stub. The link succeeds
# either way and the APK dies at runtime with a message that names nothing
# useful.
#
# kit.opengl now defends against that structurally, by passing the NDK's
# libEGL.so and libGLESv3.so as absolute paths rather than as -l names: a
# search order cannot go wrong when there is no search. This target reports the
# result, and fails only if a GLES library was bound without the kit having
# supplied a path for it — which would mean something reintroduced -lEGL.
.PHONY: verify-link
verify-link:
	@needed=$$(readelf -d $(SQ_SO) 2>/dev/null | sed -n 's/.*NEEDED.*\[\(.*\)\]/\1/p' | tr '\n' ' '); \
	case "$$needed" in \
	  *libEGL*|*libGLES*) \
	    if [ -z "$(strip $(SQ_GL_LIBS))" ]; then \
	      echo "" >&2; \
	      echo "link check: a GLES library was bound without an explicit NDK path." >&2; \
	      echo "  On Termux that is probably \$$PREFIX/lib/libEGL.so from libglvnd," >&2; \
	      echo "  which is the X11 EGL. The APK would install and fail at runtime." >&2; \
	      exit 1; \
	    fi ;; \
	esac; \
	case " $$needed " in \
	  *" libc++_shared.so "*) \
	    echo "" >&2; \
	    echo "link check: lib$(SQ_PROJECT).so needs libc++_shared.so, which the APK does not carry." >&2; \
	    echo "  The C++ runtime should be linked in; is SQ_CXX_RUNTIME empty? (make android-status)" >&2; \
	    exit 1 ;; \
	esac; \
	if [ "$(SQ_MIN_SDK)" -lt 23 ] && ! readelf -d $(SQ_SO) 2>/dev/null | grep -q '(HASH)'; then \
	  echo "" >&2; \
	  echo "link check: lib$(SQ_PROJECT).so has no DT_HASH table; Android before 6.0 cannot load it." >&2; \
	  echo "  The link needs -Wl,--hash-style=both (SQ_LDFLAGS in mk/squared_generated.mk)." >&2; \
	  exit 1; \
	fi; \
	echo "  link check     $$needed"; \
	echo "                 built for API $(SQ_MIN_SDK); C++ runtime linked in$$(readelf -d $(SQ_SO) 2>/dev/null | grep -q '(HASH)' && echo '; DT_HASH present')"

$(BUILD)/%.o: %.cpp $(SQ_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

# The native-app-glue is compiled from the NDK rather than vendored into the
# project: it is part of the platform, it is versioned with the NDK, and a
# stale copy in your tree would be a bug nobody goes looking for.
$(GLUE_OBJ): $(SQ_GLUE_SRC) $(SQ_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	@if [ ! -f "$(SQ_GLUE_SRC)" ]; then \
	  echo "native-app-glue not found at $(SQ_GLUE_SRC)" >&2; \
	  echo "run 'make android-status' to see what the build found" >&2; \
	  exit 1; \
	fi
	$(CC) $(SQ_CPPFLAGS) $(OPT) -fPIC -c $< -o $@

clean:
	rm -rf $(BUILD)

help: android-help

status: android-status squared-core-status

-include $(DEPS)
