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

# Your code plus the platform layer. The build globs; there is no source list
# to keep in sync.
#
# No native-app-glue object: SFML supplies the activity entry point, so there
# is no glue to compile. See mk/squared_generated.mk.
APP_SRC  := $(shell find sq_app/src -name '*.cpp' 2>/dev/null | sort)
PLAT_SRC := $(shell find sq_android -maxdepth 1 -name '*.cpp' 2>/dev/null | sort)
CXX_SRC  := $(APP_SRC) $(PLAT_SRC)

OBJ      := $(patsubst %.cpp,$(BUILD)/%.o,$(CXX_SRC))
DEPS     := $(OBJ:.o=.d)

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

# Report what the linker actually bound, and catch the two failures that
# otherwise surface only on a device.
#
# 1. libglvnd. On Termux $PREFIX/lib/libEGL.so is the X11 EGL, and a plain
#    -lEGL picks it over the NDK stub. The link succeeds and the APK dies at
#    runtime naming nothing useful. kit.sfml defends against this structurally
#    by passing the NDK libraries as absolute paths, so a search order cannot
#    go wrong when there is no search. This reports the result.
#
# 2. The missing entry point. If ANativeActivity_onCreate was not pulled out of
#    libsfml-main.a, everything above still succeeds and the app dies at
#    launch. One nm call settles it, so there is no reason to find out on the
#    device.
.PHONY: verify-link
verify-link:
	@if ! nm -D --defined-only $(SQ_SO) 2>/dev/null | grep -q ANativeActivity_onCreate; then \
	  echo "" >&2; \
	  echo "link check: ANativeActivity_onCreate is not exported by $(SQ_SO)." >&2; \
	  echo "  Android resolves that symbol by name when it loads the library;" >&2; \
	  echo "  without it the APK installs and dies at launch." >&2; \
	  echo "  It lives in libsfml-main.a and nothing references it, so the" >&2; \
	  echo "  linker drops it unless SQ_LDFLAGS carries -Wl,-u,ANativeActivity_onCreate." >&2; \
	  exit 1; \
	fi
	@needed=$$(readelf -d $(SQ_SO) 2>/dev/null | sed -n 's/.*NEEDED.*\[\(.*\)\]/\1/p' | tr '\n' ' '); \
	case "$$needed" in \
	  *libEGL*|*libGLES*) \
	    if [ -z "$(strip $(SQ_SFML_GL_LIBS))" ]; then \
	      echo "" >&2; \
	      echo "link check: a GLES library was bound without an explicit NDK path." >&2; \
	      echo "  On Termux that is probably \$$PREFIX/lib/libEGL.so from libglvnd," >&2; \
	      echo "  which is the X11 EGL. The APK would install and fail at runtime." >&2; \
	      exit 1; \
	    fi ;; \
	esac; \
	echo "  link check     entry point ok; $$needed"

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf $(BUILD)

help: android-help

-include $(DEPS)
