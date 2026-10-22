# squared - Linux/Android SDK in C++20 with a Lua 5.4 scripting layer.
#
#   make                 build every module (release)
#   make BUILD=debug     unoptimised with symbols
#   make headers         compile every public header on its own
#   make lint            format, source-rule and clang-tidy checks
#   make docs            write the API reference, docs/reference/ (clang-doc)
#   make docs-check      say whether docs/reference/ is current
#   make test            every module's tests; each runs only if it changed
#                        since it last passed (build/common.mk, run_tests)
#   make retest          every test again, changed or not
#   make lint LINT_ARGS=--changed   options for tools/lint.sh (also --only tidy)
#   make <module>        build one module
#   make V=1             every command, and every test's output, in full
#   make clean
#
# Header-only modules are built too, so their headers stay compilable.
#
# A build prints one short line per step (build/common.mk, "what a build
# prints"); errors and warnings in full.

# This file reads build/common.mk too, the way every module does: so local.mk
# is read before the module list below (a backend chosen there selects the
# module list as well as the right context.cpp), and the flags are said once,
# here, rather than by each module.
include build/common.mk

# gles needs EGL and GLES 3.0 headers, so it is built only when the graphics
# backend is gles. A null-backend build works on a machine with neither.
SQUARED_GRAPHICS_BACKEND ?= null

MODULES := math data files assets graphics \
           $(if $(filter gles,$(SQUARED_GRAPHICS_BACKEND)),gles,) \
           gui messaging time dev console

.PHONY: all clean docs docs-check headers lint test retest $(MODULES)

# The modules with tests, in the order they are run.
TESTED := dev files graphics $(if $(filter gles,$(SQUARED_GRAPHICS_BACKEND)),gles,) gui console time messaging

all: $(MODULES)

graphics: math files
gui: graphics math data files messaging
assets: files
gles: graphics
messaging: time

$(MODULES):
	$(MAKE) -C $@ -f Makefile

headers:
	./tools/check_headers.sh

# After `all`: graphics and gui tests link the archives.
test: all
	@for module in $(TESTED); do $(MAKE) --no-print-directory -C $$module test || exit 1; done

retest:
	@$(MAKE) --no-print-directory test RETEST=1

# Everything about linting lives outside this file: the engine is
# tools/lint.sh, and what it checks is in tools/lint-rules.txt and .clang-tidy
# (docs/developer/lint.md). LINT_ARGS, not LINT: make predefines LINT as a
# program name. Run through bash: a vendored copy can lose its executable bit.
lint:
	bash tools/lint.sh $(LINT_ARGS)

# The API reference: committed, so it reads on GitHub; made only by this.
docs:
	bash tools/docs.sh

docs-check:
	bash tools/docs.sh --check

clean:
	@for module in $(MODULES); do $(MAKE) -C $$module clean; done
	rm -rf build/release build/debug
