# Build gui into lib/libsquared_gui.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := gui
MODULE_DIR := $(SQUARED_ROOT)/gui

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/actor.cpp \
    src/actor_root.cpp \
    src/batch_painter.cpp \
    src/button.cpp \
    src/button_group.cpp \
    src/cell.cpp \
    src/check_box.cpp \
    src/color_drawable.cpp \
    src/font_resource.cpp \
    src/group.cpp \
    src/image.cpp \
    src/input_event.cpp \
    src/instantiate.cpp \
    src/label.cpp \
    src/layout.cpp \
    src/layout_file.cpp \
    src/layout_writer.cpp \
    src/linear_layout.cpp \
    src/list.cpp \
    src/margin_container.cpp \
    src/nine_patch_drawable.cpp \
    src/painter.cpp \
    src/pane_frame.cpp \
    src/panel.cpp \
    src/progress_bar.cpp \
    src/radio_button.cpp \
    src/region_drawable.cpp \
    src/scroll_pane.cpp \
    src/separator.cpp \
    src/skin.cpp \
    src/skin_atlas_resolver.cpp \
    src/skin_loader.cpp \
    src/slider.cpp \
    src/stack.cpp \
    src/table.cpp \
    src/text_field.cpp \
    src/toggle_button.cpp \
    src/ui.cpp \
    src/widget.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/gui/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_gui.a


.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(call say,ar,$(notdir $@))
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/gui/%.o: $(MODULE_DIR)/%.cpp $(SQUARED_FLAGS_STAMP) $(SQUARED_PCH_FILE)
	@mkdir -p $(dir $@)
	$(call say,cc,$(MODULE)/$*.cpp)
	$(CXX) $(SQUARED_CXXFLAGS) $(SQUARED_PCH_USE) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/gui $(TARGET)

-include $(DEPENDS)

# BatchPainter against the headless backend: draw-call counts and the scissor
# conversion, neither of which can be seen by looking at the screen.
PAINTER_TEST := $(BUILD_DIR)/batch_painter_test

# Ui::set_safe_area against the whole module: where the content is placed.
SAFE_AREA_TEST := $(BUILD_DIR)/ui_safe_area_test

# The skin loader against a skin written inline: that the libGDX class names in
# a real uiskin.json are still the ones it recognises.
SKIN_LOADER_TEST := $(BUILD_DIR)/skin_loader_test

# Keys and navigation from an app::Event to the focused widget: every named key
# arrives as itself, with its modifiers.
KEYS_TEST := $(BUILD_DIR)/ui_keys_test

# A pane's frame shown by the Ui (7.7): its background and insets, the dim
# and the taps of a modal one, closing it.
PANE_TEST := $(BUILD_DIR)/pane_frame_test

# Layouts: the checker, the C++ writer (the same table a compiled layout file
# is, and no allocation), typed taps, and the instantiator with its hooks; a
# fuzz of random layouts. (console has a layout_test of its own: its text.)
SCREEN_LAYOUT_TEST := $(BUILD_DIR)/screen_layout_test

# Layout files and names.json: what the reader accepts and refuses, each
# problem's place and words, and that a file gives the C++ writer's table.
# The cases are data, read as it runs: a prerequisite, so that a changed case
# makes a new binary and the test runs again.
LAYOUT_FILE_TEST := $(BUILD_DIR)/layout_file_test
LAYOUT_FILE_CASES := $(MODULE_DIR)/test/layout_file_cases.txt

# sqgen end to end: built here against the archives (an app's build compiles
# it from source, for its machine: mk/pkg_squared_core.mk), run on test/sqgen/,
# and a test compiled with what it wrote.
SQGEN := $(BUILD_DIR)/sqgen
SQGEN_FIXTURE := $(MODULE_DIR)/test/sqgen
SQGEN_GEN := $(BUILD_DIR)/sqgen_gen
SQGEN_TEST := $(BUILD_DIR)/sqgen_test
# The wrong files sqgen must refuse, and what it must say: read as the test runs.
SQGEN_WRONG := $(wildcard $(SQGEN_FIXTURE)/bad/*.* $(SQGEN_FIXTURE)/bad/layouts/*.json \
                          $(SQGEN_FIXTURE)/keyword/*.*)

# What a host program that reads layouts links: gui first, then what it uses.
LAYOUT_LIBS := $(TARGET) $(LIB_DIR)/libsquared_graphics.a $(LIB_DIR)/libsquared_files.a \
               $(LIB_DIR)/libsquared_messaging.a $(LIB_DIR)/libsquared_data.a \
               $(LIB_DIR)/libsquared_time.a $(LIB_DIR)/libsquared_math.a

.PHONY: test
TESTS := $(PAINTER_TEST) $(SAFE_AREA_TEST) $(SKIN_LOADER_TEST) $(KEYS_TEST) $(PANE_TEST) \
         $(SCREEN_LAYOUT_TEST) $(LAYOUT_FILE_TEST) $(SQGEN_TEST)

test: $(TESTS)
	$(call run_tests,$(TESTS))

$(TESTS): $(SQUARED_FLAGS_STAMP) $(SQUARED_TEST_PCH_FILE)

# What each test includes, from its last build (tools/test_cc.sh).
-include $(TESTS:=.d) $(SQGEN).d

$(SCREEN_LAYOUT_TEST): $(MODULE_DIR)/test/screen_layout_test.cpp $(TARGET) \
                       $(LIB_DIR)/libsquared_graphics.a $(LIB_DIR)/libsquared_files.a \
                       $(LIB_DIR)/libsquared_messaging.a $(LIB_DIR)/libsquared_time.a \
                       $(LIB_DIR)/libsquared_math.a \
                       $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) $< $(SQUARED_ROOT)/dev/src/frame_allocations.cpp \
	    $(SQUARED_ROOT)/dev/src/dev_log.cpp $(TARGET) \
	    $(LIB_DIR)/libsquared_graphics.a $(LIB_DIR)/libsquared_files.a \
	    $(LIB_DIR)/libsquared_messaging.a $(LIB_DIR)/libsquared_time.a \
	    $(LIB_DIR)/libsquared_math.a -ldl -pthread -o $@

$(LAYOUT_FILE_TEST): $(MODULE_DIR)/test/layout_file_test.cpp $(LAYOUT_LIBS) $(LAYOUT_FILE_CASES)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) -DLAYOUT_FILE_CASES='"$(LAYOUT_FILE_CASES)"' \
	    $< $(LAYOUT_LIBS) -o $@

$(SQGEN): $(SQUARED_ROOT)/tools/sqgen/sqgen.cpp $(LAYOUT_LIBS) $(SQUARED_FLAGS_STAMP) $(SQUARED_TEST_PCH_FILE)
	@mkdir -p $(dir $@)
	$(call say,bin,sqgen)
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) $< $(LAYOUT_LIBS) -o $@

$(SQGEN_GEN)/layouts.cpp: $(SQGEN) $(SQGEN_FIXTURE)/names.json $(wildcard $(SQGEN_FIXTURE)/layouts/*.json)
	@mkdir -p $(dir $@)
	@$(SQGEN) --names $(SQGEN_FIXTURE)/names.json --layouts $(SQGEN_FIXTURE)/layouts --out $(SQGEN_GEN)
	@touch $@

$(SQGEN_TEST): $(MODULE_DIR)/test/sqgen_test.cpp $(SQGEN_GEN)/layouts.cpp $(LAYOUT_LIBS) $(SQGEN_WRONG)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) -I$(SQGEN_GEN) -DSQGEN_PATH='"$(SQGEN)"' \
	    -DSQGEN_FIXTURE='"$(SQGEN_FIXTURE)"' $< $(SQGEN_GEN)/layouts.cpp $(LAYOUT_LIBS) -o $@

$(KEYS_TEST): $(MODULE_DIR)/test/ui_keys_test.cpp $(TARGET) \
              $(LIB_DIR)/libsquared_graphics.a $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_math.a
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) $< $(TARGET) \
	    $(LIB_DIR)/libsquared_graphics.a \
	    $(LIB_DIR)/libsquared_math.a $(LIB_DIR)/libsquared_files.a -o $@

$(PANE_TEST): $(MODULE_DIR)/test/pane_frame_test.cpp $(TARGET) \
              $(LIB_DIR)/libsquared_graphics.a $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_math.a
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) $< $(TARGET) \
	    $(LIB_DIR)/libsquared_graphics.a \
	    $(LIB_DIR)/libsquared_math.a $(LIB_DIR)/libsquared_files.a -o $@

$(SKIN_LOADER_TEST): $(MODULE_DIR)/test/skin_loader_test.cpp $(TARGET) \
                     $(LIB_DIR)/libsquared_graphics.a $(LIB_DIR)/libsquared_files.a \
                     $(LIB_DIR)/libsquared_data.a $(LIB_DIR)/libsquared_time.a \
                     $(LIB_DIR)/libsquared_math.a
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) $< $(TARGET) \
	    $(LIB_DIR)/libsquared_graphics.a $(LIB_DIR)/libsquared_files.a \
	    $(LIB_DIR)/libsquared_data.a $(LIB_DIR)/libsquared_time.a \
	    $(LIB_DIR)/libsquared_math.a -o $@

$(SAFE_AREA_TEST): $(MODULE_DIR)/test/ui_safe_area_test.cpp $(TARGET) \
                   $(LIB_DIR)/libsquared_graphics.a $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_math.a
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) $< $(TARGET) \
	    $(LIB_DIR)/libsquared_graphics.a \
	    $(LIB_DIR)/libsquared_math.a $(LIB_DIR)/libsquared_files.a -o $@

$(PAINTER_TEST): $(MODULE_DIR)/test/batch_painter_test.cpp \
                 $(MODULE_DIR)/src/batch_painter.cpp \
                 $(MODULE_DIR)/src/painter.cpp \
                 $(MODULE_DIR)/src/font_resource.cpp \
                 $(LIB_DIR)/libsquared_graphics.a $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_math.a
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) $(SQUARED_TEST_INPUTS) \
	    $(LIB_DIR)/libsquared_graphics.a \
	    $(LIB_DIR)/libsquared_math.a $(LIB_DIR)/libsquared_files.a -o $@

# The spike: load a real skin and paint a Label and a Button.
#
#   make -C gui spike SKIN=/path/to/a/skin/directory
#
# Not part of `test`: it needs skin files, which the framework does not ship.
# It is how a skin is proved loadable end to end - atlas, .fnt, font pages,
# the JSON, the painter and the widgets - without a device.
SKIN_SPIKE := $(BUILD_DIR)/skin_spike
-include $(SKIN_SPIKE).d
SKIN ?=

.PHONY: spike
spike: $(SKIN_SPIKE)
	@if [ -z "$(SKIN)" ]; then \
	  echo "usage: make -C gui spike SKIN=/path/to/skin"; exit 2; \
	fi
	@$(SKIN_SPIKE) $(SKIN)

$(SKIN_SPIKE): $(MODULE_DIR)/test/skin_spike.cpp \
               $(LIB_DIR)/libsquared_gui.a $(LIB_DIR)/libsquared_graphics.a $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_data.a \
              $(LIB_DIR)/libsquared_time.a \
              $(LIB_DIR)/libsquared_math.a $(SQUARED_FLAGS_STAMP) $(SQUARED_TEST_PCH_FILE)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) $< \
	    $(LIB_DIR)/libsquared_gui.a \
	    $(LIB_DIR)/libsquared_graphics.a \
	    $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_data.a \
	    $(LIB_DIR)/libsquared_time.a $(LIB_DIR)/libsquared_math.a -o $@
