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
            -I$(SQUARED_ROOT)/scene2d/include \
            -I$(SQUARED_ROOT)/math/include $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/button.cpp \
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
