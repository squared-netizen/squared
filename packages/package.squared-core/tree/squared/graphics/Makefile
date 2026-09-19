# Build graphics into lib/libsquared_graphics.a
#
# Context is one class with one implementation chosen at link time. Pick the
# backend on the command line or in local.mk:
#
#   make SQUARED_GRAPHICS_BACKEND=null    headless; the default
#   make SQUARED_GRAPHICS_BACKEND=gles    EGL + GLES 3.0, needs the NDK
#
# A new backend is a new directory under src/ holding context.cpp. No header
# and no other module changes.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := graphics
MODULE_DIR := $(SQUARED_ROOT)/graphics

SQUARED_GRAPHICS_BACKEND ?= null

INCLUDES := -I$(SQUARED_ROOT)/graphics/include $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/$(SQUARED_GRAPHICS_BACKEND)/context.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/graphics/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_graphics.a

.PHONY: all clean
all: $(TARGET)

$(MODULE_DIR)/src/$(SQUARED_GRAPHICS_BACKEND)/context.cpp:
	@echo "graphics: no backend named '$(SQUARED_GRAPHICS_BACKEND)'."
	@echo "graphics: available:" $(notdir $(wildcard $(MODULE_DIR)/src/*))
	@false

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/graphics/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/graphics $(TARGET)

-include $(DEPENDS)
