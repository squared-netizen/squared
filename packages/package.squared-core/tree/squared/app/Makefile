# Build app into lib/libsquared_app.a: sq.app, the application model an app
# and its platform layer share (Application, Event, the keys, text input). It
# has no sources of its own: the archive holds its interfaces' objects
# (7b.4; until then app/ was headers only, and not built).

include $(dir $(lastword $(MAKEFILE_LIST)))../build/common.mk

MODULE := app
MODULE_DIR := $(SQUARED_ROOT)/app

INCLUDES := $(SQUARED_INCLUDE) $(SQUARED_EXTERNAL_INCLUDES) -I$(MODULE_DIR)/src

OBJECTS :=
TARGET := $(LIB_DIR)/libsquared_app.a

.PHONY: all clean
all: $(TARGET)

$(TARGET):
	@mkdir -p $(LIB_DIR)
	$(call say_archive,$@)
	$(AR) rcs $@ $(OBJECTS)

clean:
	rm -rf $(OBJ_DIR)/app $(TARGET)

# C++20 modules (build/modules.mk, 7b.4): this module's interface units, the
# BMIs its code imports, and the order of both.
include $(SQUARED_ROOT)/build/modules.mk
