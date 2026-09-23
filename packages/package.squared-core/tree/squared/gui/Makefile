# Build gui into lib/libsquared_gui.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := gui
MODULE_DIR := $(SQUARED_ROOT)/gui

INCLUDES := -I$(SQUARED_ROOT)/gui/include \
            -I$(SQUARED_ROOT)/app/include \
            -I$(SQUARED_ROOT)/data/include \
            -I$(SQUARED_ROOT)/graphics/include \
            -I$(SQUARED_ROOT)/graphics2d/include \
            -I$(SQUARED_ROOT)/files/include \
            -I$(SQUARED_ROOT)/scene2d/include \
            -I$(SQUARED_ROOT)/math/include $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/button.cpp \
    src/batch_painter.cpp \
    src/button_group.cpp \
    src/cell.cpp \
    src/check_box.cpp \
    src/color_drawable.cpp \
    src/dialog.cpp \
    src/font_resource.cpp \
    src/image.cpp \
    src/label.cpp \
    src/linear_layout.cpp \
    src/margin_container.cpp \
    src/nine_patch_drawable.cpp \
    src/painter.cpp \
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
    src/widget.cpp \
    src/window.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/gui/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_gui.a


.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/gui/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/gui $(TARGET)

-include $(DEPENDS)

# BatchPainter against the headless backend: draw-call counts and the scissor
# conversion, neither of which can be seen by looking at the screen.
PAINTER_TEST := $(BUILD_DIR)/batch_painter_test

.PHONY: test
test: $(PAINTER_TEST)
	@$(PAINTER_TEST)

$(PAINTER_TEST): $(MODULE_DIR)/test/batch_painter_test.cpp \
                 $(MODULE_DIR)/src/batch_painter.cpp \
                 $(MODULE_DIR)/src/painter.cpp \
                 $(MODULE_DIR)/src/font_resource.cpp \
                 $(LIB_DIR)/libsquared_graphics2d.a $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_math.a
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g $(INCLUDES) $^ \
	    $(LIB_DIR)/libsquared_graphics2d.a $(LIB_DIR)/libsquared_math.a \
	    $(LIB_DIR)/libsquared_files.a -o $@

# The spike: load a real skin and paint a Label and a Button.
#
#   make -C gui spike SKIN=/path/to/a/skin/directory
#
# Not part of `test`: it needs skin files, which the framework does not ship.
# It is how a skin is proved loadable end to end - atlas, .fnt, font pages,
# the JSON, the painter and the widgets - without a device.
SKIN_SPIKE := $(BUILD_DIR)/skin_spike
SKIN ?=

.PHONY: spike
spike: $(SKIN_SPIKE)
	@if [ -z "$(SKIN)" ]; then \
	  echo "usage: make -C gui spike SKIN=/path/to/skin"; exit 2; \
	fi
	@$(SKIN_SPIKE) $(SKIN)

$(SKIN_SPIKE): $(MODULE_DIR)/test/skin_spike.cpp \
               $(LIB_DIR)/libsquared_gui.a $(LIB_DIR)/libsquared_scene2d.a $(LIB_DIR)/libsquared_graphics2d.a $(LIB_DIR)/libsquared_graphics.a $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_data.a \
              $(LIB_DIR)/libsquared_time.a \
              $(LIB_DIR)/libsquared_math.a
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g $(INCLUDES) $< \
	    $(LIB_DIR)/libsquared_gui.a $(LIB_DIR)/libsquared_scene2d.a \
	    $(LIB_DIR)/libsquared_graphics2d.a $(LIB_DIR)/libsquared_graphics.a \
	    $(LIB_DIR)/libsquared_files.a $(LIB_DIR)/libsquared_data.a \
	    $(LIB_DIR)/libsquared_time.a $(LIB_DIR)/libsquared_math.a -o $@
