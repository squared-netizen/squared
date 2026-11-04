# Build term into lib/libsquared_term.a
#
# A terminal emulator's core (Sprint 12, sqterm): bytes in, a screen of cells
# out; a program in a pseudo-terminal (POSIX, the same on Linux and Android).
# No drawing, no platform code: it builds and is tested the same on the host
# and for Android.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := term
MODULE_DIR := $(SQUARED_ROOT)/term

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/grid.cpp \
    src/key_bytes.cpp \
    src/pty.cpp \
    src/screen_snapshot.cpp \
    src/scrollback.cpp \
    src/terminal.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/term/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_term.a

.PHONY: all clean test
all: $(TARGET)

# The parser, the screen and the scrollback against hand-written streams;
# random bytes; no allocation while feeding.
TERM_TEST := $(BUILD_DIR)/term_test
# A shell in a pty, read through a Terminal: output, exit codes, the
# environment and directory, resizing, Ctrl-C, a program that cannot run,
# hanging up.
PTY_TEST := $(BUILD_DIR)/pty_test

# What keys send (12.3d): every named key with its modifiers, both cursor-key
# modes, text with Ctrl and Alt.
KEY_TEST := $(BUILD_DIR)/key_test

TESTS := $(TERM_TEST) $(PTY_TEST) $(KEY_TEST)

test: $(TESTS)
	$(call run_tests,$(TESTS))

$(TESTS): $(SQUARED_FLAGS_STAMP)

# What each test includes, from its last build (tools/test_cc.sh).
-include $(TESTS:=.d)

$(TERM_TEST): $(MODULE_DIR)/test/term_test.cpp \
              $(MODULE_DIR)/src/grid.cpp \
              $(MODULE_DIR)/src/screen_snapshot.cpp \
              $(MODULE_DIR)/src/scrollback.cpp \
              $(MODULE_DIR)/src/terminal.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(SQUARED_TEST_INPUTS) -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(call say_archive,$@)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/term/%.o: $(MODULE_DIR)/%.cpp $(SQUARED_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(SQUARED_CC) $(SQUARED_CXXFLAGS) $(INCLUDES) -c $< -o $@

$(PTY_TEST): $(MODULE_DIR)/test/pty_test.cpp \
             $(MODULE_DIR)/src/grid.cpp \
             $(MODULE_DIR)/src/pty.cpp \
             $(MODULE_DIR)/src/scrollback.cpp \
             $(MODULE_DIR)/src/terminal.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(SQUARED_TEST_INPUTS) -o $@

$(KEY_TEST): $(MODULE_DIR)/test/key_test.cpp \
             $(MODULE_DIR)/src/key_bytes.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(SQUARED_TEST_INPUTS) -o $@

clean:
	rm -rf $(OBJ_DIR)/term $(TARGET)

-include $(DEPENDS)

# C++20 modules (build/modules.mk, 7b.4): this module's interface units, the
# BMIs its code imports, and the order of both.
include $(SQUARED_ROOT)/build/modules.mk
