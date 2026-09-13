# kit.sfml build fragment.
#
# GENERATED FILE. kit.sfml owns this path and will replace it on update.
# Project-wide build settings belong in the Makefile, which is yours.
#
# This kit is not a renderer plugged into someone else's loop. SFML IS the
# platform layer: libsfml-main.a defines ANativeActivity_onCreate, the symbol
# Android calls to start the process. That is why kit.sfml works only with
# template.android.sfml, and why template.android.cpp -- which owns the same
# entry point through android_native_app_glue -- cannot use it.
#
# The archives are prebuilt for one ABI at one API level. sq_kit/BUILD-INFO
# records which. Nothing in this workspace can rebuild them.

# --- headers ---------------------------------------------------------------
#
# Two include roots, for two reasons.
#
# sq_kit/include holds SFML's own headers, reached as <SFML/Window.hpp>. Note
# the capital: this is SFML's tree verbatim, not a Squared-side bridge, so the
# D-061 kit-name directory does not apply. kit.sfml ships no bridge header, so
# there is nothing for it to collide with.
#
# The NDK supplies the Khronos headers Termux lacks -- EGL/, GLES2/, GLES3/,
# KHR/ -- for application code that draws.
#
# -idirafter, never -I. -I would place a second complete set of C headers
# ahead of libc++, and <cctype> would then find the NDK's ctype.h instead of
# libc++'s wrapper. libc++ detects that and stops the build with a message
# naming neither this file nor the flag.
SQ_KIT_CPPFLAGS += -Isq_kit/include
SQ_KIT_CPPFLAGS += -idirafter $(SQ_NDK_INC)

# --- SFML's own archives ---------------------------------------------------
#
# Order is load-bearing and this order is deliberate. Static archives resolve
# left to right: a symbol needed by an earlier archive must be defined by a
# later one.
#
#   main      needs window and system
#   graphics  needs window, and freetype/harfbuzz from the group below
#   audio     needs system, and vorbis/FLAC/ogg from the group below
#   window    needs system and EGL/GLES
#   system    needs nothing of SFML's
#
# Named by path rather than with -L and -l. An added -L changes resolution for
# every -l in the link, including ones this kit knows nothing about.
SQ_SFML_LIBS := \
    sq_kit/lib/libsfml-main.a \
    sq_kit/lib/libsfml-graphics-s.a \
    sq_kit/lib/libsfml-audio-s.a \
    sq_kit/lib/libsfml-window-s.a \
    sq_kit/lib/libsfml-system-s.a

# --- third-party archives --------------------------------------------------
#
# Discovered rather than listed: whatever the kit shipped that is not one of
# SFML's own. The set depends on how each dependency names its targets, and
# SheenBidi contributes none at all -- SFML patches it to an OBJECT library, so
# it folds into libsfml-graphics-s.a.
#
# Wrapped in --start-group because FreeType and HarfBuzz are mutually
# dependent: FreeType uses HarfBuzz for auto-hinting, HarfBuzz uses FreeType
# for glyph data. A linker pass in one direction cannot satisfy both, and
# SFML's own Dependencies.cmake resorts to listing them twice. --start-group
# makes the linker iterate until no new symbols resolve, which removes the
# ordering question entirely -- and is what makes globbing safe here, since
# order inside a group does not matter.
SQ_SFML_DEP_LIBS := $(filter-out $(SQ_SFML_LIBS),$(wildcard sq_kit/lib/*.a))

# --- NDK libraries ---------------------------------------------------------
#
# By absolute path, all of them, for one reason repeated four times: Termux
# ships its own build of several of these, and they are not Android's.
#
#   libEGL.so    $(PREFIX)/lib/libEGL.so belongs to libglvnd -- the X11 EGL.
#                A plain -lEGL binds it in preference to the NDK stub; the
#                link succeeds and the APK dies inside the package installer.
#   libz.so      $(PREFIX)/lib/libz.so is Termux's zlib. Graphics links z, and
#                an APK that bound Termux's copy would need it bundled.
#
# GLESv1_CM is what SFML links (see its FindGLES). GLESv3 is for application
# code, which draws through GLES 3.0 and would otherwise have no
# implementation to bind against. OpenSLES is what Audio's miniaudio backend
# uses on Android.
#
# A search order cannot go wrong when there is no search.
SQ_SFML_GL_LIBS := \
    $(SQ_NDK_LIB)/libEGL.so \
    $(SQ_NDK_LIB)/libGLESv1_CM.so \
    $(SQ_NDK_LIB)/libGLESv3.so \
    $(SQ_NDK_LIB)/libz.so \
    $(SQ_NDK_LIB)/libOpenSLES.so

SQ_KIT_LDLIBS += $(SQ_SFML_LIBS) \
                 -Wl,--start-group $(SQ_SFML_DEP_LIBS) -Wl,--end-group \
                 $(SQ_SFML_GL_LIBS)

SQ_KITS_PRESENT += kit.sfml

# --- diagnostics -----------------------------------------------------------

.PHONY: sfml-status
sfml-status:
	@echo "kit.sfml"
	@if [ -f sq_kit/BUILD-INFO ]; then sed 's/^/  /' sq_kit/BUILD-INFO; \
	 else echo "  BUILD-INFO    MISSING -- the payload was not installed."; \
	      echo "                A fresh clone carries this fragment and the"; \
	      echo "                manifest, but not the archives."; fi
	@echo "  sfml archives"
	@for a in $(SQ_SFML_LIBS); do \
	  if [ -f "$$a" ]; then printf '    %-30s %9s bytes\n' "$$(basename $$a)" "$$(wc -c < $$a)"; \
	  else printf '    %-30s NOT FOUND\n' "$$(basename $$a)"; fi; \
	done
	@echo "  dependency archives"
	@if [ -z "$(strip $(SQ_SFML_DEP_LIBS))" ]; then \
	  echo "    none found -- Graphics and Audio will not link"; \
	else \
	  for a in $(SQ_SFML_DEP_LIBS); do \
	    printf '    %-30s %9s bytes\n' "$$(basename $$a)" "$$(wc -c < $$a)"; \
	  done; \
	fi
	@printf '  %-15s %s\n' "entry point" \
	  "$$(nm --defined-only sq_kit/lib/libsfml-main.a 2>/dev/null | grep -c ANativeActivity_onCreate) definition(s) of ANativeActivity_onCreate"
	@echo "  ndk libraries"
	@for l in $(SQ_SFML_GL_LIBS); do \
	  if [ -f "$$l" ]; then printf '    %-30s ok\n' "$$(basename $$l)"; \
	  else printf '    %-30s NOT FOUND\n' "$$(basename $$l)"; fi; \
	done
	@if [ -e "$(PREFIX)/lib/libEGL.so" ]; then \
	  echo "  note            \$$PREFIX/lib/libEGL.so exists -- libglvnd's X11 EGL."; \
	  echo "                  This build names the NDK stub by absolute path, so"; \
	  echo "                  it cannot be bound by accident."; \
	fi
