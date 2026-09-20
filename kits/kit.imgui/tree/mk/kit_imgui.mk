# kit.imgui — build fragment.
#
# Included automatically: the workspace Makefile picks up mk/kit_*.mk by
# wildcard, so this fragment is live the moment the kit is installed. The name
# carries the kit's own name so two kits never collide.
#
# A kit ADDS to a workspace's build; it does not reorganise it
# (Generator Architecture 2.9). Two rules that came from real build failures:
#
#   - additional sysroots go in with -idirafter, never -I. -I prepends and
#     will displace the host's own C headers, at which point libc++ stops
#     finding them and the error appears far from its cause.
#   - name libraries by path rather than adding -L and using -l. An added -L
#     changes resolution for every -l in the link, including ones this kit
#     knows nothing about.
#
# The test: if your fragment only works when it comes first, it is
# reorganising the toolchain rather than adding to it.
#
# ## The include root (D-061)
#
# Headers go under sq_kit/include/imgui/ and the flag names the
# include ROOT, not the kit's directory inside it:
#
#     -Isq_kit/include            correct
#     -Isq_kit/imgui/include   wrong
#
# The distinction is not cosmetic. The flag must point at the root so the
# include reads <imgui/header.hpp>; a kit-name segment inside the
# root is what keeps two kits' headers from colliding with each other and
# with sq_app's. Point -I one level deeper and every kit's headers land in
# one flat search space, where a shared filename resolves silently by -I
# order with no diagnostic.
#
# Every kit adds this same root, so with several kits the flag appears more
# than once on the command line. Harmless, and it is the workspace's job to
# deduplicate, not this fragment's.

# Kits contribute through accumulators, never by touching the workspace's own
# variables. The template reads these after every mk/kit_*.mk has been
# included, so appending here is all a kit has to do:
#
#   SQ_KIT_CPPFLAGS   compile flags and include paths
#   SQ_KIT_LDFLAGS    link flags
#   SQ_KIT_LDLIBS     libraries, named by path
#   SQ_KIT_SRC        sources for the workspace to compile
#   SQ_KITS_PRESENT   this kit's identity, for diagnostics
#
# Appending to CXXFLAGS or LDLIBS directly would work by accident and break the
# moment the template reorders anything.

SQ_KIT_CPPFLAGS += -Isq_kit/include

# One include-root is not enough, and the reason is upstream's build, not ours.
# ImGui's sources include their own headers with quoted includes, and its
# backend lives one directory above imgui.h:
#
#   sq_kit/include/imgui/backends/imgui_impl_opengl3.cpp  ->  #include "imgui.h"
#
# A quoted include searches the current file's directory first — which here has
# no imgui.h — and then the include path, so the imgui root must be on the
# path for the backend to see its own sibling header. This is exactly what
# upstream's CMake does: it adds the imgui base directory to the include path
# and leaves the backend where it is. The D-061 rule about naming the kit root
# still stands for the code that includes THIS kit; this second flag exists so
# the kit can compile itself.
SQ_KIT_CPPFLAGS += -Isq_kit/include/imgui
SQ_KITS_PRESENT += kit.imgui

# --- sources this kit contributes -----------------------------------------
#
# ImGui keeps its sources and headers together in one directory, and its
# sources include their headers by quoted include resolved relative to
# themselves, so nothing under sq_kit/include/imgui/ can be split apart or
# moved. The generic $(BUILD)/%.o: %.cpp rule compiles them with the
# workspace's own flags, including its warning set; the tree is clean under
# -Wall -Wextra -Wpedantic with no per-object flags.
#
# imgui_demo.cpp is shipped deliberately: it is what makes ShowDemoWindow()
# link, and an immediate-mode UI kit that cannot render its own demo is
# hard to verify on a device with no screen logging.
#
# imgui_impl_opengl3.cpp is the only backend. imgui_impl_android was trimmed:
# the target is SFML, which owns input, and the bridge (sq_imgui.hpp/.cpp)
# replaces both files. sq_imgui.cpp is this kit's own code and is compiled
# like any other source the kit contributes.
#
# On Android imgui_impl_opengl3.h auto-defines IMGUI_IMPL_OPENGL_ES3, which
# routes the backend's GL calls to the NDK's <GLES3/gl3.h> and skips the
# embedded imgl3w runtime loader entirely. That is a compile-time dependency on
# the NDK Khronos headers, which reach the compiler only because kit.sfml
# appends `-idirafter $(SQ_NDK_INC)`. kit.imgui declares
# `requires.kits: ["kit.sfml"]` but the generator does not enforce that yet
# (spec decision D-070, deferred): with kit.imgui alone the build fails at
# <GLES3/gl3.h> naming nothing. Use `-k kit.sfml -k kit.imgui`, in that order —
# the order D-070 will require once enforcement lands.
SQ_KIT_SRC += \
    sq_kit/include/imgui/imgui.cpp \
    sq_kit/include/imgui/imgui_demo.cpp \
    sq_kit/include/imgui/imgui_draw.cpp \
    sq_kit/include/imgui/imgui_tables.cpp \
    sq_kit/include/imgui/imgui_widgets.cpp \
    sq_kit/include/imgui/backends/imgui_impl_opengl3.cpp \
    sq_kit/include/imgui/sq_imgui.cpp

# --- diagnostics -----------------------------------------------------------

.PHONY: imgui-status
imgui-status:
	@echo "kit.imgui"
	@echo "  sources"
	@for s in $(SQ_KIT_SRC); do \
	  if [ -f "$$s" ]; then printf '    %-52s %9s bytes\n' "$$s" "$$(wc -c < $$s)"; \
	  else printf '    %-52s NOT FOUND\n' "$$s"; fi; \
	done
	@if [ -f sq_kit/include/imgui/backends/imgui_impl_opengl3.cpp ]; then \
	  echo "  backend          imgui_impl_opengl3 (GLES 3, \"#version 300 es\")"; \
	else \
	  echo "  backend          imgui_impl_opengl3 NOT FOUND -- the payload was not installed"; \
	fi