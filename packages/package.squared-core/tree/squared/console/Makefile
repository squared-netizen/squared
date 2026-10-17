# Build console into lib/libsquared_console.a
#
# The Console: text in, text out, for when the rest is broken (Sprint 4). A
# project links this archive only when it asks for it (CONSOLE=app or
# CONSOLE=monitor in the Android template); a CONSOLE=0 build, the default,
# carries none of it. Nothing else in squared depends on it.
#
# Its sizes are compile-time: SQUARED_CONSOLE_SCROLLBACK and
# SQUARED_CONSOLE_COMMANDS (console/src/limits.hpp), passed by the template as
# -D flags so the archive and the application agree.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := console
MODULE_DIR := $(SQUARED_ROOT)/console

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/args.cpp \
    src/commands.cpp \
    src/console.cpp \
    src/copy_command.cpp \
    src/font8x8.cpp \
    src/input_line.cpp \
    src/scrollback.cpp \
    src/system_commands.cpp

# The renderer and the `gl` command need the GLES 3.0 headers, so they are
# built with the gles backend only; the rest of the Console builds anywhere.
SQUARED_GRAPHICS_BACKEND ?= null
ifeq ($(SQUARED_GRAPHICS_BACKEND),gles)
SOURCES += src/gles/gl_command.cpp \
    src/gles/renderer.cpp
endif

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/console/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_console.a

.PHONY: all clean test
all: $(TARGET)

# Checked against a plain model over random pushes into several storage sizes;
# the frame-allocation counter confirms pushing allocates nothing. Asserts on,
# whatever OPTIMISE says.
SCROLLBACK_TEST := $(BUILD_DIR)/scrollback_test
# The input line, words, the command table and the Console together: editing,
# history, completion, running commands; none of it allocates.
CONSOLE_TEST := $(BUILD_DIR)/console_test
# Where every character goes, read back as a grid; the font atlas; no
# allocation while laying out.
LAYOUT_TEST := $(BUILD_DIR)/layout_test
# With the gles backend: the renderer against a fake driver that records calls
# (test/gl_stub.cpp). It proves the call sequence, not what a driver accepts.
RENDERER_TEST := $(BUILD_DIR)/console_renderer_test

# Every test is rebuilt when any of the module's headers change: layout() is
# a template in one, and a header's change must not leave a stale test passing.
HEADERS := $(wildcard $(MODULE_DIR)/*.hpp $(MODULE_DIR)/src/*.hpp $(MODULE_DIR)/test/*.hpp)

TESTS := $(SCROLLBACK_TEST) $(CONSOLE_TEST) $(LAYOUT_TEST)
ifeq ($(SQUARED_GRAPHICS_BACKEND),gles)
TESTS += $(RENDERER_TEST)
endif

test: $(TESTS)
	@for t in $(TESTS); do $$t || exit 1; done

$(LAYOUT_TEST): $(MODULE_DIR)/test/layout_test.cpp $(addprefix $(MODULE_DIR)/,$(filter-out src/gles/%,$(SOURCES))) \
                $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g $(SQUARED_INCLUDE) $(filter %.cpp,$^) -ldl -pthread -o $@

$(RENDERER_TEST): $(MODULE_DIR)/test/renderer_test.cpp $(MODULE_DIR)/test/gl_stub.cpp \
                  $(addprefix $(MODULE_DIR)/,$(SOURCES)) \
                  $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) $(filter %.cpp,$^) -ldl -pthread -o $@

$(CONSOLE_TEST): $(MODULE_DIR)/test/console_test.cpp $(addprefix $(MODULE_DIR)/,$(filter-out src/gles/%,$(SOURCES))) \
                 $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g $(SQUARED_INCLUDE) $(filter %.cpp,$^) -ldl -pthread -o $@

$(SCROLLBACK_TEST): $(MODULE_DIR)/test/scrollback_test.cpp $(MODULE_DIR)/src/scrollback.cpp \
                    $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g $(SQUARED_INCLUDE) $(filter %.cpp,$^) -ldl -pthread -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/console/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/console $(TARGET)

-include $(DEPENDS)
