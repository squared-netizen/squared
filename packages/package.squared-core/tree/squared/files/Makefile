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

INCLUDES := -I$(SQUARED_ROOT)/files/include $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/file_handle.cpp \
    src/file_system.cpp \
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

test: $(TEST_BINARY)
	@$(TEST_BINARY)

$(TEST_BINARY): $(MODULE_DIR)/test/android_asset_test.cpp \
                $(MODULE_DIR)/test/asset_stub.cpp \
                $(MODULE_DIR)/src/android_asset_file_system.cpp \
                $(MODULE_DIR)/src/posix_file_system.cpp \
                $(MODULE_DIR)/src/file_handle.cpp \
                $(MODULE_DIR)/src/file_system.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g -I$(SQUARED_ROOT)/files/include \
	    -I$(MODULE_DIR)/test $^ -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/files/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/files $(TARGET)

-include $(DEPENDS)
