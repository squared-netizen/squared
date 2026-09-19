# Build math into lib/libsquared_math.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := math
MODULE_DIR := $(SQUARED_ROOT)/math

INCLUDES := -I$(SQUARED_ROOT)/math/include $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/matrix4.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/math/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_math.a

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/math/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/math $(TARGET)

-include $(DEPENDS)
