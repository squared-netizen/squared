# Build stage into lib/libsquared_stage.a: the App and the Stage (Sprint 7).
#
# Generated file list. Adding a type means adding its .cpp here; the one
# type per file rule keeps the diff readable.

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := stage
MODULE_DIR := $(SQUARED_ROOT)/stage

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

# catalog.cpp alone holds the weak layouts::all(): a static archive brings it
# in only for an app that has no layout files. default_skin.cpp is the
# default skin's files as constant bytes, which tools/embed_files.sh writes
# from skin/default/skin/ (stage_test checks that they match).
SOURCES := src/app.cpp \
    src/catalog.cpp \
    src/default_skin.cpp \
    src/save.cpp \
    src/stage.cpp

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/stage/%.o,$(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)
TARGET := $(LIB_DIR)/libsquared_stage.a

.PHONY: all clean test
all: $(TARGET)

# What a program with an App links: stage first, then what it uses.
STAGE_LIBS := $(TARGET) $(LIB_DIR)/libsquared_gui.a $(LIB_DIR)/libsquared_graphics.a \
              $(LIB_DIR)/libsquared_assets.a $(LIB_DIR)/libsquared_files.a \
              $(LIB_DIR)/libsquared_messaging.a $(LIB_DIR)/libsquared_data.a \
              $(LIB_DIR)/libsquared_time.a $(LIB_DIR)/libsquared_math.a

# The App and its Stage on the null backend, with the embedded default skin
# and C++ layouts: navigation, taps to the model, no allocation in a frame,
# leaving, quitting, an activity made again. It defines layouts::all()
# itself, as sqgen does, and links the framework's default as an object, not
# through the archive: only a weak default links beside it. It compares the
# embedded skin with its files, so they are its prerequisites.
STAGE_TEST := $(BUILD_DIR)/stage_test
CATALOG_OBJECT := $(OBJ_DIR)/stage/src/catalog.o
STAGE_SKIN_DIR := $(MODULE_DIR)/skin/default/skin

# The same App with no layout files: the framework's own empty layouts::all().
STAGE_ALONE_TEST := $(BUILD_DIR)/stage_alone_test

TESTS := $(STAGE_TEST) $(STAGE_ALONE_TEST)

test: $(TESTS)
	$(call run_tests,$(TESTS))

$(TESTS): $(SQUARED_FLAGS_STAMP) $(SQUARED_TEST_PCH_FILE) $(STAGE_LIBS)

# What each test includes, from its last build (tools/test_cc.sh).
-include $(TESTS:=.d)

$(STAGE_TEST): $(MODULE_DIR)/test/stage_test.cpp $(CATALOG_OBJECT) $(wildcard $(STAGE_SKIN_DIR)/*)
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) -DSTAGE_SKIN_DIR='"$(STAGE_SKIN_DIR)"' $< \
	    $(SQUARED_ROOT)/dev/src/frame_allocations.cpp $(SQUARED_ROOT)/dev/src/dev_log.cpp \
	    $(CATALOG_OBJECT) $(STAGE_LIBS) -ldl -pthread -o $@

$(STAGE_ALONE_TEST): $(MODULE_DIR)/test/stage_alone_test.cpp
	@mkdir -p $(dir $@)
	$(call say,bin,$(MODULE)/$(notdir $@))
	$(TEST_CXX) $(TESTFLAGS) $(INCLUDES) $< \
	    $(STAGE_LIBS) -ldl -pthread -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(call say,ar,$(notdir $@))
	$(AR) rcs $@ $(OBJECTS)

$(OBJ_DIR)/stage/%.o: $(MODULE_DIR)/%.cpp $(SQUARED_FLAGS_STAMP) $(SQUARED_PCH_FILE)
	@mkdir -p $(dir $@)
	$(call say,cc,$(MODULE)/$*.cpp)
	$(CXX) $(SQUARED_CXXFLAGS) $(SQUARED_PCH_USE) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/stage $(TARGET)

-include $(DEPENDS)
