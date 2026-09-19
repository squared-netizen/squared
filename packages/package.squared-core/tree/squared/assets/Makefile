# Build assets into lib/libsquared_assets.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := assets
MODULE_DIR := $(SQUARED_ROOT)/assets

INCLUDES := -I$(SQUARED_ROOT)/assets/include \
            -I$(SQUARED_ROOT)/files/include $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/asset_manager.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/assets/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_assets.a

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/assets/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/assets $(TARGET)

-include $(DEPENDS)
