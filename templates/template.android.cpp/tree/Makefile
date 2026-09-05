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
#   make install         hand it to the package installer
#   make android-status  what the build found on this host
#   make android-help    every target and override
#   make clean
#
# Overridable, e.g. `make SQ_MIN_SDK=21 OPT=-O3 apk`.

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

include mk/squared_generated.mk
-include $(wildcard mk/kit_*.mk)

SQ_SO := $(BUILD)/lib$(SQ_PROJECT).so

# Your code, plus the platform layer, plus the NDK's native-app-glue. The build
# globs; there is no source list to keep in sync.
APP_SRC  := $(shell find sq_app/src -name '*.cpp' 2>/dev/null | sort)
PLAT_SRC := $(shell find sq_android -maxdepth 1 -name '*.cpp' 2>/dev/null | sort)
CXX_SRC  := $(APP_SRC) $(PLAT_SRC)

CXX_OBJ  := $(patsubst %.cpp,$(BUILD)/%.o,$(CXX_SRC))
GLUE_OBJ := $(BUILD)/glue/android_native_app_glue.o
OBJ      := $(CXX_OBJ) $(GLUE_OBJ)
DEPS     := $(CXX_OBJ:.o=.d)

CPPFLAGS += $(SQ_CPPFLAGS) $(SQ_KIT_CPPFLAGS)
CXXFLAGS += -std=$(CXXSTD) $(OPT) $(WARN) -fPIC -fvisibility=hidden
LDFLAGS  += $(SQ_LDFLAGS) $(SQ_KIT_LDFLAGS)
LDLIBS   += $(SQ_LDLIBS) $(SQ_KIT_LDLIBS)

include mk/squared_android_package.mk

.PHONY: all clean help
all: require-ndk $(SQ_SO)

$(SQ_SO): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -shared $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@
	@$(MAKE) --no-print-directory verify-link

# Report what the linker actually bound.
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
	echo "  link check     $$needed"

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

# The native-app-glue is compiled from the NDK rather than vendored into the
# project: it is part of the platform, it is versioned with the NDK, and a
# stale copy in your tree would be a bug nobody goes looking for.
$(GLUE_OBJ): $(SQ_GLUE_SRC)
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

-include $(DEPS)
