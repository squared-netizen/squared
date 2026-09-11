# kit.sfml build fragment.
#
# GENERATED FILE. kit.sfml owns this path and will replace it on update.
# Project-wide build settings belong in the Makefile, which is yours.
#
# This kit is not a renderer plugged into someone else's loop. SFML IS the
# platform layer: libsfml-main.a defines ANativeActivity_onCreate, which is the
# symbol Android calls to start the process. That is why kit.sfml works only
# with template.android.sfml, and why template.android.cpp -- which owns that
# same entry point through android_native_app_glue -- cannot use it.
#
# The archives are prebuilt for one ABI at one API level; see sq_kit/BUILD-INFO
# for which. They are not compiled here, and nothing in this workspace can
# rebuild them.

# --- headers ---------------------------------------------------------------
#
# Two include roots, for two different reasons.
#
# sq_kit/include holds SFML's own headers, reached as <SFML/Window.hpp>. Note
# the capital: this is SFML's tree verbatim, not a Squared-side bridge, so the
# D-057 kit-name directory does not apply. There is no kit.sfml bridge header
# to collide with it.
#
# The NDK supplies the Khronos headers Termux lacks -- EGL/, GLES2/, GLES3/,
# KHR/ -- for application code that draws. -idirafter, never -I: -I would place
# a second complete set of C headers ahead of libc++, and <cctype> would then
# find the NDK's ctype.h instead of libc++'s wrapper. libc++ detects that and
# stops the build with a message that names neither this file nor the flag.
# -idirafter appends to the very end of the search path, so the NDK supplies
# only what nothing else provides.
SQ_KIT_CPPFLAGS += -Isq_kit/include
SQ_KIT_CPPFLAGS += -idirafter $(SQ_NDK_INC)

# --- libraries -------------------------------------------------------------
#
# Order is load-bearing. These are static archives, and a static linker
# resolves left to right: a symbol needed by an earlier archive must be
# defined by a later one. main needs window, window needs system.
#
# Named by path rather than with -L and -l. An added -L changes resolution for
# every -l in the link, including ones this kit knows nothing about.
SQ_SFML_LIBS := \
    sq_kit/lib/libsfml-main.a \
    sq_kit/lib/libsfml-window-s.a \
    sq_kit/lib/libsfml-system-s.a

# The NDK stubs SFML's EGL backend needs, by absolute path.
#
# $(PREFIX)/lib/libEGL.so exists on Termux and belongs to libglvnd -- the X11
# EGL, not Android's. A plain -lEGL binds it in preference to the NDK stub; the
# link succeeds and the APK dies inside the package installer, which is the
# worst place to find out. A search order cannot go wrong when there is no
# search.
#
# GLESv1_CM is what SFML links (see its FindGLES). GLESv3 is here for
# application code: sq_app draws through GLES 3.0 and would otherwise have no
# implementation to bind against.
SQ_SFML_GL_LIBS := \
    $(SQ_NDK_LIB)/libEGL.so \
    $(SQ_NDK_LIB)/libGLESv1_CM.so \
    $(SQ_NDK_LIB)/libGLESv3.so

SQ_KIT_LDLIBS   += $(SQ_SFML_LIBS) $(SQ_SFML_GL_LIBS)
SQ_KITS_PRESENT += kit.sfml

.PHONY: sfml-status
sfml-status:
	@echo "kit.sfml"
	@if [ -f sq_kit/BUILD-INFO ]; then sed 's/^/  /' sq_kit/BUILD-INFO; \
	 else echo "  BUILD-INFO   MISSING -- payload was not installed"; fi
	@echo "  archives"
	@for a in $(SQ_SFML_LIBS); do \
	  if [ -f "$$a" ]; then printf '    %-28s %s bytes\n' "$$a" "$$(wc -c < $$a)"; \
	  else printf '    %-28s NOT FOUND\n' "$$a"; fi; \
	done
	@printf '  %-14s %s\n' "entry point" \
	  "$$(nm --defined-only sq_kit/lib/libsfml-main.a 2>/dev/null | grep -c ANativeActivity_onCreate) definition(s) of ANativeActivity_onCreate"
	@echo "  ndk stubs"
	@for l in $(SQ_SFML_GL_LIBS); do \
	  if [ -f "$$l" ]; then printf '    %-28s ok\n' "$$(basename $$l)"; \
	  else printf '    %-28s NOT FOUND\n' "$$(basename $$l)"; fi; \
	done
	@if [ -e "$(PREFIX)/lib/libEGL.so" ]; then \
	  echo "  note           \$$PREFIX/lib/libEGL.so exists -- that is libglvnd's X11 EGL."; \
	  echo "                 This build names the NDK stub by absolute path, so it"; \
	  echo "                 cannot be bound by accident."; \
	fi
