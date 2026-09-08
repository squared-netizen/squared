# kit.lua build fragment.
#
# GENERATED FILE. kit.lua owns this path and will replace it on update.
#
# Lua is acquired from the build host (manifest: external.acquisition =
# "system"), so this fragment's job is detection. Generation never needed Lua
# and never touched the network; the build does.
#
# Detection order, first hit wins:
#   1. SQ_LUA_CFLAGS / SQ_LUA_LIBS set by you       -- always respected
#   2. pkg-config, over the names distributions use
#   3. a bare -llua with the header already on the include path
#
# When nothing is found the project still builds. lua_host.hpp compiles to a
# stub that reports the situation at runtime, because a workspace that refuses
# to compile over a missing optional dependency is far more annoying to
# diagnose than one that builds and says what is wrong.

SQ_LUA_PKGCONFIG ?= pkg-config
SQ_LUA_PACKAGES  := lua5.4 lua-5.4 lua54 lua

ifeq ($(strip $(SQ_LUA_LIBS)),)
  SQ_LUA_FOUND_PKG := $(firstword $(foreach p,$(SQ_LUA_PACKAGES),\
      $(shell $(SQ_LUA_PKGCONFIG) --exists $(p) 2>/dev/null && echo $(p))))
  ifneq ($(strip $(SQ_LUA_FOUND_PKG)),)
    SQ_LUA_CFLAGS := $(shell $(SQ_LUA_PKGCONFIG) --cflags $(SQ_LUA_FOUND_PKG))
    SQ_LUA_LIBS   := $(shell $(SQ_LUA_PKGCONFIG) --libs $(SQ_LUA_FOUND_PKG))
  endif
endif

ifeq ($(strip $(SQ_LUA_LIBS)),)
  SQ_LUA_PROBE := $(shell printf '\043include <lua.h>\nint main(void){return 0;}\n' \
      > .sq_lua_probe.c 2>/dev/null && \
      $(CC) .sq_lua_probe.c -llua -o /dev/null 2>/dev/null && echo yes; \
      rm -f .sq_lua_probe.c)
  ifeq ($(SQ_LUA_PROBE),yes)
    SQ_LUA_CFLAGS :=
    SQ_LUA_LIBS   := -llua
  endif
endif

ifneq ($(strip $(SQ_LUA_LIBS)),)
  SQ_KIT_CPPFLAGS += -DSQ_HAVE_LUA $(SQ_LUA_CFLAGS)
  SQ_KIT_LDLIBS   += $(SQ_LUA_LIBS)
  SQ_LUA_STATUS   := found
else
  SQ_LUA_STATUS   := missing
endif

SQ_KIT_CPPFLAGS += -Isq_kit/include
SQ_KITS_PRESENT += kit.lua

.PHONY: lua-status
lua-status:
	@echo "kit.lua: Lua 5.4 $(SQ_LUA_STATUS)"
ifeq ($(SQ_LUA_STATUS),missing)
	@echo "  scripts in sq_lua/ will not run in this build"
	@echo "  Termux:  pkg install lua54"
	@echo "  Debian:  apt install liblua5.4-dev"
	@echo "  or set SQ_LUA_CFLAGS and SQ_LUA_LIBS by hand"
else
	@echo "  cflags: $(SQ_LUA_CFLAGS)"
	@echo "  libs:   $(SQ_LUA_LIBS)"
endif
