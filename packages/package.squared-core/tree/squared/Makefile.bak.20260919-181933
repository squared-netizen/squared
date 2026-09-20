# squared - Linux/Android SDK in C++20 with a Lua 5.4 scripting layer.
#
#   make                 build every module (release)
#   make BUILD=debug     unoptimised with symbols
#   make headers         compile every public header on its own
#   make <module>        build one module
#   make clean
#
# Header-only modules are built too, so their headers stay compilable.

# Module Makefiles reach local.mk through build/common.mk. This file does not
# include common.mk, so it reads local.mk directly - otherwise a backend chosen
# there would select the right context.cpp but not the right module list.
-include local.mk

# gles needs EGL and GLES 3.0 headers, so it is built only when the graphics
# backend is gles. A null-backend build works on a machine with neither.
SQUARED_GRAPHICS_BACKEND ?= null

MODULES := math data files assets graphics \
           $(if $(filter gles,$(SQUARED_GRAPHICS_BACKEND)),gles,) \
           graphics2d scene2d gui messaging time

.PHONY: all clean headers $(MODULES)

all: $(MODULES)

graphics2d: graphics math
scene2d: graphics2d math
gui: graphics graphics2d scene2d math data
messaging: time data
assets: files
gles: graphics

$(MODULES):
	$(MAKE) -C $@ -f Makefile

headers:
	./tools/check_headers.sh

clean:
	@for module in $(MODULES); do $(MAKE) -C $$module clean; done
	rm -rf build/release build/debug
