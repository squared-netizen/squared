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

endif

# --- the framework ----------------------------------------------------------

# Modules that publish headers. A module with no sources still has an include
# directory, which is why this is a list rather than the archive list.
SQ_SQUARED_MODULES := app assets data files gles graphics graphics2d gui \
                      math messaging scene2d time

SQ_SQUARED_INC := $(foreach m,$(SQ_SQUARED_MODULES),\
                    -I$(SQ_SQUARED_DIR)/$(m)/include)

# Link order matters for static archives: a dependent must precede what it
# depends on, because the linker resolves left to right and does not go back.
# gui needs graphics2d and scene2d; those need graphics and math; assets needs
# files; everything that parses needs data.
SQ_SQUARED_LIB_DIR := $(SQ_SQUARED_DIR)/build/release/lib
SQ_SQUARED_LIBS := \
  $(SQ_SQUARED_LIB_DIR)/libsquared_gui.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_scene2d.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_graphics2d.a \
  $(if $(filter gles,$(SQUARED_GRAPHICS_BACKEND)),\
     $(SQ_SQUARED_LIB_DIR)/libsquared_gles.a,) \
  $(SQ_SQUARED_LIB_DIR)/libsquared_graphics.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_assets.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_files.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_messaging.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_data.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_time.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_math.a

SQ_CPPFLAGS += $(SQ_SQUARED_INC)
SQ_LDLIBS   += $(SQ_SQUARED_LIBS)

# --- targets ----------------------------------------------------------------

# Build the vendored framework with the same toolchain and backend this project
# uses. squared's own local.mk is not consulted: the project decides, and a
# local.mk left in the vendored tree would silently override it.
.PHONY: squared-core squared-core-clean squared-core-status
squared-core:
	@$(MAKE) --no-print-directory -C $(SQ_SQUARED_DIR) \
	    CXX="$(CXX)" \
	    SQUARED_GRAPHICS_BACKEND=$(SQUARED_GRAPHICS_BACKEND) \
	    SQUARED_EXTERNAL_INCLUDES="$(if $(strip $(SQ_GL_LIBS)),-idirafter $(SQ_NDK_INC),)"

squared-core-clean:
	@$(MAKE) --no-print-directory -C $(SQ_SQUARED_DIR) clean

squared-core-status:
	@printf 'squared-core\n'
	@printf '  %-14s %s\n' "tree" "$(SQ_SQUARED_DIR)"
	@printf '  %-14s %s\n' "backend" "$(SQUARED_GRAPHICS_BACKEND)"
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
