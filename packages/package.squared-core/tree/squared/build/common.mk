# Shared build settings for every squared module.
#
# Overridable from the command line or the environment, for example:
#   make CXX=clang++ BUILD=debug
#   make CXXFLAGS_EXTRA=-fsanitize=address

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
CXXFLAGS += $(CXXFLAGS_EXTRA)

# Every squared module is in this tree, and yyjson is vendored under
# third_party/ and built by data/Makefile. This variable is therefore optional;
# set it in local.mk if a project needs something else on the include path.
#
# local.mk is optional, per-checkout, and should not be committed.
SQUARED_EXTERNAL_INCLUDES ?=

-include $(SQUARED_ROOT)/local.mk

DEPFLAGS = -MMD -MP

# --- tests: run again only when something they cover changed ---------------
#
# Every test binary depends on its sources, the archives it links, and every
# header in the tree (SQUARED_HEADERS), so it is rebuilt whenever anything it
# could include changes. A test that passed leaves <binary>.passed beside the
# binary; `make test` runs a test only when its binary is newer than that
# mark, so with nothing changed it runs none, and after a change to one
# module's .cpp it runs only the tests built from it. The same binary on the
# same inputs gives the same answer, which is what makes skipping it honest;
# the line each module prints says how many ran and how many were kept.
# `make test RETEST=1` (or `make retest` at the top) runs every one.
#
# Reversed 2026-10-19, from "every test runs on every make test" (graphics):
# that rule was right while a test did not depend on its headers, so an
# unchanged binary could hide a changed header. Now it cannot.
SQUARED_HEADERS := $(shell find $(SQUARED_ROOT) \( -path $(SQUARED_ROOT)/build -o -path $(SQUARED_ROOT)/third_party \) -prune -o -name '*.hpp' -print)

# $(call run_tests,<test binaries>): the recipe of a module's `test` target.
define run_tests
@ran=0; kept=0; \
for t in $(1); do \
  if [ -z "$(RETEST)" ] && [ -f "$$t.passed" ] && [ "$$t.passed" -nt "$$t" ]; then \
    kept=$$((kept + 1)); \
  else \
    rm -f "$$t.passed"; "$$t" || exit 1; touch "$$t.passed"; ran=$$((ran + 1)); \
  fi; \
done; \
echo "  $(MODULE): $$ran test(s) ran and passed, $$kept unchanged since they last passed"
endef
