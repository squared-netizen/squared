# Build messaging into lib/libsquared_messaging.a
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := messaging
MODULE_DIR := $(SQUARED_ROOT)/messaging

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

SOURCES := src/address.cpp \
    src/list_window.cpp \
    src/model_list.cpp \
    src/model_runner.cpp \
    src/name_table.cpp \
    src/telegram.cpp \
    src/telegram_data.cpp \
    src/telegram_ring.cpp \
    src/telegram_text.cpp \
    src/telegraph.cpp \
    src/value_cache.cpp \
    src/wake.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/messaging/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_messaging.a

.PHONY: all clean test
all: $(TARGET)

# Telegraph v2's names and addresses (Sprint 6.1), with no allocation counted.
ADDRESS_TEST := $(BUILD_DIR)/address_test

# The telegram, its text form both ways (a fuzz test) and the keys (6.1b, 6.2).
TELEGRAM_TEST := $(BUILD_DIR)/telegram_test

# The ring each direction of the Telegraph runs on (6.3): one thread, a model,
# and two real threads (so -pthread).
RING_TEST := $(BUILD_DIR)/telegram_ring_test

# The latest-value cache on the UI side (6.4): one thread, a model.
CACHE_TEST := $(BUILD_DIR)/value_cache_test

# Virtualized lists (6.5): the model's list, the widget's window, the two together.
LIST_TEST := $(BUILD_DIR)/list_test

# The Telegraph single-threaded (6.6a): both ends, held-back states, lists, lifecycle.
TELEGRAPH_TEST := $(BUILD_DIR)/telegraph_test

# The Telegraph on two threads (6.6c): the model's own thread, both wakes, the
# bounded wait, ticks, timers (6.6b), quit, and the inline runner (so -pthread).
# The runner measures its ticks with time's Timepiece, so time's sources too.
THREAD_TEST := $(BUILD_DIR)/telegraph_thread_test

TESTS := $(ADDRESS_TEST) $(TELEGRAM_TEST) $(RING_TEST) $(CACHE_TEST) $(LIST_TEST) \
    $(TELEGRAPH_TEST) $(THREAD_TEST)

test: $(TESTS)
	$(call run_tests,$(TESTS))

$(TESTS): $(SQUARED_HEADERS) $(SQUARED_FLAGS_STAMP)

$(ADDRESS_TEST): $(MODULE_DIR)/test/address_test.cpp \
                 $(MODULE_DIR)/src/address.cpp \
                 $(MODULE_DIR)/src/name_table.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter-out %.hpp $(SQUARED_FLAGS_STAMP),$^) -o $@

$(TELEGRAM_TEST): $(MODULE_DIR)/test/telegram_test.cpp \
                  $(MODULE_DIR)/src/address.cpp \
                  $(MODULE_DIR)/src/name_table.cpp \
                  $(MODULE_DIR)/src/telegram.cpp \
                  $(MODULE_DIR)/src/telegram_data.cpp \
                  $(MODULE_DIR)/src/telegram_text.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter-out %.hpp $(SQUARED_FLAGS_STAMP),$^) -o $@

$(RING_TEST): $(MODULE_DIR)/test/telegram_ring_test.cpp \
              $(MODULE_DIR)/src/telegram.cpp \
              $(MODULE_DIR)/src/telegram_ring.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter-out %.hpp $(SQUARED_FLAGS_STAMP),$^) -pthread -o $@

$(CACHE_TEST): $(MODULE_DIR)/test/value_cache_test.cpp \
               $(MODULE_DIR)/src/telegram.cpp \
               $(MODULE_DIR)/src/value_cache.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter-out %.hpp $(SQUARED_FLAGS_STAMP),$^) -o $@

$(LIST_TEST): $(MODULE_DIR)/test/list_test.cpp \
              $(MODULE_DIR)/src/telegram.cpp \
              $(MODULE_DIR)/src/model_list.cpp \
              $(MODULE_DIR)/src/list_window.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter-out %.hpp $(SQUARED_FLAGS_STAMP),$^) -o $@

$(TELEGRAPH_TEST): $(MODULE_DIR)/test/telegraph_test.cpp \
                   $(MODULE_DIR)/src/telegram.cpp \
                   $(MODULE_DIR)/src/telegram_ring.cpp \
                   $(MODULE_DIR)/src/value_cache.cpp \
                   $(MODULE_DIR)/src/model_list.cpp \
                   $(MODULE_DIR)/src/list_window.cpp \
                   $(MODULE_DIR)/src/telegraph.cpp \
                   $(MODULE_DIR)/src/name_table.cpp \
                   $(MODULE_DIR)/src/address.cpp \
                   $(MODULE_DIR)/src/wake.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter-out %.hpp $(SQUARED_FLAGS_STAMP),$^) -o $@

$(THREAD_TEST): $(MODULE_DIR)/test/telegraph_thread_test.cpp \
                $(MODULE_DIR)/src/telegram.cpp \
                $(MODULE_DIR)/src/telegram_ring.cpp \
                $(MODULE_DIR)/src/value_cache.cpp \
                $(MODULE_DIR)/src/model_list.cpp \
                $(MODULE_DIR)/src/list_window.cpp \
                $(MODULE_DIR)/src/telegraph.cpp \
                $(MODULE_DIR)/src/name_table.cpp \
                $(MODULE_DIR)/src/address.cpp \
                $(MODULE_DIR)/src/wake.cpp \
                $(MODULE_DIR)/src/model_runner.cpp \
                $(SQUARED_ROOT)/time/src/now.cpp \
                $(SQUARED_ROOT)/time/src/timepiece.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(CXX) $(TESTFLAGS) $(SQUARED_INCLUDE) $(filter-out %.hpp $(SQUARED_FLAGS_STAMP),$^) -pthread -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(call say,ar,$(notdir $@))
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/messaging/%.o: $(MODULE_DIR)/%.cpp $(SQUARED_FLAGS_STAMP)
	@mkdir -p $(dir $@)
	$(call say,cc,$(MODULE)/$*.cpp)
	$(CXX) $(SQUARED_CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/messaging $(TARGET)

-include $(DEPENDS)
