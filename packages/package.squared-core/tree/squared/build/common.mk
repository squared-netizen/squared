# Shared build settings for every squared module.
#
# Overridable from the command line or the environment, for example:
#   make CXX=clang++ BUILD=debug
#   make CXXFLAGS_EXTRA=-fsanitize=address

CXX ?= g++
AR ?= ar
BUILD ?= release

SQUARED_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))..)

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
