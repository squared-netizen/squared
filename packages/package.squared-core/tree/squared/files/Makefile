# Build files into lib/libsquared_files.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.
#
# posix_file_system.cpp is the POSIX backend. The Android asset backend for
# FileType::Internal is a second .cpp added here, not a change to any of these.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

# android_asset_file_system.cpp needs <android/asset_manager.h>, so it is only
# compiled for that platform. A separate variable from
# SQUARED_GRAPHICS_BACKEND on purpose: "has GLES" and "is Android" happen to
# coincide today and are not the same question, and overloading one for the
# other is the kind of implicit coupling that gets refactored away by someone
# who does not know.
SQUARED_PLATFORM ?= posix

MODULE := files
MODULE_DIR := $(SQUARED_ROOT)/files

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/file_handle.cpp \
    src/file_system.cpp \
    src/embedded_file_system.cpp \
    src/posix_file_system.cpp \
    $(if $(filter android,$(SQUARED_PLATFORM)),src/android_asset_file_system.cpp,)

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/files/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_files.a

.PHONY: all clean test
all: $(TARGET)

# The asset backend against a fake AAssetManager. The stub serves short reads
# the way a compressed asset does, which is the behaviour the reader loop
# exists for and the one a device would only reveal intermittently.
TEST_BINARY := $(BUILD_DIR)/android_asset_test

# The POSIX backend against real directories under $TMPDIR, with a count of
# the allocations its everyday calls make.
POSIX_TEST := $(BUILD_DIR)/posix_file_system_test

# The embedded backend over a table made in the test: lookups, directories,
# listing, the refusals, and a handle reading through it.
EMBEDDED_TEST := $(BUILD_DIR)/embedded_file_system_test

TESTS := $(TEST_BINARY) $(POSIX_TEST) $(EMBEDDED_TEST)

test: $(TESTS)
	$(call run_tests,$(TESTS))

$(TESTS): $(SQUARED_FLAGS_STAMP) $(SQUARED_TEST_PCH_FILE)

# What each test includes, from its last build (tools/test_cc.sh).
-include $(TESTS:=.d)

$(TEST_BINARY): $(MODULE_DIR)/test/android_asset_test.cpp \
                $(MODULE_DIR)/test/asset_stub.cpp \
                $(MODULE_DIR)/src/android_asset_file_system.cpp \
                $(MODULE_DIR)/src/posix_file_system.cpp \
                $(MODULE_DIR)/src/file_handle.cpp \
                $(MODULE_DIR)/src/file_system.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) \
	    -I$(MODULE_DIR)/test $(SQUARED_TEST_INPUTS) -o $@

$(POSIX_TEST): $(MODULE_DIR)/test/posix_file_system_test.cpp \
               $(MODULE_DIR)/src/posix_file_system.cpp \
               $(MODULE_DIR)/src/file_handle.cpp \
               $(MODULE_DIR)/src/file_system.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(SQUARED_TEST_INPUTS) -o $@

$(EMBEDDED_TEST): $(MODULE_DIR)/test/embedded_file_system_test.cpp \
                  $(MODULE_DIR)/src/embedded_file_system.cpp \
                  $(MODULE_DIR)/src/file_handle.cpp \
                  $(MODULE_DIR)/src/file_system.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(SQUARED_TEST_INPUTS) -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(call say,ar,$(notdir $@))
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/files/%.o: $(MODULE_DIR)/%.cpp $(SQUARED_FLAGS_STAMP) $(SQUARED_PCH_FILE)
	@mkdir -p $(dir $@)
	$(call say,cc,$(MODULE)/$*.cpp)
	$(CXX) $(SQUARED_CXXFLAGS) $(SQUARED_PCH_USE) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/files $(TARGET)

-include $(DEPENDS)
