# kit.opengl build fragment.
#
# GENERATED FILE. kit.opengl owns this path and will replace it on update.
#
# Deliberately thin. SQ_NDK, SQ_NDK_INC and SQ_NDK_LIB are located by
# mk/squared_generated.mk, which the *template* owns — NativeActivity is the
# template's architecture, not a renderer's, and a project with no rendering
# kit still needs the NDK glue. Duplicating detection here would mean two
# copies to keep in agreement, and a second rendering kit would make it three.
#
# So this fragment adds exactly what a renderer needs, and it does so in two
# unusual ways, both for reasons that cost a build to learn.

# --- headers: -idirafter, not -I ------------------------------------------
#
# Termux's clang ships its own complete sysroot, including every android/*
# header. The NDK is needed here only for the Khronos headers Termux lacks:
# EGL/, GLES2/, GLES3/, KHR/.
#
# Adding the NDK sysroot with -I puts a *second* complete set of C headers
# ahead of libc++, and <cctype> then finds the NDK's ctype.h instead of
# libc++'s wrapper. libc++ notices and stops the build:
#
#     <cctype> tried including <ctype.h> but didn't find libc++'s <ctype.h>
#
# -idirafter appends to the very end of the search path, after the standard
# system directories. Termux's own headers and libc++ win everywhere they
# overlap, and the NDK supplies only what nothing else provides — which is
# exactly the four Khronos directories.
SQ_KIT_CPPFLAGS += -Isq_kit/include
SQ_KIT_CPPFLAGS += -idirafter $(SQ_NDK_INC)

# --- libraries: absolute paths, not -l names -------------------------------
#
# $PREFIX/lib/libEGL.so belongs to libglvnd — the X11 EGL, not Android's — and
# a plain -lEGL binds it in preference to the NDK stub. The link succeeds and
# the APK dies at runtime inside the package installer, which is the worst
# place to find out.
#
# -L ordering would usually fix that, but it also puts the NDK's libc, libm and
# libdl stubs ahead of Termux's for every implicit -l the driver adds. Naming
# the two files outright avoids both problems: a search order cannot go wrong
# when there is no search.
SQ_GL_LIBS := $(SQ_NDK_LIB)/libEGL.so $(SQ_NDK_LIB)/libGLESv3.so

SQ_KIT_LDLIBS   += $(SQ_GL_LIBS)
SQ_KITS_PRESENT += kit.opengl

.PHONY: gl-status
gl-status:
	@echo "kit.opengl"
	@printf '  %-14s %s\n' "headers" "$(if $(wildcard $(SQ_NDK_INC)/GLES3/gl3.h),$(SQ_NDK_INC)  (via -idirafter),NOT FOUND)"
	@printf '  %-14s %s\n' "libEGL" "$(if $(wildcard $(SQ_NDK_LIB)/libEGL.so),$(SQ_NDK_LIB)/libEGL.so,NOT FOUND)"
	@printf '  %-14s %s\n' "libGLESv3" "$(if $(wildcard $(SQ_NDK_LIB)/libGLESv3.so),$(SQ_NDK_LIB)/libGLESv3.so,NOT FOUND)"
	@printf '  %-14s %s\n' "profile" "GLES 3.0 / GLSL ES 3.00"
	@if [ -e "$(PREFIX)/lib/libEGL.so" ]; then \
	  echo "  note           \$$PREFIX/lib/libEGL.so exists — that is libglvnd's X11 EGL."; \
	  echo "                 This build names the NDK stub by absolute path, so it"; \
	  echo "                 cannot be bound by accident. 'make' reports what was."; \
	fi
