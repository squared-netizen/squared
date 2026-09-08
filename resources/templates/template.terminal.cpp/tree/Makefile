# {{project_name}} — build.
#
# This file is YOURS (ownership class: seeded). The generator wrote it once and
# will not overwrite it. Edit it freely.
#
# The parts the generator does own live in mk/, are included below, and are
# rewritten on update. That split is deliberate: you get a build file you can
# change without fear, and the generator gets files it can regenerate without
# destroying your work (Generator Architecture §2.7.10).
#
#   make            build
#   make run        build and run
#   make debug      build with -O0 -g and sanitizers where available
#   make test       build and run anything under sq_app/test/
#   make lua-status report whether kit.lua found an interpreter
#   make clean
#
# Overridable on the command line, e.g. `make CXX=clang++ OPT=-O3`.

CXX      ?= c++
CXXSTD   ?= c++20
OPT      ?= -O2
WARN     ?= -Wall -Wextra -Wpedantic

BUILD    ?= build
BIN      ?= $(BUILD)/{{project_name}}

# `make` with no target builds. Stated explicitly because the included
# fragments below define targets of their own, and make's default goal is the
# first target it *sees* -- which would otherwise be whichever one a fragment
# happened to declare.
.DEFAULT_GOAL := all

# Generator-owned fragments. squared_generated.mk sets the project-wide
# defaults; each kit_*.mk adds its own include paths and libraries.
include mk/squared_generated.mk
-include $(wildcard mk/kit_*.mk)

APP_SRC  := $(shell find sq_app/src -name '*.cpp' 2>/dev/null | sort)
APP_OBJ  := $(patsubst %.cpp,$(BUILD)/%.o,$(APP_SRC))
DEPS     := $(APP_OBJ:.o=.d)

CPPFLAGS += $(SQ_CPPFLAGS) $(SQ_KIT_CPPFLAGS)
CXXFLAGS += -std=$(CXXSTD) $(OPT) $(WARN)
LDLIBS   += $(SQ_KIT_LDLIBS)

.PHONY: all run debug test clean help
all: $(BIN)

$(BIN): $(APP_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(APP_OBJ) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

run: $(BIN)
	@./$(BIN)

debug:
	@$(MAKE) OPT='-O0 -g -fno-omit-frame-pointer' BUILD=$(BUILD)/debug \
	         BIN=$(BUILD)/debug/{{project_name}} all

TEST_SRC := $(shell find sq_app/test -name '*.cpp' 2>/dev/null | sort)
test: $(BIN)
ifeq ($(strip $(TEST_SRC)),)
	@echo "no tests: add .cpp files under sq_app/test/"
else
	@fail=0; for t in $(TEST_SRC); do \
	  out=$(BUILD)/$$(basename $$t .cpp); \
	  mkdir -p $(BUILD); \
	  $(CXX) $(CPPFLAGS) $(CXXFLAGS) $$t $(LDFLAGS) $(LDLIBS) -o $$out || exit 1; \
	  ./$$out || fail=1; \
	done; \
	[ $$fail -eq 0 ] && echo "tests passed" || { echo "TEST FAILURES"; exit 1; }
endif

clean:
	rm -rf $(BUILD)

help:
	@echo "{{project_name}} — targets: all run debug test lua-status clean"
	@echo "kits applied: $(SQ_KITS_PRESENT)"

-include $(DEPS)
