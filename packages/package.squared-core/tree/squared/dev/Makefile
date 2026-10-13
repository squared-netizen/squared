# Build dev into lib/libsquared_dev.a
#
# Development aids: not part of a release build. A project links this archive
# only when it asks for it (DEV_LOG=1 in the Android template). Nothing else in
# squared depends on it.
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := dev
MODULE_DIR := $(SQUARED_ROOT)/dev

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/dev_log.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/dev/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_dev.a

.PHONY: all clean test
all: $(TARGET)

# Sends a line over loopback and provokes a real SIGSEGV in a child process, so
# the crash path is exercised and not just compiled.
TEST_BINARY := $(BUILD_DIR)/dev_log_test

test: $(TEST_BINARY)
	@$(TEST_BINARY)

$(TEST_BINARY): $(MODULE_DIR)/test/dev_log_test.cpp $(MODULE_DIR)/src/dev_log.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g $(SQUARED_INCLUDE) $^ -ldl -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/dev/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/dev $(TARGET)

-include $(DEPENDS)
