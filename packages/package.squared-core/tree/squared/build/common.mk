# Shared build settings for every squared module, and for the root Makefile.
#
# Overridable from the command line or the environment, for example:
#   make CXX=clang++ BUILD=debug
#   make CXXFLAGS_EXTRA=-fsanitize=address
#   make V=1                  every command in full, as make prints them

CXX ?= g++
AR ?= ar
BUILD ?= release

SQUARED_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))..)

# The public include root is the directory that holds squared/. Every header,
# workhorse or supporting, is reached as <squared/<module>/...> through it, so a
# module no longer lists the include directories of the modules it uses. That
# only works while this directory is named squared.
ifneq ($(notdir $(SQUARED_ROOT)),squared)
$(error squared: this directory must be named squared, not $(notdir $(SQUARED_ROOT)), because headers are included as <squared/...>)
endif
PUBLIC_ROOT := $(abspath $(SQUARED_ROOT)/..)
SQUARED_INCLUDE := -I$(PUBLIC_ROOT)

BUILD_DIR ?= $(SQUARED_ROOT)/build/$(BUILD)
LIB_DIR := $(BUILD_DIR)/lib
OBJ_DIR := $(BUILD_DIR)/obj

STANDARD := -std=c++20
WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion

ifeq ($(BUILD),debug)
  OPTIMISE := -O0 -g
else
  OPTIMISE := -O2 -DNDEBUG
endif

# Android is the constraining target, so size-sensitive codegen is on by
# default: function and data sections let the linker drop what is unused.
CODEGEN := -fno-common -ffunction-sections -fdata-sections -fvisibility=hidden

# -fno-rtti builds clean for the whole tree and is verified, including a link
# and run. Turn it on with:
#   make SQUARED_ABI=-fno-rtti
# -fno-exceptions does not build yet; docs/developer/priority-audit.md item 3
# has the remaining list.
SQUARED_ABI ?=

CXXFLAGS ?= $(STANDARD) $(OPTIMISE) $(WARNINGS) $(CODEGEN) $(SQUARED_ABI)

# What every object is compiled with: CXXFLAGS, then CXXFLAGS_EXTRA. Kept out
# of CXXFLAGS itself because the root Makefile reads this file too: a CXXFLAGS
# from the environment would reach the modules with the extra flags already in
# it, and they would get them twice.
SQUARED_CXXFLAGS = $(CXXFLAGS) $(CXXFLAGS_EXTRA)

# Tests and the small tools built beside them: quick to compile, with symbols,
# and the same SQUARED_ABI as the archives they link (a test built with RTTI
# against archives built without it fails to link: no typeinfo).
TESTFLAGS = $(STANDARD) -O1 -g $(SQUARED_ABI)

# Every squared module is in this tree, and yyjson is vendored under
# third_party/ and built by data/Makefile. This variable is therefore optional;
# set it in local.mk if a project needs something else on the include path.
#
# local.mk is optional, per-checkout, and should not be committed.
SQUARED_EXTERNAL_INCLUDES ?=

-include $(SQUARED_ROOT)/local.mk

# The compiler, named in the flags stamp below: a new clang rebuilds what the
# old one made (a precompiled header from another version is refused).
# One word, for the stamp: `clang_version_21.1.8`.
SQUARED_CXX_VERSION := $(shell $(CXX) --version 2>/dev/null | head -n 1 | tr ' ' '_')

# Whether the standard headers are precompiled (below, "the standard headers,
# precompiled").
SQUARED_PCH_HEADER := $(SQUARED_ROOT)/build/std_pch.hpp
ifneq ($(findstring clang,$(SQUARED_CXX_VERSION)),)
  SQUARED_PCH ?= on
endif
SQUARED_PCH_ON := $(filter on,$(SQUARED_PCH))
ifneq ($(SQUARED_PCH_ON),)
  SQUARED_PCH_FILE := $(BUILD_DIR)/pch/std.pch
  SQUARED_TEST_PCH_FILE := $(BUILD_DIR)/pch/std-test.pch
  SQUARED_PCH_USE := -include-pch $(SQUARED_PCH_FILE)
else
  SQUARED_PCH_FILE :=
  SQUARED_TEST_PCH_FILE :=
  SQUARED_PCH_USE :=
endif

DEPFLAGS = -MMD -MP

# --- what a build prints -----------------------------------------------------
#
# One short line as each step starts, so a long compile is never silent:
#
#   squared release: flags as before
#     cc   gui/src/stage.cpp
#     ar   libsquared_gui.a
#     bin  gui/ui_keys_test
#     test gui/ui_keys_test
#          ok
#
# Compiler warnings and errors print in full, as always. make's own chatter
# (the commands, "Entering directory", "Nothing to be done") does not: -s
# below, which every module make inherits. `make V=1` prints every command in
# full and every test's output, as before.
ifneq ($(V),1)
MAKEFLAGS += -s
endif

# $(call say,<step>,<what>): that line. Nothing under V=1, which prints the
# command itself.
say = $(if $(filter 1,$(V)),,@printf '  %-4s %s\n' '$(1)' '$(2)')

# --- the flags, said once ----------------------------------------------------
#
# Every object and test binary depends on build/<BUILD>/flags, which holds the
# compiler and the flags it last built with. The file is rewritten only when
# they change, so a change (another CXX, SQUARED_ABI=-fno-rtti, the template's
# NDK flags) rebuilds everything it affects, and nothing else ever does. Before
# this, make compared only timestamps, and an object built with other flags
# was kept.
#
# The first make to read this file says what the flags are: one line when they
# are what they were, all of them, one to a line, when they are not. It exports
# SQUARED_FLAGS_SAID, so the module makes it starts say nothing more. Both
# happen only for a make that builds or tests; `make clean`, `make lint` (whose
# tools ask the Makefiles for variables) and the rest leave the file alone.
SQUARED_FLAGS_STAMP := $(BUILD_DIR)/flags
# Then, one word each: the tests' flags, whether the standard headers are
# precompiled, and the compiler's version.
squared_space := $(subst ,, )
squared_flags := $(strip $(CXX) $(SQUARED_CXXFLAGS) $(SQUARED_EXTERNAL_INCLUDES) \
    tests:$(subst $(squared_space),:,$(strip $(TESTFLAGS))) \
    pch:$(if $(SQUARED_PCH_ON),on,off) $(SQUARED_CXX_VERSION))

squared_goals := $(or $(MAKECMDGOALS),all)
squared_building := $(strip $(filter all test retest,$(squared_goals)) \
    $(wildcard $(patsubst %,$(SQUARED_ROOT)/%/Makefile,$(squared_goals))))

# $(squared_nothing) starts an indented $(info) line: make drops the spaces
# before the first character of a function's argument, and this is one.
squared_nothing :=

# The flags one to a line, for the screen only: an option that takes a path
# keeps it on its line (squared_join marks that space with ^), and a long path
# is shortened, Termux's $PREFIX and ~.
squared_join = $(subst -idirafter ,-idirafter^,$(subst -isystem ,-isystem^,$(subst -include ,-include^,$(1))))
squared_word = $(subst ^, ,$(call squared_show,$(1)))
squared_show = $(if $(HOME),$(subst $(HOME),~,$(squared_show_prefix)),$(squared_show_prefix))
squared_show_prefix = $(if $(PREFIX),$(subst $(PREFIX),$$PREFIX,$(1)),$(1))

ifneq ($(squared_building),)
  squared_flags_before := $(file <$(SQUARED_FLAGS_STAMP))
  ifneq ($(squared_flags),$(squared_flags_before))
    $(shell mkdir -p '$(BUILD_DIR)')
    $(file >$(SQUARED_FLAGS_STAMP),$(squared_flags))
  endif
  ifeq ($(SQUARED_FLAGS_SAID),)
    ifeq ($(squared_flags),$(squared_flags_before))
      $(info squared $(BUILD): flags as before)
    else
      $(info squared $(BUILD): flags $(if $(squared_flags_before),changed,(first build)))
      $(foreach w,$(call squared_join,$(squared_flags)),$(info $(squared_nothing)    $(call squared_word,$(w))$(if $(squared_flags_before),$(if $(filter $(w),$(call squared_join,$(squared_flags_before))),,  (new)))))
      $(foreach w,$(filter-out $(call squared_join,$(squared_flags)),$(call squared_join,$(squared_flags_before))),$(info $(squared_nothing)    $(call squared_word,$(w))  (gone)))
    endif
  endif
  export SQUARED_FLAGS_SAID := 1
endif

# A make that does not build but needs the file anyway (one object asked for
# by name) writes it here. Being the first rule in this file, it would also be
# every module's default goal; .DEFAULT_GOAL keeps that `all`.
.DEFAULT_GOAL := all
$(SQUARED_FLAGS_STAMP):
	@mkdir -p $(dir $@)
	$(file >$@,$(squared_flags))

# --- the standard headers, precompiled (7b.2) -------------------------------
#
# The standard headers are most of each compile (measured on the phone,
# 2026-10-27: the 47 squared includes take 4.3 s to parse alone, and a file's
# front end is 2.7 to 5.5 s). build/std_pch.hpp includes them all, and is
# compiled once per flag set into a precompiled header (22 MB): the
# framework's objects, with SQUARED_CXXFLAGS, and the tests, with TESTFLAGS.
# Every source is then compiled with -include-pch, which takes a third to
# three quarters off each.
#
# clang only (-include-pch); g++ builds as before. Off with SQUARED_PCH=off.
# The precompiled header is included into every file, so a .cpp that forgets
# a standard include still builds here: `make lint` (clang-tidy, no
# precompiled header) and `make headers` still find it.
# The tests' precompiled header is made with -pthread, and tools/test_cc.sh
# compiles every test source with it: some tests run threads, and clang
# refuses a precompiled header made with the other choice ("POSIX thread
# support was disabled in PCH file").
#
# A test, or a host tool built beside the tests, through tools/test_cc.sh:
# each source compiled on its own (with the test precompiled header), so the
# binary's .d lists what it really includes, then linked. The objects are kept
# in build/<BUILD>/test-objects/ and shared between the tests that compile the
# same source with the same flags.
TEST_CXX = SQUARED_TEST_PCH='$(SQUARED_TEST_PCH_FILE)' SQUARED_TEST_OBJECTS='$(BUILD_DIR)/test-objects' \
    bash $(SQUARED_ROOT)/tools/test_cc.sh $(CXX)

ifneq ($(SQUARED_PCH_ON),)
# -MD, not -MMD: the standard headers are system headers, and a new libc++
# must make the precompiled header again (clang refuses a stale one).
$(SQUARED_PCH_FILE): $(SQUARED_PCH_HEADER) $(SQUARED_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(call say,pch,standard headers ($(notdir $@)))
	$(CXX) $(SQUARED_CXXFLAGS) $(SQUARED_EXTERNAL_INCLUDES) -x c++-header $< -MD -MP -MF $@.d -o $@

$(SQUARED_TEST_PCH_FILE): $(SQUARED_PCH_HEADER) $(SQUARED_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(call say,pch,standard headers ($(notdir $@)))
	$(CXX) $(TESTFLAGS) -pthread -x c++-header $< -MD -MP -MF $@.d -o $@

-include $(SQUARED_PCH_FILE).d $(SQUARED_TEST_PCH_FILE).d
endif

# --- tests: run again only when something they cover changed ---------------
#
# Every test binary depends on its sources, the archives it links, the
# headers its sources include (its .d, written by tools/test_cc.sh) and the
# flags, so it is rebuilt whenever anything it includes changes, and not when
# another module's header does. (Until 7b.2 it depended on every header in the
# tree: one header changed rebuilt 38 of 39 tests, 13 minutes on the phone.) A test that passed leaves
# <binary>.passed beside the binary; `make test` runs a test only when its
# binary is newer than that mark, so with nothing changed it runs none, and
# after a change to one module's .cpp it runs only the tests built from it.
# The same binary on the same inputs gives the same answer, which is what makes
# skipping it honest. `make test RETEST=1` (or `make retest` at the top) runs
# every one.
#
# A test's output goes to <binary>.log beside it. The build says "ok" under
# its name, or "FAIL" and then the whole log. Under V=1 the output prints as
# the test runs, as before.
#
# Reversed 2026-10-19, from "every test runs on every make test" (graphics):
# that rule was right while a test did not depend on its headers, so an
# unchanged binary could hide a changed header. Now it cannot.
# What a test recipe hands the compiler from its prerequisites: sources,
# objects and archives, in their order. Not the headers its .d adds, nor the
# flags stamp or the precompiled header.
SQUARED_TEST_INPUTS = $(filter %.cpp %.c %.o %.a,$^)

# $(call run_tests,<test binaries>): the recipe of a module's `test` target.
define run_tests
@kept=0; \
for t in $(1); do \
  if [ -z "$(RETEST)" ] && [ -f "$$t.passed" ] && [ "$$t.passed" -nt "$$t" ]; then \
    kept=$$((kept + 1)); continue; \
  fi; \
  rm -f "$$t.passed"; \
  printf '  test %s\n' "$(MODULE)/$${t##*/}"; \
  if [ "$(V)" = 1 ]; then "$$t"; else "$$t" > "$$t.log" 2>&1; fi; \
  status=$$?; \
  if [ $$status -ne 0 ]; then \
    if [ $$status -gt 128 ]; then why="signal $$((status - 128))"; else why="exit $$status"; fi; \
    printf '       FAIL (%s)\n' "$$why"; \
    if [ "$(V)" != 1 ]; then cat "$$t.log"; printf '  (that was %s.log)\n' "$${t#$(SQUARED_ROOT)/}"; fi; \
    exit 1; \
  fi; \
  echo "       ok"; \
  touch "$$t.passed"; \
done; \
if [ $$kept -gt 0 ]; then echo "  $(MODULE): $$kept unchanged since they passed"; fi
endef
