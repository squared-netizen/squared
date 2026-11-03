# Build font into lib/libsquared_font.a
#
# Font faces read and rendered at run time (Sprint 12.3, sqterm), by
# FreeType 2.15, pinned in third_party/freetype/ (tools/pin_freetype.sh) and
# compiled here into the same archive, as data compiles yyjson: an app links
# one library, and one that never makes a Face pays nothing (the linker
# keeps only what is called). Which FreeType modules and options:
# src/freetype/freetype/config/, squared's own, first on the include path.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := font
MODULE_DIR := $(SQUARED_ROOT)/font
FREETYPE_DIR := $(SQUARED_ROOT)/third_party/freetype

FREETYPE_INCLUDES := -I$(MODULE_DIR)/src/freetype -I$(FREETYPE_DIR)/include
INCLUDES := $(SQUARED_INCLUDE) $(FREETYPE_INCLUDES) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/face.cpp \
    src/font_set.cpp \
    src/glyph_atlas.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/font/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)

# FreeType: each module's one-file build (truetype.c includes the rest of
# truetype/), the base layer's, and the synthesis and bitmap helpers that
# bold and italic need.
FREETYPE_SOURCES := base/ftbase.c base/ftinit.c base/ftsystem.c base/ftdebug.c \
    base/ftbitmap.c base/ftsynth.c base/ftglyph.c base/ftmm.c \
    truetype/truetype.c sfnt/sfnt.c smooth/smooth.c autofit/autofit.c \
    cff/cff.c psaux/psaux.c pshinter/pshinter.c psnames/psnames.c
FREETYPE_OBJECTS := $(patsubst %.c,$(OBJ_DIR)/font/freetype/%.o,$(FREETYPE_SOURCES))
OBJECTS += $(FREETYPE_OBJECTS)
THIRD_PARTY_OBJECTS := $(FREETYPE_OBJECTS)

TARGET := $(LIB_DIR)/libsquared_font.a

.PHONY: all clean test
all: $(TARGET)

# A face from the test font (JetBrains Mono, test/data/, read from the
# module's directory, where tests run): metrics, glyphs, bold and italic
# made, a missing glyph, errors, moves; a FontSet with the system's
# fallbacks (CJK when the machine has Noto CJK); the glyph atlas.
FONT_TEST := $(BUILD_DIR)/font_test

TESTS := $(FONT_TEST)

test: $(TESTS)
	$(call run_tests,$(TESTS))

$(TESTS): $(SQUARED_FLAGS_STAMP)

-include $(TESTS:=.d)

$(FONT_TEST): $(MODULE_DIR)/test/font_test.cpp $(MODULE_DIR)/src/face.cpp \
              $(MODULE_DIR)/src/font_set.cpp $(MODULE_DIR)/src/glyph_atlas.cpp $(FREETYPE_OBJECTS)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(FREETYPE_INCLUDES) $(SQUARED_TEST_INPUTS) -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(call say_archive,$@)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/font/%.o: $(MODULE_DIR)/%.cpp $(SQUARED_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(SQUARED_CC) $(SQUARED_CXXFLAGS) $(INCLUDES) -c $< -o $@

# Third-party code: C, squared's warnings off (FreeType's own to keep).
$(OBJ_DIR)/font/freetype/%.o: $(FREETYPE_DIR)/src/%.c $(SQUARED_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(SQUARED_CC) -x c -std=c11 $(OPTIMISE) $(CODEGEN) $(SQUARED_PREFIX_MAP) -w \
	    -DFT2_BUILD_LIBRARY $(FREETYPE_INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/font $(TARGET)

-include $(DEPENDS)
-include $(FREETYPE_OBJECTS:.o=.d)

# C++20 modules (build/modules.mk, 7b.4): this module's interface units, the
# BMIs its code imports, and the order of both.
include $(SQUARED_ROOT)/build/modules.mk
