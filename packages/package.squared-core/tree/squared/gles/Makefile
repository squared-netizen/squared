# Build gles into lib/libsquared_gles.a
#
# Public GL object layer: Buffer, Shader, Program, VertexArray, Texture2D.
#
# This module needs EGL and GLES 3.0 headers, so it is only built when the
# graphics backend is gles. Point the build at the NDK sysroot in local.mk:
#
#   SQUARED_EXTERNAL_INCLUDES += -idirafter $(SQ_NDK_INC)
#
# Use -idirafter, not -I: the NDK sysroot is a second complete set of C headers
# and putting it ahead of libc++ makes <cctype> find the wrong ctype.h.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := gles
MODULE_DIR := $(SQUARED_ROOT)/gles

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/buffer.cpp \
    src/gles_error.cpp \
    src/program.cpp \
    src/shader.cpp \
    src/texture2d.cpp \
    src/vertex_array.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/gles/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_gles.a

.PHONY: all clean test
all: $(TARGET)

# Run the module against a fake GLES driver.
#
# test/gl_stub.cpp defines the GL entry points this module calls, compiled
# against the real GL headers so every signature is checked by the compiler
# rather than by hand. It records calls, which lets the test assert things a
# real driver cannot be asked about - above all that invalidate() does not
# call glDelete*, which on a device would silently free an unrelated object in
# the new context.
#
# This proves the module's own logic and call sequence. It proves nothing about
# what a real driver accepts.
TEST_BINARY := $(BUILD_DIR)/gles_test

TESTS := $(TEST_BINARY)

test: $(TESTS)
	$(call run_tests,$(TESTS))

$(TESTS): $(SQUARED_FLAGS_STAMP) $(SQUARED_TEST_PCH_FILE)

# What each test includes, from its last build (tools/test_cc.sh).
-include $(TESTS:=.d)

$(TEST_BINARY): $(MODULE_DIR)/test/gles_test.cpp \
                $(MODULE_DIR)/test/gl_stub.cpp \
                $(wildcard $(MODULE_DIR)/src/*.cpp)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) $(SQUARED_TEST_INPUTS) -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(call say,ar,$(notdir $@))
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/gles/%.o: $(MODULE_DIR)/%.cpp $(SQUARED_FLAGS_STAMP) $(SQUARED_PCH_FILE)
	@mkdir -p $(dir $@)
	$(call say,cc,$(MODULE)/$*.cpp)
	$(CXX) $(SQUARED_CXXFLAGS) $(SQUARED_PCH_USE) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/gles $(TARGET)

-include $(DEPENDS)
