# squared-core wiring.
#
# This file is the GENERATOR'S (ownership class: generated). It is rewritten on
# update; do not edit it.
#
# It ships with package.squared-core rather than with a template, so every
# template that vendors the framework gets the same wiring instead of keeping
# its own copy to drift. Templates pick it up by globbing mk/pkg_*.mk.
#
# squared-core is vendored, not installed: a project gets its own copy so it
# can be edited to suit. That copy is built once and linked as archives, which
# is why this fragment is include paths plus a link line rather than a source
# list.

# The package installs the framework tree at squared/ in the project root.
SQ_SQUARED_DIR ?= squared

# --- rendering backend ------------------------------------------------------
#
# squared::graphics::Context has one implementation, chosen at link time. Which
# one is decided here, from what the toolchain actually provides, rather than
# from a flag somebody has to remember to set.
#
# The NDK paths come from mk/squared_generated.mk, which already derives them
# for the glue and libc++. Nothing new is computed here; this only asks whether
# GLES is present and reacts.

ifneq ($(strip $(SQ_NDK_LIB)),)
  SQ_GLES_AVAILABLE := $(wildcard $(SQ_NDK_LIB)/libEGL.so)
endif

ifneq ($(strip $(SQ_GLES_AVAILABLE)),)

SQUARED_GRAPHICS_BACKEND := gles

# The platform decides which FileSystem is compiled. On Android that is
# AndroidAssetFileSystem, which reads the APK's assets through AAssetManager;
# without it FileType::Internal cannot reach a single bundled file.
#
# Deliberately a separate variable from the backend. "Has GLES" and "is
# Android" coincide here, and are not the same question.
SQUARED_PLATFORM := android

# -idirafter, not -I.
#
# The NDK sysroot is a second complete set of C headers. Ahead of libc++ on the
# search path, <cctype> finds the NDK's ctype.h instead of the one libc++
# expects, and the error names neither file. -idirafter appends to the very end
# of the search path, so the NDK supplies only what nothing else provides -
# which for this build is the Khronos headers and nothing more.
SQ_CPPFLAGS += -idirafter $(SQ_NDK_INC)

# Absolute paths, not -lEGL.
#
# On Termux, $PREFIX/lib/libEGL.so belongs to libglvnd - the X11 EGL, not
# Android's - and a plain -lEGL binds it in preference to the NDK stub. The
# link succeeds and the APK dies at runtime naming nothing useful. A search
# order cannot go wrong when there is no search.
SQ_GL_LIBS := $(SQ_NDK_LIB)/libEGL.so $(SQ_NDK_LIB)/libGLESv3.so
SQ_LDLIBS  += $(SQ_GL_LIBS)

else

# No GLES in reach: the null backend. Every Context symbol resolves and nothing
# is drawn, so the project builds and runs and shows a black window rather than
# failing to link with a message naming a private member function.
SQUARED_GRAPHICS_BACKEND := null
SQUARED_PLATFORM := posix

endif

# --- the Console ------------------------------------------------------------
#
#   make CONSOLE=app install       the in-app console (Sprint 4)
#   make CONSOLE=monitor install   the same, plus the monitor process (Sprint 4.7)
#   CONSOLE_SCROLLBACK=<bytes>     its history, default 65536 (about 1,000 lines)
#   CONSOLE_COMMANDS=<n>           its command table, default 64
#
# CONSOLE=0, the default, carries none of it: the archive is not linked and no
# flag reaches the application. Otherwise the sizes reach the application as
# -D flags, beside SQUARED_CONSOLE itself.
#
# The framework always gets the sizes, whatever CONSOLE is. Its Console
# archive is built in every build, and only linked when CONSOLE asks for it,
# so switching CONSOLE on or off changes nothing the framework compiles: the
# flags stamp below stays the same and nothing is rebuilt. Only the sizes
# change what the archive holds, and changing them rebuilds it (with the rest
# of the framework: the stamp is one for the whole tree).
CONSOLE            ?= 0
CONSOLE_SCROLLBACK ?= 65536
CONSOLE_COMMANDS   ?= 64
ifeq ($(filter 0 app monitor,$(CONSOLE)),)
  $(error CONSOLE must be 0, app or monitor, not '$(CONSOLE)')
endif
SQ_CONSOLE_SIZES := -DSQUARED_CONSOLE_SCROLLBACK=$(CONSOLE_SCROLLBACK) \
                    -DSQUARED_CONSOLE_COMMANDS=$(CONSOLE_COMMANDS)
ifneq ($(CONSOLE),0)
  SQ_CONSOLE_FLAGS := -DSQUARED_CONSOLE=$(if $(filter monitor,$(CONSOLE)),2,1) $(SQ_CONSOLE_SIZES)
  SQ_CPPFLAGS += $(SQ_CONSOLE_FLAGS)
endif

# --- the framework ----------------------------------------------------------

# Modules that publish headers. A module with no sources still publishes
# headers, which is why this is a list rather than the archive list.
SQ_SQUARED_MODULES := app assets console data dev files gles graphics gui \
                      math messaging time

# One include root for every header, workhorse or supporting: the directory
# that holds squared/. Headers are written <squared/<module>/...>.
SQ_SQUARED_INC := -I$(SQ_SQUARED_DIR)/..

# Link order matters for static archives: a dependent must precede what it
# depends on, because the linker resolves left to right and does not go back.
# gui needs graphics; graphics needs math and files; assets needs files;
# everything that parses needs data. gles stands alone.
SQ_SQUARED_LIB_DIR := $(SQ_SQUARED_DIR)/build/release/lib
SQ_SQUARED_LIBS := \
  $(if $(filter-out 0,$(CONSOLE)),$(SQ_SQUARED_LIB_DIR)/libsquared_console.a,) \
  $(SQ_SQUARED_LIB_DIR)/libsquared_gui.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_graphics.a \
  $(if $(filter gles,$(SQUARED_GRAPHICS_BACKEND)),\
     $(SQ_SQUARED_LIB_DIR)/libsquared_gles.a,) \
  $(SQ_SQUARED_LIB_DIR)/libsquared_assets.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_files.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_messaging.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_data.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_time.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_math.a \
  $(if $(DEV_LOG),$(SQ_SQUARED_LIB_DIR)/libsquared_dev.a,)

SQ_CPPFLAGS += $(SQ_SQUARED_INC)
SQ_LDLIBS   += $(SQ_SQUARED_LIBS)

# --- targets ----------------------------------------------------------------

# Build the vendored framework with the same toolchain, backend and platform
# this project uses. squared's own local.mk is not consulted: the project
# decides, and a local.mk left in the vendored tree would silently override it.
#
# Every selector must be passed here. One that is set above but not passed is
# silently ignored: the framework falls back to its own default and builds
# without complaint. SQUARED_PLATFORM was exactly that for a while - set to
# android, never passed, so every APK shipped a files library that could not
# read its own assets.
#
# The framework is compiled for the same target as the application: the
# template's SQ_TARGET (the ABI, the API level, the NDK sysroot) rides on CXX,
# so the C it vendors (yyjson, stb) gets it too, and SQ_CXX_INC (the NDK's
# libc++ headers, which match the runtime the application links) on
# CXXFLAGS_EXTRA, which only C++ compiles read. Both are empty under a template
# that does not set them, and the framework then builds as before.
#
# The framework's build does not track flags, so the flags are stamped here.
# When they change (another SQ_MIN_SDK, another NDK, a first build after this
# change) the framework is cleaned before it is built: archives compiled for
# one API level or one libc++ linked into a library for another would link
# without complaint and fail on the device.
SQ_SQUARED_FLAGS := $(CXX) $(SQ_TARGET) | $(SQ_CXX_INC) $(SQ_CONSOLE_SIZES) | $(SQUARED_GRAPHICS_BACKEND) $(SQUARED_PLATFORM)
SQ_SQUARED_STAMP := $(SQ_SQUARED_DIR)/build/release/.project-flags

.PHONY: squared-core squared-core-clean squared-core-status
squared-core:
	@if [ "$$(cat '$(SQ_SQUARED_STAMP)' 2>/dev/null)" != '$(SQ_SQUARED_FLAGS)' ]; then \
	  if [ -d '$(SQ_SQUARED_DIR)/build/release' ]; then \
	    echo "  squared-core   build flags changed: cleaning the framework first"; \
	    $(MAKE) --no-print-directory -C $(SQ_SQUARED_DIR) clean >/dev/null; \
	  fi; \
	  mkdir -p '$(dir $(SQ_SQUARED_STAMP))'; \
	  printf '%s' '$(SQ_SQUARED_FLAGS)' > '$(SQ_SQUARED_STAMP)'; \
	fi
	@$(MAKE) --no-print-directory -C $(SQ_SQUARED_DIR) \
	    CXX="$(strip $(CXX) $(SQ_TARGET))" \
	    CXXFLAGS_EXTRA="$(strip $(SQ_CXX_INC) $(SQ_CONSOLE_SIZES))" \
	    SQUARED_GRAPHICS_BACKEND=$(SQUARED_GRAPHICS_BACKEND) \
	    SQUARED_PLATFORM=$(SQUARED_PLATFORM) \
	    SQUARED_EXTERNAL_INCLUDES="$(if $(strip $(SQ_GL_LIBS)),-idirafter $(SQ_NDK_INC),)"

# The archives are built by squared-core, and must say so.
#
# Without this rule the archives are plain files with no recipe. Anything that
# links against them - the .so, and through it `make apk` - checks only that
# they exist, never that they are current, so `make apk` would link whatever
# happened to be lying in the tree and never build the framework at all. That
# is how an APK once shipped a files library without AndroidAssetFileSystem in
# it: the .so linked, and failed to load on the device.
#
# The recipe is deliberately empty (the trailing ';'). squared-core does the
# work; make then re-examines each archive's timestamp, so an up-to-date
# framework does not relink the application, and a changed one does.
$(SQ_SQUARED_LIBS): squared-core ;

squared-core-clean:
	@$(MAKE) --no-print-directory -C $(SQ_SQUARED_DIR) clean

squared-core-status:
	@printf 'squared-core\n'
	@printf '  %-14s %s\n' "tree" "$(SQ_SQUARED_DIR)"
	@printf '  %-14s %s\n' "backend" "$(SQUARED_GRAPHICS_BACKEND)"
	@printf '  %-14s %s\n' "platform" "$(SQUARED_PLATFORM)"
	@printf '  %-14s %s\n' "console" "$(if $(filter-out 0,$(CONSOLE)),$(CONSOLE) (scrollback $(CONSOLE_SCROLLBACK) bytes, $(CONSOLE_COMMANDS) commands),off (CONSOLE=0))"
	@printf '  %-14s %s\n' "compiled for" "$(if $(strip $(SQ_TARGET)),$(SQ_TARGET),the compiler's default target)"
	@printf '  %-14s %s\n' "headers" \
	    "$(if $(strip $(SQ_GL_LIBS)),$(SQ_NDK_INC)  (via idirafter),not needed for the null backend)"
	@printf '  %-14s %s\n' "libEGL" \
	    "$(if $(strip $(SQ_GL_LIBS)),$(SQ_NDK_LIB)/libEGL.so,NOT USED)"
	@printf '  %-14s %s\n' "libGLESv3" \
	    "$(if $(strip $(SQ_GL_LIBS)),$(SQ_NDK_LIB)/libGLESv3.so,NOT USED)"
	@printf '  %-14s %s\n' "archives" \
	    "$(if $(wildcard $(SQ_SQUARED_LIB_DIR)/libsquared_math.a),$(SQ_SQUARED_LIB_DIR),NOT BUILT - run 'make squared-core')"
	@if [ "$(SQUARED_GRAPHICS_BACKEND)" = "null" ]; then \
	  echo ""; \
	  echo "  The null backend draws nothing. That is correct when the NDK has"; \
	  echo "  no GLES stub at the path above, and wrong otherwise - check"; \
	  echo "  'make android-status' for where the NDK was found."; \
	fi
