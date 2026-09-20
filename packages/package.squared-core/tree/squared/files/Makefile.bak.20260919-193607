# Build files into lib/libsquared_files.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.
#
# posix_file_system.cpp is the POSIX backend. The Android asset backend for
# FileType::Internal is a second .cpp added here, not a change to any of these.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := files
MODULE_DIR := $(SQUARED_ROOT)/files

INCLUDES := -I$(SQUARED_ROOT)/files/include $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/file_handle.cpp \
    src/file_system.cpp \
    src/posix_file_system.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/files/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_files.a

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/files/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/files $(TARGET)

-include $(DEPENDS)
