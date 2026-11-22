# Build tui into lib/libsquared_tui.a
#
# Character-cell programs (squared-tui's first layers,
# docs/design/squared-tui.md): a screen of sq::term cells and the renderer
# that writes what changed; the terminal a program runs in (POSIX raw mode,
# put back however it ends) and its input as events; a test bench through
# sq.term's emulator. No drawing, no platform code beyond POSIX: it builds
# and is tested the same on the host and for Android.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := tui
MODULE_DIR := $(SQUARED_ROOT)/tui

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/input_decoder.cpp \
    src/renderer.cpp \
    src/screen.cpp \
    src/simple.cpp \
    src/terminal.cpp \
    src/test_bench.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/tui/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_tui.a

.PHONY: all clean test
all: $(TARGET)

# The screen, the renderer read back through sq.term's emulator, the input
# decoder as the inverse of sq.term's key_bytes and paste_text, the
# terminal's raw mode in a pty.
TUI_TEST := $(BUILD_DIR)/tui_test

TESTS := $(TUI_TEST)

test: $(TESTS)
	$(call run_tests,$(TESTS))

$(TESTS): $(SQUARED_FLAGS_STAMP)

# What each test includes, from its last build (tools/test_cc.sh).
-include $(TESTS:=.d)

TERM_DIR := $(SQUARED_ROOT)/term

$(TUI_TEST): $(MODULE_DIR)/test/tui_test.cpp \
             $(MODULE_DIR)/src/input_decoder.cpp \
             $(MODULE_DIR)/src/renderer.cpp \
             $(MODULE_DIR)/src/screen.cpp \
             $(MODULE_DIR)/src/simple.cpp \
             $(MODULE_DIR)/src/terminal.cpp \
             $(MODULE_DIR)/src/test_bench.cpp \
             $(TERM_DIR)/src/grid.cpp \
             $(TERM_DIR)/src/key_bytes.cpp \
             $(TERM_DIR)/src/pty.cpp \
             $(TERM_DIR)/src/screen_snapshot.cpp \
             $(TERM_DIR)/src/scrollback.cpp \
             $(TERM_DIR)/src/terminal.cpp \
             $(TERM_DIR)/src/text.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(SQUARED_TEST_INPUTS) -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(call say_archive,$@)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/tui/%.o: $(MODULE_DIR)/%.cpp $(SQUARED_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(SQUARED_CC) $(SQUARED_CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/tui $(TARGET)

-include $(DEPENDS)

# C++20 modules (build/modules.mk, 7b.4): this module's interface units, the
# BMIs its code imports, and the order of both.
include $(SQUARED_ROOT)/build/modules.mk
