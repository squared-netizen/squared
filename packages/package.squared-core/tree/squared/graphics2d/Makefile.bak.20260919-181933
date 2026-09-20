# Build graphics2d into lib/libsquared_graphics2d.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := graphics2d
MODULE_DIR := $(SQUARED_ROOT)/graphics2d

STB_DIR := $(SQUARED_ROOT)/third_party/stb

INCLUDES := -I$(STB_DIR) -I$(SQUARED_ROOT)/graphics2d/include \
            -I$(SQUARED_ROOT)/graphics/include \
            -I$(SQUARED_ROOT)/math/include $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/detail/image_decode.cpp \
    src/bitmap_font.cpp \
    src/glyph_layout.cpp \
    src/orthographic_camera.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/graphics2d/%.o,$(SOURCES))

# Vendored third-party C, compiled into the same archive with squared's
# warning set switched off. Same treatment as yyjson in the data module.
STB_OBJECT := $(OBJ_DIR)/graphics2d/third_party/stb_image.o
OBJECTS += $(STB_OBJECT)
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_graphics2d.a

.PHONY: all clean test
all: $(TARGET)

# Round-trip the image decoder against a PNG it encodes itself.
#
# stb_image_write is vendored alongside stb_image for exactly this: the test
# encodes a known 3x2 image, decodes it, and compares byte for byte. Width 3 is
# deliberate - it is the row-alignment case that breaks naive decoding.
TEST_BINARY := $(BUILD_DIR)/image_decode_test

test: $(TEST_BINARY)
	@$(TEST_BINARY)

$(TEST_BINARY): $(MODULE_DIR)/test/image_decode_test.cpp \
                $(MODULE_DIR)/src/detail/image_decode.cpp \
                $(STB_OBJECT)
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g -I$(STB_DIR) -I$(MODULE_DIR)/src $^ -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/graphics2d/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

$(STB_DIR)/stb_image.h:
	@echo "graphics2d: third_party/stb/stb_image.h is missing."
	@echo "graphics2d: run ./tools/vendor_third_party.sh to check dependencies."
	@false

$(STB_OBJECT): $(STB_DIR)/stb_image_impl.c $(STB_DIR)/stb_image.h
	@mkdir -p $(dir $@)
	$(CXX) -x c -std=c11 $(OPTIMISE) $(CODEGEN) -w $(DEPFLAGS) \
	    -I$(STB_DIR) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/graphics2d $(TARGET)

-include $(DEPENDS)
-include $(STB_OBJECT:.o=.d)
