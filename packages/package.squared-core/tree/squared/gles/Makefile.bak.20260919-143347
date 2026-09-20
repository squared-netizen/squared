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

INCLUDES := -I$(SQUARED_ROOT)/gles/include $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/buffer.cpp \
    src/gles_error.cpp \
    src/program.cpp \
    src/shader.cpp \
    src/texture2d.cpp \
    src/vertex_array.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/gles/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_gles.a

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/gles/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/gles $(TARGET)

-include $(DEPENDS)
