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

SOURCES := src/scrollback.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/console/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_console.a

.PHONY: all clean test
all: $(TARGET)

# Checked against a plain model over random pushes into several storage sizes;
# the frame-allocation counter confirms pushing allocates nothing. Asserts on,
# whatever OPTIMISE says.
SCROLLBACK_TEST := $(BUILD_DIR)/scrollback_test

test: $(SCROLLBACK_TEST)
	@$(SCROLLBACK_TEST)

$(SCROLLBACK_TEST): $(MODULE_DIR)/test/scrollback_test.cpp $(MODULE_DIR)/src/scrollback.cpp \
                    $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g $(SQUARED_INCLUDE) $^ -ldl -pthread -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/console/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/console $(TARGET)

-include $(DEPENDS)
