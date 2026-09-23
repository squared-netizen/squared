# Build graphics2d into lib/libsquared_graphics2d.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

# Texture is split the way graphics::Context is: a backend-independent half in
# src/common, and a GPU half chosen at link time. The null backend hands out
# names from a counter so every observable - valid(), has_content(), the
# generation counter - behaves as it does against a real driver.
SQUARED_GRAPHICS_BACKEND ?= null

MODULE := graphics2d
MODULE_DIR := $(SQUARED_ROOT)/graphics2d

STB_DIR := $(SQUARED_ROOT)/third_party/stb

INCLUDES := -I$(STB_DIR) -I$(SQUARED_ROOT)/files/include -I$(SQUARED_ROOT)/graphics2d/include \
            -I$(SQUARED_ROOT)/graphics/include \
            -I$(SQUARED_ROOT)/math/include $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/detail/image_decode.cpp \
    src/common/atlas_region.cpp \
    src/common/sprite_batch_common.cpp \
    src/common/texture_atlas.cpp \
    src/common/texture_common.cpp \
    src/$(SQUARED_GRAPHICS_BACKEND)/sprite_batch.cpp \
    src/$(SQUARED_GRAPHICS_BACKEND)/texture.cpp \
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



$(TEST_BINARY): $(MODULE_DIR)/test/image_decode_test.cpp \
                $(MODULE_DIR)/src/detail/image_decode.cpp \
                $(STB_OBJECT)
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g -I$(STB_DIR) -I$(MODULE_DIR)/src $^ -o $@

# Texture against the headless backend. Recovery runs once per context loss on
# a device; here every branch of it runs in microseconds.
TEXTURE_TEST := $(BUILD_DIR)/texture_test

ATLAS_TEST := $(BUILD_DIR)/texture_atlas_test

# Every test runs on every `make test`, not only when it was rebuilt: a suite
# that quietly skips what is up to date reports green without having run.
BATCH_TEST := $(BUILD_DIR)/sprite_batch_test

test: $(TEST_BINARY) $(TEXTURE_TEST) $(ATLAS_TEST) $(BATCH_TEST)
	@$(TEST_BINARY)
	@$(TEXTURE_TEST)
	@$(ATLAS_TEST)
	@$(BATCH_TEST)

$(BATCH_TEST): $(MODULE_DIR)/test/sprite_batch_test.cpp \
               $(MODULE_DIR)/src/common/sprite_batch_common.cpp \
               $(MODULE_DIR)/src/null/sprite_batch.cpp \
               $(MODULE_DIR)/src/common/texture_common.cpp \
               $(MODULE_DIR)/src/null/texture.cpp \
               $(MODULE_DIR)/src/orthographic_camera.cpp \
               $(MODULE_DIR)/src/detail/image_decode.cpp \
               $(STB_OBJECT) \
               $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_math.a
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g -I$(STB_DIR) -I$(MODULE_DIR)/src \
	    $(INCLUDES) $^ $(LIB_DIR)/libsquared_math.a \
	    $(LIB_DIR)/libsquared_files.a -o $@

$(ATLAS_TEST): $(MODULE_DIR)/test/texture_atlas_test.cpp \
               $(MODULE_DIR)/src/common/atlas_region.cpp \
               $(MODULE_DIR)/src/common/texture_atlas.cpp \
               $(MODULE_DIR)/src/common/texture_common.cpp \
               $(MODULE_DIR)/src/null/texture.cpp \
               $(MODULE_DIR)/src/detail/image_decode.cpp \
               $(STB_OBJECT) \
               $(LIB_DIR)/libsquared_files.a
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g -I$(STB_DIR) -I$(MODULE_DIR)/src \
	    $(INCLUDES) $^ $(LIB_DIR)/libsquared_files.a -o $@
	    $(INCLUDES) $^ $(LIB_DIR)/libsquared_files.a -o $@

$(TEXTURE_TEST): $(MODULE_DIR)/test/texture_test.cpp \
                 $(MODULE_DIR)/src/common/atlas_region.cpp \
                 $(MODULE_DIR)/src/common/texture_common.cpp \
                 $(MODULE_DIR)/src/null/texture.cpp \
                 $(MODULE_DIR)/src/detail/image_decode.cpp \
                 $(STB_OBJECT) \
                 $(LIB_DIR)/libsquared_files.a
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g -I$(STB_DIR) -I$(MODULE_DIR)/src \
	    $(INCLUDES) $^ $(LIB_DIR)/libsquared_files.a -o $@

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
