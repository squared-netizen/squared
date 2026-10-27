# Build time into lib/libsquared_time.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := time
MODULE_DIR := $(SQUARED_ROOT)/time

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/now.cpp \
    src/timepiece.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/time/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_time.a

.PHONY: all clean test
all: $(TARGET)

# Timepiece, the clocks, and DeadlineQueue against a model, with no allocation counted.
TIME_TEST := $(BUILD_DIR)/time_test

TESTS := $(TIME_TEST)

test: $(TESTS)
	$(call run_tests,$(TESTS))

$(TESTS): $(SQUARED_FLAGS_STAMP) $(SQUARED_TEST_PCH_FILE)

# What each test includes, from its last build (tools/test_cc.sh).
-include $(TESTS:=.d)

$(TIME_TEST): $(MODULE_DIR)/test/time_test.cpp \
              $(MODULE_DIR)/src/now.cpp \
              $(MODULE_DIR)/src/timepiece.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(SQUARED_TEST_INPUTS) -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(call say,ar,$(notdir $@))
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/time/%.o: $(MODULE_DIR)/%.cpp $(SQUARED_FLAGS_STAMP) $(SQUARED_PCH_FILE)
	@mkdir -p $(dir $@)
	$(call say,cc,$(MODULE)/$*.cpp)
	$(CXX) $(SQUARED_CXXFLAGS) $(SQUARED_PCH_USE) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/time $(TARGET)

-include $(DEPENDS)
