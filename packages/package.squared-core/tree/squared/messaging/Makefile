# Build messaging into lib/libsquared_messaging.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := messaging
MODULE_DIR := $(SQUARED_ROOT)/messaging

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/address.cpp \
    src/endpoint_id.cpp \
    src/message_dispatcher.cpp \
    src/message_id.cpp \
    src/name_table.cpp \
    src/receipt.cpp \
    src/subscription.cpp \
    src/telegram.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/messaging/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_messaging.a

.PHONY: all clean test
all: $(TARGET)

# Telegraph v2's names and addresses (Sprint 6.1), with no allocation counted.
ADDRESS_TEST := $(BUILD_DIR)/address_test

TESTS := $(ADDRESS_TEST)

test: $(TESTS)
	$(call run_tests,$(TESTS))

$(TESTS): $(SQUARED_HEADERS)

$(ADDRESS_TEST): $(MODULE_DIR)/test/address_test.cpp \
                 $(MODULE_DIR)/src/address.cpp \
                 $(MODULE_DIR)/src/name_table.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(STANDARD) -O1 -g $(SQUARED_INCLUDE) $(filter-out %.hpp,$^) -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/messaging/%.o: $(MODULE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/messaging $(TARGET)

-include $(DEPENDS)
