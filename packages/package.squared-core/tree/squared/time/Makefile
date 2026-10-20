# Build time into lib/libsquared_time.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := time
MODULE_DIR := $(SQUARED_ROOT)/time

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/manual_timepiece.cpp \
    src/timepiece.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/time/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_time.a

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(call say,ar,$(notdir $@))
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/time/%.o: $(MODULE_DIR)/%.cpp $(SQUARED_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(call say,cc,$(MODULE)/$*.cpp)
	$(CXX) $(SQUARED_CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/time $(TARGET)

-include $(DEPENDS)
