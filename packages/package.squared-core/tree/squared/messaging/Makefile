# Build messaging into lib/libsquared_messaging.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := messaging
MODULE_DIR := $(SQUARED_ROOT)/messaging

INCLUDES := -I$(SQUARED_ROOT)/messaging/include \
            -I$(SQUARED_ROOT)/time/include \
            -I$(SQUARED_ROOT)/data/include $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/endpoint_id.cpp \
    src/message_dispatcher.cpp \
    src/message_id.cpp \
    src/receipt.cpp \
    src/subscription.cpp \
    src/telegram.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/messaging/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_messaging.a

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/messaging/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/messaging $(TARGET)

-include $(DEPENDS)
