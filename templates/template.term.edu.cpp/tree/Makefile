# =============================================================================
# Makefile for {{project_name}}
# =============================================================================
#
# This file tells `make` how to turn your C++ source files into a program.
# It is written to be read, top to bottom. Lines starting with # are comments:
# make ignores them, they are here for you.
#
# HOW TO USE IT
#
#   make          build the program
#   make run      build it (if needed), then run it
#   make clean    delete everything make built, to start fresh
#   make -n       show the commands make WOULD run, without running them
#
# HOW MAKE THINKS
#
# A Makefile is a list of RULES. A rule looks like this:
#
#   target: prerequisites
#   <TAB>recipe
#
#   target         a file to build (or a name for an action)
#   prerequisites  the files the target is built from
#   recipe         the shell command(s) that build it
#
# Before running a recipe, make compares file times. If the target exists and is
# newer than every prerequisite, nothing has changed, so make skips it. That is
# why a second `make` right after the first does nothing, and why editing one
# file only recompiles that file.
#
# One thing to know: every recipe line must start with a TAB character, not
# spaces. If you see "missing separator", that is almost always the cause.

# -----------------------------------------------------------------------------
# SETTINGS (variables)
# -----------------------------------------------------------------------------
# A variable is a name for some text. Write NAME = text to set it and $(NAME)
# to use it. make replaces $(NAME) with the text.

# The C++ compiler. `?=` means "only set this if it is not already set", so you
# can swap compilers from the command line without editing this file:
#   make CXX=clang++
CXX      ?= c++

# Options passed to the compiler:
#   -std=c++20   use the C++20 version of the language
#   -Wall -Wextra  turn on warnings. Warnings point at code that compiles but
#                  is probably a mistake. Read them; they teach a lot.
#   -g           keep information a debugger needs (names, line numbers)
CXXFLAGS  = -std=c++20 -Wall -Wextra -g

# Options for the "preprocessor", the step that handles #include lines.
#   -Isq_app/include   also look in sq_app/include for headers you #include
#   $(SQ_KIT_CPPFLAGS) more -I options, added by any kits this project uses
#                      (see KITS at the end). Empty when there are none.
#
# This line uses `=`, not `:=`, on purpose: the value is looked up when it is
# used, which is after the KITS lines at the end have been read.
CPPFLAGS  = -Isq_app/include $(SQ_KIT_CPPFLAGS)

# Where things live, and what they are called.
#
# `:=` means "work this out once, right now". `=` would work it out again each
# time the variable is used. For lists of files, `:=` is the right choice.

# $(wildcard PATTERN) turns a pattern into the list of files that match it.
# So SRC becomes every .cpp file directly inside sq_app/src/. (Only that one
# folder: files in sub-folders are not found.)
SRC := $(wildcard sq_app/src/*.cpp)

# $(SRC:old=new) takes the list in SRC and rewrites each name. Here
# sq_app/src/main.cpp becomes build/main.o. The % stands for "whatever is
# in this part of the name". An .o file ("object file") is a compiled
# source file that is not yet a finished program.
OBJ := $(SRC:sq_app/src/%.cpp=build/%.o)

# The finished program.
BIN := build/{{project_name}}

# -----------------------------------------------------------------------------
# RULES
# -----------------------------------------------------------------------------

# These names are actions, not files. .PHONY tells make never to look for a
# file called "run" or "clean" and think it is already up to date.
.PHONY: all run clean

# The FIRST rule in the file is what plain `make` builds. This one has no
# recipe: it just says "to build all, build the program".
all: $(BIN)

# Rule 1: LINK. Join all the object files into the finished program.
# $@ means "the target of this rule", here build/{{project_name}}.
$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $@

# Rule 2: COMPILE. Turn one .cpp file into one .o file.
# This is a "pattern rule": it applies to ANY build/NAME.o that has a matching
# sq_app/src/NAME.cpp. So you never add a rule when you add a source file.
#
#   $<   the first prerequisite (the .cpp file)
#   $@   the target (the .o file)
#   -c   compile only; do not link yet
#   -o   name of the output file
#   @    at the start of a line: run it but do not print it
#
# -MMD -MP asks the compiler to also write a small .d file next to the .o. The
# .d file lists every header the source file included. See the last section.
build/%.o: sq_app/src/%.cpp
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

# make run: build the program if it is out of date, then run it.
run: $(BIN)
	./$(BIN)

# make clean: delete the build folder. The next `make` rebuilds everything.
clean:
	rm -rf build

# -----------------------------------------------------------------------------
# KITS
# -----------------------------------------------------------------------------
# A kit is a ready-made add-on, such as kit.terminal. Each kit that was added to
# this project leaves a small file called mk/kit_NAME.mk that says what the kit
# needs from the build, for example an extra -I option. This line reads them all.
#
# $(wildcard mk/kit_*.mk) is every file in mk/ whose name starts with kit_ and
# ends with .mk. The leading `-` means "do not complain if there are none",
# which is the case when no kit was added. Those files are generated: do not
# edit them.
-include $(wildcard mk/kit_*.mk)

# -----------------------------------------------------------------------------
# HEADER TRACKING
# -----------------------------------------------------------------------------
# Rule 2 only lists the .cpp file as a prerequisite, so on its own make would not
# notice if you edited a header that the file includes, and you would keep
# running a stale program. The .d files fix that: each one is a tiny Makefile
# saying "main.o also depends on these headers". This line reads them in.
#
# The leading `-` means "do not complain if a file is missing". On the very first
# build there are no .d files yet, and that is fine.
-include $(OBJ:.o=.d)
