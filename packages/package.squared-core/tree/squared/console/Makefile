# Build console into lib/libsquared_console.a
#
# The Console: text in, text out, for when the rest is broken (Sprint 4). A
# project links this archive only when it asks for it (CONSOLE=app or
# CONSOLE=monitor in the Android template); a CONSOLE=0 build, the default,
# carries none of it. Nothing else in squared depends on it.
#
# Its sizes are compile-time: SQUARED_CONSOLE_SCROLLBACK and
# SQUARED_CONSOLE_COMMANDS (console/src/limits.hpp), passed by the template as
# -D flags so the archive and the application agree; SQUARED_CONSOLE_GROUPS
# and SQUARED_CONSOLE_GROUP_LINES likewise, by hand, until a project needs them.
#
# The Telegraph's tools (telegraph_tools.cpp) use messaging and time, which
# every project links; the other tests build without them.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := console
MODULE_DIR := $(SQUARED_ROOT)/console

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/args.cpp \
    src/bridge.cpp \
    src/commands.cpp \
    src/console.cpp \
    src/copy_command.cpp \
    src/crash_file.cpp \
    src/file_commands.cpp \
    src/font8x8.cpp \
    src/groups.cpp \
    src/input_line.cpp \
    src/log_file.cpp \
    src/scrollback.cpp \
    src/system_commands.cpp \
    src/telegraph_tools.cpp

# The renderer and the `gl` command need the GLES 3.0 headers, so they are
# built with the gles backend only; the rest of the Console builds anywhere.
SQUARED_GRAPHICS_BACKEND ?= null
ifeq ($(SQUARED_GRAPHICS_BACKEND),gles)
SOURCES += src/gles/gl_command.cpp \
    src/gles/renderer.cpp
endif

# What the tests below link: everything but the drawing and the Telegraph's tools.
CORE_SOURCES := $(filter-out src/gles/% src/telegraph_tools.cpp,$(SOURCES))

# Messaging and time, for the Telegraph tools' test: built from source,
# as every test here builds the module.
MESSAGING_SOURCES := $(wildcard $(SQUARED_ROOT)/messaging/src/*.cpp) \
                     $(wildcard $(SQUARED_ROOT)/time/src/*.cpp)

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
HEADERS := $(wildcard $(MODULE_DIR)/*.hpp $(MODULE_DIR)/src/*.hpp $(MODULE_DIR)/test/*.hpp) \
           $(wildcard $(SQUARED_ROOT)/dev/*.hpp $(SQUARED_ROOT)/dev/src/*.hpp)

# Groups and dump: queued, one line a frame, combinations, stop, the limits.
DUMP_TEST := $(BUILD_DIR)/dump_test

# The Telegraph's tools over a real line, the model inline: the capacities,
# every value in order with its name, the tap, recording and replaying.
TELEGRAPH_TOOLS_TEST := $(BUILD_DIR)/telegraph_tools_test

# A child process crashes for real (SIGSEGV, abort) with the crash file
# installed; the parent reads the report, shows it, and checks the signal
# still reached the handler before it and killed the child.
CRASH_TEST := $(BUILD_DIR)/crash_file_test

# The bridge to Termux against real sockets, and its Termux end, sqconsole,
# run for real (--run, and interactive with its reconnect).
BRIDGE_TEST := $(BUILD_DIR)/bridge_test

# sqconsole, built for the machine running the tests: here the same compiler.
# A project builds it for Termux with `make console` (mk/pkg_squared_core.mk).
SQCONSOLE := $(BUILD_DIR)/sqconsole

TESTS := $(SCROLLBACK_TEST) $(CONSOLE_TEST) $(DUMP_TEST) $(TELEGRAPH_TOOLS_TEST) $(LAYOUT_TEST) \
    $(CRASH_TEST) $(BRIDGE_TEST)
ifeq ($(SQUARED_GRAPHICS_BACKEND),gles)
TESTS += $(RENDERER_TEST)
endif

test: $(TESTS)
	$(call run_tests,$(TESTS))

$(TESTS): $(SQUARED_FLAGS_STAMP) $(SQUARED_TEST_PCH_FILE)

# What each test includes, from its last build (tools/test_cc.sh).
-include $(TESTS:=.d) $(SQCONSOLE).d

$(SQCONSOLE): $(MODULE_DIR)/client/sqconsole.cpp $(SQUARED_FLAGS_STAMP) $(SQUARED_TEST_PCH_FILE)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $< -o $@

$(DUMP_TEST): $(MODULE_DIR)/test/dump_test.cpp $(addprefix $(MODULE_DIR)/,$(CORE_SOURCES)) \
              $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter %.cpp,$^) -ldl -pthread -o $@

$(TELEGRAPH_TOOLS_TEST): $(MODULE_DIR)/test/telegraph_tools_test.cpp \
                         $(addprefix $(MODULE_DIR)/,$(CORE_SOURCES)) \
                         $(MODULE_DIR)/src/telegraph_tools.cpp $(MESSAGING_SOURCES) \
                         $(SQUARED_ROOT)/dev/src/frame_allocations.cpp \
                         $(SQUARED_ROOT)/dev/src/dev_log.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter %.cpp,$^) -ldl -pthread -o $@

$(BRIDGE_TEST): $(MODULE_DIR)/test/bridge_test.cpp $(addprefix $(MODULE_DIR)/,$(CORE_SOURCES)) \
                $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp $(SQCONSOLE)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) -DSQCONSOLE_PATH='"$(SQCONSOLE)"' $(filter %.cpp,$^) -ldl -pthread -o $@

$(CRASH_TEST): $(MODULE_DIR)/test/crash_file_test.cpp $(addprefix $(MODULE_DIR)/,$(CORE_SOURCES)) $(HEADERS)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter %.cpp,$^) -ldl -o $@

$(LAYOUT_TEST): $(MODULE_DIR)/test/layout_test.cpp $(addprefix $(MODULE_DIR)/,$(CORE_SOURCES)) \
                $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter %.cpp,$^) -ldl -pthread -o $@

$(RENDERER_TEST): $(MODULE_DIR)/test/renderer_test.cpp $(MODULE_DIR)/test/gl_stub.cpp \
                  $(addprefix $(MODULE_DIR)/,$(SOURCES)) \
                  $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) $(filter %.cpp,$^) -ldl -pthread -o $@

$(CONSOLE_TEST): $(MODULE_DIR)/test/console_test.cpp $(addprefix $(MODULE_DIR)/,$(CORE_SOURCES)) \
                 $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter %.cpp,$^) -ldl -pthread -o $@

$(SCROLLBACK_TEST): $(MODULE_DIR)/test/scrollback_test.cpp $(MODULE_DIR)/src/scrollback.cpp \
                    $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter %.cpp,$^) -ldl -pthread -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(call say,ar,$(notdir $@))
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/console/%.o: $(MODULE_DIR)/%.cpp $(SQUARED_FLAGS_STAMP) $(SQUARED_PCH_FILE)
	@mkdir -p $(dir $@)
	$(call say,cc,$(MODULE)/$*.cpp)
	$(CXX) $(SQUARED_CXXFLAGS) $(SQUARED_PCH_USE) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/console $(TARGET)

-include $(DEPENDS)
