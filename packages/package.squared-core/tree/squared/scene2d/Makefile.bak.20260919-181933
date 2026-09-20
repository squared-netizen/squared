# Build scene2d into lib/libsquared_scene2d.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := scene2d
MODULE_DIR := $(SQUARED_ROOT)/scene2d

INCLUDES := -I$(SQUARED_ROOT)/scene2d/include \
            -I$(SQUARED_ROOT)/graphics/include \
            -I$(SQUARED_ROOT)/graphics2d/include \
            -I$(SQUARED_ROOT)/math/include $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/actor.cpp \
    src/group.cpp \
    src/input_event.cpp \
    src/stage.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/scene2d/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_scene2d.a

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/scene2d/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/scene2d $(TARGET)

-include $(DEPENDS)
