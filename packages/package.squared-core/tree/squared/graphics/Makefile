# Build graphics into lib/libsquared_graphics.a
#
# Context, Texture and SpriteBatch each have one implementation chosen at link
# time. Pick the backend on the command line or in local.mk:
#
#   make SQUARED_GRAPHICS_BACKEND=null    headless; the default
#   make SQUARED_GRAPHICS_BACKEND=gles    EGL + GLES 3.0, needs the NDK
#
# A new backend is a new directory under src/ holding context.cpp,
# sprite_batch.cpp and texture.cpp. No header and no other module changes.
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

# Texture is split the way graphics::Context is: a backend-independent half in
# src/common, and a GPU half chosen at link time. The null backend hands out
# names from a counter so every observable - valid(), has_content(), the
# generation counter - behaves as it does against a real driver.
SQUARED_GRAPHICS_BACKEND ?= null

MODULE := graphics
MODULE_DIR := $(SQUARED_ROOT)/graphics

STB_DIR := $(SQUARED_ROOT)/third_party/stb

INCLUDES := $(SQUARED_INCLUDE) -I$(STB_DIR) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/$(SQUARED_GRAPHICS_BACKEND)/context.cpp \
    src/detail/image_decode.cpp \
    src/common/atlas_region.cpp \
    src/common/sprite_batch_common.cpp \
    src/common/texture_atlas.cpp \
    src/common/texture_common.cpp \
    src/$(SQUARED_GRAPHICS_BACKEND)/sprite_batch.cpp \
    src/$(SQUARED_GRAPHICS_BACKEND)/texture.cpp \
    src/bitmap_font.cpp \
    src/glyph_layout.cpp \
    src/orthographic_camera.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/graphics/%.o,$(SOURCES))

# Vendored third-party C, compiled into the same archive with squared's
# warning set switched off. Same treatment as yyjson in the data module.
STB_OBJECT := $(OBJ_DIR)/graphics/third_party/stb_image.o
OBJECTS += $(STB_OBJECT)
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_graphics.a

.PHONY: all clean test
all: $(TARGET)

# A backend is a directory under src/ that holds context.cpp. Anything else
# under src/ - common/, detail/, the .cpp files - is not one.
$(MODULE_DIR)/src/$(SQUARED_GRAPHICS_BACKEND)/context.cpp:
	@echo "graphics: no backend named '$(SQUARED_GRAPHICS_BACKEND)'."
	@echo "graphics: available:" $(patsubst $(MODULE_DIR)/src/%/context.cpp,%,$(wildcard $(MODULE_DIR)/src/*/context.cpp))
	@false

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

# Laying the same text out again allocates nothing. Counted with the dev
# module's frame-allocation counter, linked into this test only.
GLYPH_TEST := $(BUILD_DIR)/glyph_layout_test
# What load() and set_text() return when they fail; a failed layout is counted
# by the frame-allocation counter too.
FONT_ERROR_TEST := $(BUILD_DIR)/bitmap_font_error_test

test: $(TEST_BINARY) $(TEXTURE_TEST) $(ATLAS_TEST) $(BATCH_TEST) $(GLYPH_TEST) $(FONT_ERROR_TEST)
	@$(TEST_BINARY)
	@$(TEXTURE_TEST)
	@$(ATLAS_TEST)
	@$(BATCH_TEST)
	@$(GLYPH_TEST)
	@$(FONT_ERROR_TEST)

$(FONT_ERROR_TEST): $(MODULE_DIR)/test/bitmap_font_error_test.cpp \
                    $(MODULE_DIR)/src/glyph_layout.cpp \
                    $(MODULE_DIR)/src/bitmap_font.cpp \
                    $(SQUARED_ROOT)/dev/src/frame_allocations.cpp \
                    $(SQUARED_ROOT)/dev/src/dev_log.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g -I$(MODULE_DIR)/src $(INCLUDES) $^ -ldl -pthread -o $@

$(GLYPH_TEST): $(MODULE_DIR)/test/glyph_layout_test.cpp \
               $(MODULE_DIR)/src/glyph_layout.cpp \
               $(MODULE_DIR)/src/bitmap_font.cpp \
               $(SQUARED_ROOT)/dev/src/frame_allocations.cpp \
               $(SQUARED_ROOT)/dev/src/dev_log.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g -I$(MODULE_DIR)/src $(INCLUDES) $^ -ldl -pthread -o $@

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
	    $(INCLUDES) $^ $(LIB_DIR)/libsquared_graphics.a \
	    $(LIB_DIR)/libsquared_math.a $(LIB_DIR)/libsquared_files.a -o $@

$(ATLAS_TEST): $(MODULE_DIR)/test/texture_atlas_test.cpp \
               $(MODULE_DIR)/src/common/atlas_region.cpp \
               $(MODULE_DIR)/src/common/texture_atlas.cpp \
               $(MODULE_DIR)/src/common/texture_common.cpp \
               $(MODULE_DIR)/src/null/texture.cpp \
               $(MODULE_DIR)/src/detail/image_decode.cpp \
               $(STB_OBJECT) \
               $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_graphics.a
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g -I$(STB_DIR) -I$(MODULE_DIR)/src \
	    $(INCLUDES) $^ $(LIB_DIR)/libsquared_graphics.a \
	    $(LIB_DIR)/libsquared_files.a -o $@

$(TEXTURE_TEST): $(MODULE_DIR)/test/texture_test.cpp \
                 $(MODULE_DIR)/src/common/atlas_region.cpp \
                 $(MODULE_DIR)/src/common/texture_common.cpp \
                 $(MODULE_DIR)/src/null/texture.cpp \
                 $(MODULE_DIR)/src/detail/image_decode.cpp \
                 $(STB_OBJECT) \
                 $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_graphics.a
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g -I$(STB_DIR) -I$(MODULE_DIR)/src \
	    $(INCLUDES) $^ $(LIB_DIR)/libsquared_graphics.a \
	    $(LIB_DIR)/libsquared_files.a -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/graphics/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

$(STB_DIR)/stb_image.h:
	@echo "graphics: third_party/stb/stb_image.h is missing."
	@echo "graphics: run ./tools/vendor_third_party.sh to check dependencies."
	@false

$(STB_OBJECT): $(STB_DIR)/stb_image_impl.c $(STB_DIR)/stb_image.h
	@mkdir -p $(dir $@)
	$(CXX) -x c -std=c11 $(OPTIMISE) $(CODEGEN) -w $(DEPFLAGS) \
	    -I$(STB_DIR) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/graphics $(TARGET)

-include $(DEPENDS)
-include $(STB_OBJECT:.o=.d)
