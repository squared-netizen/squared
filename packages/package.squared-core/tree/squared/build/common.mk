# Shared build settings for every squared module, and for the root Makefile.
#
# Overridable from the command line or the environment, for example:
#   make CXX=clang++ BUILD=debug
#   make CXXFLAGS_EXTRA=-fsanitize=address
#   make V=1                  every command in full, as make prints them

# clang: squared is C++20 modules (7b.4), built the way clang builds them. On
# Termux, g++ and c++ are clang already. (make's own default for CXX is g++,
# which ?= would keep.)
ifeq ($(origin CXX),default)
  CXX := clang++
endif
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

# The language every importer of a squared module must share with it: clang
# refuses a BMI made with another standard, or without threads, RTTI or
# exceptions when the importer has them ("configuration mismatch"). Tests,
# tools and the template's app code are compiled with it too (-pthread: the
# Telegraph's threads; on Android it changes nothing but the check).
#
# C++23, with libc++ (decided 2026-11-02): `import std;` is libc++'s own
# module (third_party/libcxx/), and libc++ is the standard library everywhere,
# Android's, Termux's and a desktop's alike (libstdc++ ships no std module
# before GCC 15). -D__BIONIC_CTYPE_INLINE=inline: Android's <ctype.h> makes
# isalpha and the rest static inline, which a module cannot export; with it
# they are plain inline, in the std module and in a file that includes
# <ctype.h> itself alike. Elsewhere it names nothing.
#
# -stdlib=libc++ only where nothing names libc++'s headers already: an Android
# build gives the NDK's with -stdlib++-isystem (the template's flags), and
# there clang warned "argument unused during compilation: '-stdlib=libc++'"
# on every interface (the demo's build, 2026-11-02). Android's C++ library is
# libc++ in any case.
squared_names_libcxx = $(findstring -stdlib++-isystem,$(CXXFLAGS_EXTRA) $(SQUARED_EXTERNAL_INCLUDES))
STANDARD = -std=c++23 $(if $(squared_names_libcxx),,-stdlib=libc++) -pthread -D__BIONIC_CTYPE_INLINE=inline
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

# Paths in an object (debug information, __FILE__) are written from the
# directory that holds squared/, as squared/gui/src/ui.cpp, not from /: so an
# object does not name its workspace, and the build cache (below) gives one
# workspace's object to another. A debugger finds the sources from the
# workspace's directory (gdb: `directory <it>`); addr2line prints the same
# short paths.
SQUARED_PREFIX_MAP = -ffile-prefix-map=$(PUBLIC_ROOT)/=

# What every object is compiled with: CXXFLAGS, then CXXFLAGS_EXTRA. Kept out
# of CXXFLAGS itself because the root Makefile reads this file too: a CXXFLAGS
# from the environment would reach the modules with the extra flags already in
# it, and they would get them twice.
SQUARED_CXXFLAGS = $(CXXFLAGS) $(CXXFLAGS_EXTRA) $(SQUARED_PREFIX_MAP)

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

# C++20 modules (7b.4, build/modules.mk): every BMI in one directory, where
# every compile looks for what it imports.
SQUARED_MODULE_DIR := $(BUILD_DIR)/modules
SQUARED_MODULE_USE := -fprebuilt-module-path=$(SQUARED_MODULE_DIR)
# An interface unit puts its sources in its BMI, so a BMI from the build cache,
# made in another workspace, reads nothing of that one (build/modules.mk).
SQUARED_EMBED := -Xclang -fmodules-embed-all-files

# std, the standard library as a module (C++23, libc++'s, pinned in
# third_party/libcxx/), made by tools/std_bmi.sh once per compiler, flags and
# libc++, and shared: modules/std.pcm links to it. std.ready is made with it,
# and is what everything that imports std waits for (std.pcm's own date is the
# shared BMI's, older than this build's). std.inputs lists PINNED for the
# build cache's key. libsquared_std.a holds its object: every program that
# imports a squared module links it. The root Makefile makes them first.
SQUARED_LIBCXX := $(SQUARED_ROOT)/third_party/libcxx
SQUARED_STD_PINNED := $(SQUARED_LIBCXX)/PINNED
SQUARED_STD_READY := $(SQUARED_MODULE_DIR)/std.ready
SQUARED_STD_INPUTS := $(SQUARED_MODULE_DIR)/std.inputs
SQUARED_STD_LIB := $(LIB_DIR)/libsquared_std.a
# One per device when squared-pg is set up, as the build cache is.
SQUARED_STD_HOME ?= $(if $(wildcard $(SQUARED_SQPG_HOME)/.),$(SQUARED_SQPG_HOME)/std,$(SQUARED_MODULE_DIR)/std)
SQUARED_STD_KEEP ?= 6

# --- the build cache (7b.3) ---------------------------------------------------
#
# Every framework object is compiled through tools/cached_cc.sh, which keeps
# it in a cache keyed by what the compile read: the source and every header,
# by content, the compiler and the flags. A compile that read the same again,
# in this workspace or another, takes the object instead (a fresh workspace,
# a `make clean`, the template's flags changed and changed back, a header
# edited and edited back). The cache is one for every workspace on this
# device when squared-pg is set up (~/sqsysroot/.sqpg), else this workspace's
# build/cache, which `make clean` keeps. It is pruned after each build to
# SQUARED_CACHE_SIZE megabytes, least lately used first.
#
#   make SQUARED_CACHE=off           compile everything, keep nothing
#   make SQUARED_CACHE=/somewhere    another cache
#   make SQUARED_CACHE_SIZE=2000     a bigger one (megabytes)
#
# Tests are not cached here: tools/test_cc.sh shares their objects within
# the workspace.
SQUARED_SQPG_HOME ?= $(HOME)/sqsysroot/.sqpg
SQUARED_CACHE ?= $(if $(wildcard $(SQUARED_SQPG_HOME)/.),$(SQUARED_SQPG_HOME)/cache,$(SQUARED_ROOT)/build/cache)
SQUARED_CACHE_SIZE ?= 500

# The compile of one object, in an object rule's recipe: then its flags, then
# -c $< -o $@. It writes the object's .d (the headers, system ones too: a new
# libc++ rebuilds), and says `  cc   <source>` when it compiles.
SQUARED_CC = SQUARED_CACHE='$(SQUARED_CACHE)' SQUARED_PUBLIC_ROOT='$(PUBLIC_ROOT)' \
    SQUARED_CXX_VERSION='$(SQUARED_CXX_VERSION)' \
    SQUARED_SAY='$(if $(filter 1,$(V)),,$(patsubst $(SQUARED_ROOT)/%,%,$<))' \
    SQUARED_HITS='$(OBJ_DIR)/$(MODULE)/.cache-hits' \
    SQUARED_CACHE_INPUTS='$(strip $(SQUARED_IMPORT_INPUTS))' \
    bash $(SQUARED_ROOT)/tools/cached_cc.sh $(CXX) $(SQUARED_MODULE_USE)

# $(call say_archive,<archive>): the `ar` line, with how many of its objects
# this build took from the cache.
say_archive = @n=$$(cat '$(OBJ_DIR)/$(MODULE)/.cache-hits' 2>/dev/null | wc -l); \
    rm -f '$(OBJ_DIR)/$(MODULE)/.cache-hits'; \
    $(if $(filter 1,$(V)),:,if [ "$$n" -gt 0 ]; then \
      printf '  %-4s %s (%s from the cache)\n' ar '$(notdir $(1))' "$$n"; \
    else printf '  %-4s %s\n' ar '$(notdir $(1))'; fi)

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
    $(SQUARED_CXX_VERSION))

squared_goals := $(or $(MAKECMDGOALS),all)
squared_building := $(strip $(filter all test retest umbrella sqgen sqconsole,$(squared_goals)) \
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
  ifeq ($(findstring clang,$(SQUARED_CXX_VERSION)),)
    $(error squared: $(CXX) is not clang ($(or $(SQUARED_CXX_VERSION),not found)); squared's C++23 modules build with clang: make CXX=clang++)
  endif
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

# --- the standard library, as a module: import std (C++23) ---------------------
#
# Every file imports std and names any standard type without saying where it
# comes from; a C macro is the exception (assert, errno, PROP_VALUE_MAX): it
# needs its C header, in the file's global module fragment.
#
# History: the standard headers were precompiled (7b.2), then squared's own
# sq.std (7b.4, build/std.cppm, a generated list of 161 names) until 2026-11-02,
# when C++23 brought libc++'s std module. A precompiled header and modules do
# not mix (clang 18 crashed on a module built with one). Measured on the phone
# (2026-10-27, clang 21, every framework file 8 at a time): 35.1 s with the
# precompiled header, 33.3 s with sq.std.
#
# tools/std_bmi.sh makes it, or finds it made (by this build before, or by
# another project with the same compiler, flags and libc++), and prints one
# `mod std` line either way; then std.sum, what std.inputs names holds
# (tools/inputs_sum.sh, for the build cache). It runs when the flags or
# PINNED change, when a libc++ header it read changes (std.d), and when the
# shared BMI went (the link dangles: pruned, or ~/sqsysroot/.sqpg wiped).
squared_std_gone := $(if $(wildcard $(SQUARED_STD_READY)),$(if $(realpath $(SQUARED_MODULE_DIR)/std.pcm),,squared-std-gone))
.PHONY: squared-std-gone
$(SQUARED_STD_READY) $(SQUARED_STD_INPUTS) $(SQUARED_STD_LIB) &: $(SQUARED_STD_PINNED) $(SQUARED_FLAGS_STAMP) \
                                                            $(SQUARED_ROOT)/tools/std_bmi.sh \
                                                            $(SQUARED_ROOT)/tools/inputs_sum.sh $(squared_std_gone)
	@mkdir -p $(SQUARED_MODULE_DIR) $(LIB_DIR)
	SQUARED_STD_HOME='$(SQUARED_STD_HOME)' SQUARED_STD_KEEP='$(SQUARED_STD_KEEP)' AR='$(AR)' \
	    SQUARED_SAY='$(if $(filter 1,$(V)),,1)' \
	    bash $(SQUARED_ROOT)/tools/std_bmi.sh $(SQUARED_MODULE_DIR) $(SQUARED_STD_LIB) \
	    $(CXX) $(SQUARED_CXXFLAGS)
	bash $(SQUARED_ROOT)/tools/inputs_sum.sh $(SQUARED_STD_INPUTS) $(SQUARED_MODULE_DIR)/std.sum $(PUBLIC_ROOT)
	touch $(SQUARED_STD_READY)
-include $(SQUARED_MODULE_DIR)/std.d
# The shared copy of the sources, gone (pruned, or ~/sqsysroot/.sqpg wiped):
# std.d names it, and make makes std again rather than stop.
$(SQUARED_STD_HOME)/%/src/std.cppm: ;

# An object compiled before 7b.4 names the precompiled header in its
# dependency file, without -MP's empty rule for it (clang lists it first, as
# it would the source). The header is gone, and make would stop at "No rule
# to make target build/std_pch.hpp" before it got to rebuild that object (the
# flags changed, so every object is). These rules let it get there. They can
# go once no workspace has objects from before 7b.4.
$(SQUARED_ROOT)/build/std_pch.hpp $(BUILD_DIR)/pch/std.pch $(BUILD_DIR)/pch/std-test.pch: ;

# A test, or a host tool built beside the tests, through tools/test_cc.sh:
# each source compiled on its own, so the binary's .d lists what it really
# includes, then linked. The objects are kept in build/<BUILD>/test-objects/
# and shared between the tests that compile the same source with the same
# flags. Every test ends its link with its module's archive, those of the
# modules it uses, sq.app's and std's (SQUARED_TEST_LINK, one group;
# archives, so only what a test needs comes in).
TEST_CXX = SQUARED_TEST_OBJECTS='$(BUILD_DIR)/test-objects' \
    SQUARED_TEST_DEPS='$(strip $(SQUARED_IMPORT_PCMS))' \
    SQUARED_TEST_LINK='-Wl,--start-group $(sort $(SQUARED_TEST_ARCHIVES) $(wildcard $(LIB_DIR)/libsquared_app.a)) $(SQUARED_STD_LIB) -Wl,--end-group' \
    bash $(SQUARED_ROOT)/tools/test_cc.sh $(CXX) $(SQUARED_MODULE_USE)

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
