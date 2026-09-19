# Build data into lib/libsquared_data.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.
#
# json.cpp is the only translation unit that needs yyjson, and yyjson is
# vendored under third_party/ rather than taken from the system. It has to be:
# this tree is built with the NDK toolchain for an APK, so a package installed
# for the host sysroot would be the wrong target.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := data
MODULE_DIR := $(SQUARED_ROOT)/data
YYJSON_DIR := $(SQUARED_ROOT)/third_party/yyjson

INCLUDES := -I$(SQUARED_ROOT)/data/include \
            -I$(YYJSON_DIR) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/json.cpp \
    src/json_value.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/data/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)

# Vendored third-party C, compiled into the same archive so an application
# links one library rather than two.
YYJSON_OBJECT := $(OBJ_DIR)/data/third_party/yyjson.o
OBJECTS += $(YYJSON_OBJECT)

TARGET := $(LIB_DIR)/libsquared_data.a

.PHONY: all clean
all: $(TARGET)

$(YYJSON_DIR)/yyjson.h:
	@echo "data: third_party/yyjson is missing."
	@echo "data: run ./tools/vendor_third_party.sh to unpack it."
	@false

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/data/%.o: $(MODULE_DIR)/%.cpp $(YYJSON_DIR)/yyjson.h
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

# Third-party code is compiled as C with squared's warning set switched off.
# It is not ours to keep clean, and -Wconversion on a JSON parser is noise that
# would bury a real warning from squared.
$(YYJSON_OBJECT): $(YYJSON_DIR)/yyjson.c $(YYJSON_DIR)/yyjson.h
	@mkdir -p $(dir $@)
	$(CXX) -x c -std=c11 $(OPTIMISE) $(CODEGEN) -w $(DEPFLAGS) \
	    -I$(YYJSON_DIR) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/data $(TARGET)

-include $(DEPENDS)
-include $(YYJSON_OBJECT:.o=.d)
