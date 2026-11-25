# Build preferences into lib/libsquared_preferences.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := preferences
MODULE_DIR := $(SQUARED_ROOT)/preferences

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/preferences.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/preferences/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_preferences.a

.PHONY: all clean test
all: $(TARGET)

# An app's preferences cartridge against a real directory: made, kept,
# typed settings, the app's files, names kept in the tree, other kinds refused.
PREFERENCES_TEST := $(BUILD_DIR)/preferences_test

TESTS := $(PREFERENCES_TEST)

test: $(TESTS)
	$(call run_tests,$(TESTS))

$(TESTS): $(SQUARED_FLAGS_STAMP)

# What each test includes, from its last build (tools/test_cc.sh).
-include $(TESTS:=.d)

PREFERENCES_LIBS := $(TARGET) $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_data.a

$(PREFERENCES_TEST): $(MODULE_DIR)/test/preferences_test.cpp $(PREFERENCES_LIBS)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) $< $(PREFERENCES_LIBS) -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(call say_archive,$@)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/preferences/%.o: $(MODULE_DIR)/%.cpp $(SQUARED_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(SQUARED_CC) $(SQUARED_CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/preferences $(TARGET)

-include $(DEPENDS)

# C++20 modules (build/modules.mk, 7b.4): this module's interface units, the
# BMIs its code imports, and the order of both.
include $(SQUARED_ROOT)/build/modules.mk
