# squared - Linux/Android SDK in C++23 with a Lua 5.4 scripting layer.
#
#   make                 build every module (release)
#   make BUILD=debug     unoptimised with symbols
#   make headers         check the pinned std sources and the modules' build order
#                        (tools/check_modules.sh; until 7b.4, every header
#                        compiled on its own)
#   make lint            format, source-rule and clang-tidy checks
#   make docs            write the API reference, docs/reference/ (clang-doc)
#   make docs-check      say whether docs/reference/ is current
#   make test            every module's tests; each runs only if it changed
#                        since it last passed (build/common.mk, run_tests)
#   make retest          every test again, changed or not
#   make lint LINT_ARGS=--changed   options for tools/lint.sh (also --only tidy)
#   make <module>        build one module
#   make sqgen           sqgen (and sqconsole), the host tools, in build/<BUILD>/
#   make V=1             every command, and every test's output, in full
#   make SQUARED_CACHE=off   no build cache (build/common.mk, "the build cache")
#   make clean
#
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

MODULES := app math data files assets graphics \
           $(if $(filter gles,$(SQUARED_GRAPHICS_BACKEND)),gles,) \
           gui messaging time dev console stage term font

.PHONY: all clean docs docs-check headers lint test retest umbrella sqgen sqconsole $(MODULES)

# The modules with tests, in the order they are run.
TESTED := dev files graphics $(if $(filter gles,$(SQUARED_GRAPHICS_BACKEND)),gles,) gui console time messaging stage term font

# Then the build cache, back under its size (build/common.mk, "the build
# cache"). Quick when it is under: one du.
all: $(MODULES) umbrella
	@if [ "$(SQUARED_CACHE)" != off ] && [ -n "$(SQUARED_CACHE)" ]; then \
	  bash tools/cache_prune.sh '$(SQUARED_CACHE)' '$(SQUARED_CACHE_SIZE)'; fi

# The order the modules import each other in (7b.4): a module's BMIs are made
# before any module that imports it is compiled. tools/module_order.sh checks
# this list against the import lines (tools/check_modules.sh; make headers
# runs it).
graphics: dev math files
app: graphics files assets
gui: app graphics math data dev files messaging
assets: files
gles: graphics
messaging: time
console: app dev messaging time
stage: app gui graphics assets dev files messaging data time math
term: app

# std first, here, once: every module imports it, and they build at the
# same time under -j (build/common.mk, "the standard library, as a module").
$(MODULES): $(SQUARED_STD_READY)
	$(MAKE) -C $@ -f Makefile

# squared, the one import for an app (squared.cppm: the standard library, the
# App and its Stage, the widgets, the Telegraph), after the modules it
# re-exports; its object in lib/libsquared.a, which an app links first.
SQUARED_UMBRELLA_PCM := $(SQUARED_MODULE_DIR)/squared.pcm
SQUARED_UMBRELLA_LIB := $(LIB_DIR)/libsquared.a
# Its own make, after the modules: so it is made again only when one of their
# BMIs changed, not on every build (an app's objects wait for squared.pcm, and
# would all compile again). Through the build cache, as the modules'
# interfaces are (build/modules.mk): its sources inside its BMI, its key
# reading the modules' .inputs (by their .sums). Before 2026-11-02 it was
# compiled in every workspace, the one `mod` line a fresh one still printed.
SQUARED_UMBRELLA_USES := app gui messaging stage
umbrella: $(SQUARED_UMBRELLA_USES)
	$(MAKE) --no-print-directory $(SQUARED_UMBRELLA_LIB)
$(SQUARED_UMBRELLA_LIB): private MODULE := squared
$(SQUARED_UMBRELLA_LIB): private SQUARED_IMPORT_INPUTS := \
    $(foreach m,$(SQUARED_UMBRELLA_USES),$(SQUARED_MODULE_DIR)/sq.$(m).inputs)
$(SQUARED_UMBRELLA_LIB): squared.cppm $(SQUARED_STD_READY) $(SQUARED_FLAGS_STAMP) \
                         $(foreach m,$(SQUARED_UMBRELLA_USES),$(SQUARED_MODULE_DIR)/sq.$(m).pcm \
                                                             $(SQUARED_MODULE_DIR)/sq.$(m).inputs)
	@mkdir -p $(OBJ_DIR)/squared
	SQUARED_SAY_LINE='$(if $(filter 1,$(V)),,  mod  squared)' \
	    $(SQUARED_CC) $(SQUARED_CXXFLAGS) $(SQUARED_EMBED) -c $< \
	    -fmodule-output=$(SQUARED_UMBRELLA_PCM) -o $(OBJ_DIR)/squared/squared.o
	rm -f $@
	$(AR) rcs $@ $(OBJ_DIR)/squared/squared.o
	$(call say_archive,$@)
-include $(OBJ_DIR)/squared/squared.d

headers:
	bash tools/check_modules.sh

# The host tools a project builds for the machine it is built on
# (mk/pkg_squared_core.mk: `make BUILD=host CXX=<host compiler> sqgen`), from
# the framework built for that machine.
sqgen: all
	$(MAKE) -C gui $(BUILD_DIR)/sqgen

sqconsole: all
	$(MAKE) -C console $(BUILD_DIR)/sqconsole

# dexgen (tools/dexgen.sh, docs/developer/shims.md): the shim dex's writer, a
# host tool that imports std and nothing of squared's. Built as the tests are
# (TEST_CXX: the std module), since C++23 made it `import std;` (12.6: until
# then tools/dexgen.sh compiled it alone, without the module, and failed).
DEXGEN := $(BUILD_DIR)/tools/dexgen
DEXGEN_SELF_TEST := $(BUILD_DIR)/tools/dexgen-self-test
dexgen: all
	@$(MAKE) --no-print-directory $(DEXGEN) $(DEXGEN_SELF_TEST)

-include $(DEXGEN).d $(DEXGEN_SELF_TEST).d

$(DEXGEN): tools/dexgen/dexgen.cpp $(SQUARED_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(call say,bin,tools/dexgen)
	$(TEST_CXX) $(TESTFLAGS) $< -o $@

$(DEXGEN_SELF_TEST): tools/dexgen/dexgen.cpp $(SQUARED_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(call say,bin,tools/dexgen-self-test)
	$(TEST_CXX) $(TESTFLAGS) -DDEXGEN_SELF_TEST $< -o $@

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
