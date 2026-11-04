# squared-core wiring.
#
# This file is the GENERATOR'S (ownership class: generated). It is rewritten on
# update; do not edit it.
#
# It ships with package.squared-core rather than with a template, so every
# template that vendors the framework gets the same wiring instead of keeping
# its own copy to drift. Templates pick it up by globbing mk/pkg_*.mk.
#
# squared-core is vendored, not installed: a project gets its own copy so it
# can be edited to suit. That copy is built once and linked as archives, which
# is why this fragment is include paths plus a link line rather than a source
# list.

# The package installs the framework tree at squared/ in the project root.
SQ_SQUARED_DIR ?= squared

# --- rendering backend ------------------------------------------------------
#
# squared::graphics::Context has one implementation, chosen at link time. Which
# one is decided here, from what the toolchain actually provides, rather than
# from a flag somebody has to remember to set.
#
# The NDK paths come from mk/squared_generated.mk, which already derives them
# for the glue and libc++. Nothing new is computed here; this only asks whether
# GLES is present and reacts.

ifneq ($(strip $(SQ_NDK_LIB)),)
  SQ_GLES_AVAILABLE := $(wildcard $(SQ_NDK_LIB)/libEGL.so)
endif

ifneq ($(strip $(SQ_GLES_AVAILABLE)),)

SQUARED_GRAPHICS_BACKEND := gles

# The platform decides which FileSystem is compiled. On Android that is
# AndroidAssetFileSystem, which reads the APK's assets through AAssetManager;
# without it FileType::Internal cannot reach a single bundled file.
#
# Deliberately a separate variable from the backend. "Has GLES" and "is
# Android" coincide here, and are not the same question.
SQUARED_PLATFORM := android

# -idirafter, not -I.
#
# The NDK sysroot is a second complete set of C headers. Ahead of libc++ on the
# search path, <cctype> finds the NDK's ctype.h instead of the one libc++
# expects, and the error names neither file. -idirafter appends to the very end
# of the search path, so the NDK supplies only what nothing else provides -
# which for this build is the Khronos headers and nothing more.
SQ_CPPFLAGS += -idirafter $(SQ_NDK_INC)

# Absolute paths, not -lEGL.
#
# On Termux, $PREFIX/lib/libEGL.so belongs to libglvnd - the X11 EGL, not
# Android's - and a plain -lEGL binds it in preference to the NDK stub. The
# link succeeds and the APK dies at runtime naming nothing useful. A search
# order cannot go wrong when there is no search.
SQ_GL_LIBS := $(SQ_NDK_LIB)/libEGL.so $(SQ_NDK_LIB)/libGLESv3.so
SQ_LDLIBS  += $(SQ_GL_LIBS)

else

# No GLES in reach: the null backend. Every Context symbol resolves and nothing
# is drawn, so the project builds and runs and shows a black window rather than
# failing to link with a message naming a private member function.
SQUARED_GRAPHICS_BACKEND := null
SQUARED_PLATFORM := posix

endif

# --- the Console ------------------------------------------------------------
#
#   make dev                       a development build, installed: the Console
#                                  and the dev log (CONSOLE=app DEV_LOG=1 install)
#   make dev console               the same, then its Console in this terminal
#   make CONSOLE=app install       the in-app console (Sprint 4)
#   make CONSOLE=monitor install   the same, plus the monitor process (Sprint 4.7)
#   CONSOLE_SCROLLBACK=<bytes>     its history, default 65536 (about 1,000 lines)
#   CONSOLE_COMMANDS=<n>           its command table, default 64
#
# CONSOLE=0, the default, carries none of it: the archive is not linked and no
# flag reaches the application. Otherwise the sizes reach the application as
# -D flags, beside SQUARED_CONSOLE itself.
#
# The framework always gets the sizes, whatever CONSOLE is. Its Console
# archive is built in every build, and only linked when CONSOLE asks for it,
# so switching CONSOLE on or off changes nothing the framework compiles: the
# flags stamp below stays the same and nothing is rebuilt. Only the sizes
# change what the archive holds, and changing them rebuilds it (with the rest
# of the framework: the stamp is one for the whole tree).
CONSOLE            ?= 0
CONSOLE_SCROLLBACK ?= 65536
CONSOLE_COMMANDS   ?= 64
ifeq ($(filter 0 app monitor,$(CONSOLE)),)
  $(error CONSOLE must be 0, app or monitor, not '$(CONSOLE)')
endif
SQ_CONSOLE_SIZES := -DSQUARED_CONSOLE_SCROLLBACK=$(CONSOLE_SCROLLBACK) \
                    -DSQUARED_CONSOLE_COMMANDS=$(CONSOLE_COMMANDS)
ifneq ($(CONSOLE),0)
  SQ_CONSOLE_FLAGS := -DSQUARED_CONSOLE=$(if $(filter monitor,$(CONSOLE)),2,1) $(SQ_CONSOLE_SIZES)
  SQ_CPPFLAGS += $(SQ_CONSOLE_FLAGS)
endif

# --- the Telegraph's capacities ---------------------------------------------
#
#   TELEGRAPH_RING=<n>         telegrams each way (256)
#   TELEGRAPH_DATA=<bytes>     text and uid lists each way (8192)
#   TELEGRAPH_PENDING=<n>      states held back while the ring is full (256)
#   TELEGRAPH_CACHE=<n>        latest values on the UI side (512)
#   TELEGRAPH_CACHE_TEXT=<bytes>   their text over 16 bytes (8192)
#   TELEGRAPH_LISTS=<n>        virtualized lists a model declares (16)
#   TELEGRAPH_LIST_VIEW=<n>    the most rows a list's view holds (128)
#   TELEGRAPH_WAIT_MS=<ms>     how long the model waits for room (100)
#   TELEGRAPH_NAMES=<bytes>    the dev tools' name table, Console builds (8192)
#   TELEGRAPH_SNAPSHOTS=<n>    snapshots a model publishes (4)
#   TELEGRAPH_WATCHES=<n>      descriptors a model watches (4)
#
# The defaults are in squared/messaging/src/telegraph_config.cppm, one place;
# only the ones set here are passed, so a default build adds no flags. They
# reach the application and the framework alike (the application makes the
# TelegraphLine, so both must agree on its layout), and the framework's stamp,
# so changing one rebuilds both. RING, DATA, PENDING and CACHE must be powers
# of two: the framework checks when it compiles.
SQ_TELEGRAPH_KNOBS := RING DATA PENDING CACHE CACHE_TEXT LISTS LIST_VIEW WAIT_MS NAMES \
                      SNAPSHOTS WATCHES
SQ_TELEGRAPH_SIZES := $(strip $(foreach k,$(SQ_TELEGRAPH_KNOBS),\
    $(if $(TELEGRAPH_$(k)),-DSQUARED_TELEGRAPH_$(k)=$(TELEGRAPH_$(k)))))
SQ_CPPFLAGS += $(SQ_TELEGRAPH_SIZES)

# --- the bridge: the Console from Termux -------------------------------------
#
#   make console                    the app's Console in this terminal: its log
#                                   as it is written, and what you type runs
#   make console CMD='app restart'  one command, its output, and back
#   make console CMD='pull files/log.txt'   a file of the app's, into $(BUILD)/pulled
#
# Typed in it: `pull <path>` (`ls` to look first) brings one of the app's files
# here, `push <file> [name]` puts one of yours in the app's files/.
#
# A CONSOLE build listens on 127.0.0.1 (this device only), on a port made from
# the package name, so two projects do not collide, for a client that knows a
# secret. The secret is made once per build directory, kept in
# $(BUILD)/dev/bridge.conf beside the port, and compiled into the app; `make
# clean` makes a new one. The app's APK holds it, so it keeps out other apps
# that are not looking for it, not one that reads this app's APK: dev builds
# only. sqconsole, the client, is built for this machine (HOST_CXX) from
# squared/console/client/, against the framework built for this machine
# (squared/build/host/: it imports sq.console, and the BMIs of the Android
# build are for the device; the build cache keeps that build quick).
#
# Where the bridge's port and secret are is the same in every build, so `make
# console` finds them without being told which kind of build is installed.
SQ_BRIDGE_CONF := $(BUILD)/dev/bridge.conf
ifneq ($(CONSOLE),0)
  SQ_BRIDGE_PORT := $(shell printf '%s' '$(SQ_PACKAGE)' | cksum | awk '{ print 20000 + $$1 % 20000 }')
  $(shell mkdir -p '$(BUILD)/dev'; \
    grep -qx 'port=$(SQ_BRIDGE_PORT)' '$(SQ_BRIDGE_CONF)' 2>/dev/null || { \
      secret=$$(od -An -N16 -tx1 /dev/urandom | tr -d ' \n'); \
      printf 'port=%s\nsecret=%s\npackage=%s\n' '$(SQ_BRIDGE_PORT)' "$$secret" '$(SQ_PACKAGE)' \
        > '$(SQ_BRIDGE_CONF)'; })
  SQ_BRIDGE_SECRET := $(shell sed -n 's/^secret=//p' '$(SQ_BRIDGE_CONF)')
  ifneq ($(words $(SQ_BRIDGE_SECRET)) $(shell printf '%s' '$(SQ_BRIDGE_SECRET)' | wc -c | tr -d ' '),1 32)
    $(error the bridge secret in $(SQ_BRIDGE_CONF) is not 32 hex digits; delete the file and build again)
  endif
  # As two 64-bit numbers, so no quotes travel through make and the shell.
  SQ_CPPFLAGS += -DSQUARED_BRIDGE_PORT=$(SQ_BRIDGE_PORT) \
                 -DSQUARED_BRIDGE_SECRET_HIGH=0x$(shell printf '%s' '$(SQ_BRIDGE_SECRET)' | cut -c1-16)ULL \
                 -DSQUARED_BRIDGE_SECRET_LOW=0x$(shell printf '%s' '$(SQ_BRIDGE_SECRET)' | cut -c17-32)ULL
endif

SQ_BRIDGE_STATUS = 127.0.0.1:$(SQ_BRIDGE_PORT) (secret in $(SQ_BRIDGE_CONF)); make console

HOST_CXX ?= c++
SQ_SQCONSOLE := $(BUILD)/host/sqconsole

# The framework for this machine, and a host tool from it: $(call
# sq_host_tool,<tool>), sqgen or sqconsole (squared's root Makefile).
sq_host_tool = @mkdir -p $(dir $@) && echo "  host c++       $(1), for this machine (squared/build/host)" && \
    $(MAKE) --no-print-directory -C $(SQ_SQUARED_DIR) BUILD=host CXX="$(HOST_CXX)" $(1) && \
    cp -f $(SQ_SQUARED_DIR)/build/host/$(1) $@

$(SQ_SQCONSOLE): $(SQ_SQUARED_DIR)/console/client/sqconsole.cpp $(SQ_SQUARED_LIBS)
	$(call sq_host_tool,sqconsole)

# `make console` needs no CONSOLE=: it reads the bridge's file, and the build's
# flags stamp says whether the last build was a CONSOLE build. A CONSOLE build
# made, then a plain one, means the app installed last is likely the plain one:
# said, rather than waiting for an app that will never answer.
.PHONY: console dev
console: $(SQ_SQCONSOLE)
	@if [ ! -f '$(SQ_BRIDGE_CONF)' ]; then \
	  echo "make console: no CONSOLE build has been made here. Make one and install it:" >&2; \
	  echo "  make dev   (CONSOLE=app DEV_LOG=1 install), or: make dev console" >&2; exit 1; \
	fi
	@if [ -n '$(SQ_FLAGS_STAMP)' ] && [ -f '$(SQ_FLAGS_STAMP)' ] && \
	   ! grep -q 'SQUARED_CONSOLE=' '$(SQ_FLAGS_STAMP)'; then \
	  echo "make console: the last build here has no Console. Make one and install it:" >&2; \
	  echo "  make dev   (CONSOLE=app DEV_LOG=1 install), or: make dev console" >&2; exit 1; \
	fi
	@$(SQ_SQCONSOLE) --conf '$(SQ_BRIDGE_CONF)' --into '$(BUILD)/pulled' $(if $(CMD),--run '$(CMD)',)

# One word for the build you debug with. CONSOLE and DEV_LOG given beside it
# win (`make dev CONSOLE=monitor`). Its own make, so the flags are the dev
# build's whatever this one was started with; `make dev console` then runs the
# Console here, after the install.
dev:
	@$(MAKE) --no-print-directory CONSOLE=$(if $(filter-out 0,$(CONSOLE)),$(CONSOLE),app) \
	    DEV_LOG=$(if $(DEV_LOG),$(DEV_LOG),1) install

# --- a setting in the wrong case ---------------------------------------------
#
# make's variables are case-sensitive: `make console=app install` sets a
# variable nothing reads, and builds and installs a plain build in place of the
# CONSOLE one. So a command-line setting that is one of these in another case
# stops the build at once, naming the one meant.
SQ_SETTINGS := CONSOLE CONSOLE_SCROLLBACK CONSOLE_COMMANDS DEV_LOG DEV_LOG_PORT CMD DEVICE \
               SQ_ADB SQ_DEBUGGABLE $(addprefix TELEGRAPH_,$(SQ_TELEGRAPH_KNOBS))
SQ_COMMAND_LINE := $(foreach v,$(.VARIABLES),$(if $(filter command line,$(origin $(v))),$(v)))
SQ_MISCASED := $(if $(strip $(SQ_COMMAND_LINE)),$(shell for v in $(SQ_COMMAND_LINE); do \
    u=$$(printf '%s' "$$v" | tr 'a-z' 'A-Z'); [ "$$u" = "$$v" ] && continue; \
    case ' $(SQ_SETTINGS) ' in (*" $$u "*) printf '%s %s' "$$v" "$$u"; break ;; esac; \
  done))
ifneq ($(SQ_MISCASED),)
  $(error $(word 1,$(SQ_MISCASED))= is not a setting: did you mean $(word 2,$(SQ_MISCASED))=? (make's names are case-sensitive))
endif

# --- sqgen: names.json and layouts/ into C++ --------------------------------
#
# An app with sq_app/names.json or sq_app/layouts/*.json gets them compiled by
# sqgen (squared/tools/sqgen/), built for this machine like sqconsole, into
# $(BUILD)/gen: names.hpp (every address as a typed key, namespace names),
# layouts.hpp and layouts.cpp (every layout's node table, namespace layouts).
# A problem in either stops the build, each one with its file and where in it.
# An app with neither builds exactly as before: none of this applies.
SQ_NAMES_FILE := $(wildcard sq_app/names.json)
SQ_LAYOUT_FILES := $(sort $(wildcard sq_app/layouts/*.json))
ifneq ($(strip $(SQ_NAMES_FILE) $(SQ_LAYOUT_FILES)),)
  SQ_GEN_DIR := $(BUILD)/gen
  SQ_SQGEN := $(BUILD)/host/sqgen
  # Which files there are, so adding or removing one regenerates too.
  $(shell mkdir -p '$(SQ_GEN_DIR)'; \
    printf '%s\n' $(SQ_NAMES_FILE) $(SQ_LAYOUT_FILES) | cmp -s - '$(SQ_GEN_DIR)/inputs' || \
    printf '%s\n' $(SQ_NAMES_FILE) $(SQ_LAYOUT_FILES) > '$(SQ_GEN_DIR)/inputs')
  SQ_KIT_SRC += $(SQ_GEN_DIR)/layouts.cpp
  SQ_KIT_CPPFLAGS += -I$(SQ_GEN_DIR)
  # Your sources include what sqgen writes, so they wait for it (order only:
  # the dependency files then say which of them use it).
  $(patsubst %.cpp,$(BUILD)/%.o,$(shell find sq_app/src -name '*.cpp' 2>/dev/null)): | $(SQ_GEN_DIR)/layouts.cpp
endif

ifneq ($(SQ_SQGEN),)
# Built from the framework for this machine, as sqconsole is (a change to
# the framework reaches it through the archives the device build makes).
$(SQ_SQGEN): $(SQ_SQUARED_DIR)/tools/sqgen/sqgen.cpp $(SQ_SQUARED_LIBS)
	$(call sq_host_tool,sqgen)

$(SQ_GEN_DIR)/layouts.cpp: $(SQ_SQGEN) $(SQ_NAMES_FILE) $(SQ_LAYOUT_FILES) $(SQ_GEN_DIR)/inputs
	@$(SQ_SQGEN) $(if $(SQ_NAMES_FILE),--names $(SQ_NAMES_FILE),) \
	    $(if $(SQ_LAYOUT_FILES),--layouts sq_app/layouts,) --out $(SQ_GEN_DIR)
	@touch $@
endif

# --- the framework ----------------------------------------------------------

# Modules that publish headers. A module with no sources still publishes
# headers, which is why this is a list rather than the archive list.
SQ_SQUARED_MODULES := app assets console data dev files font gles graphics gui \
                      math messaging stage term time

# One include root, the directory that holds squared/: the few headers that
# are still headers (and squared's third_party/).
SQ_SQUARED_INC := -I$(SQ_SQUARED_DIR)/..

# squared is C++23 modules: your code writes `import squared;` (or
# `import sq.gui;`, ...; `import std;` for the standard library alone), and the
# compiler reads the framework's BMIs from its build, std's among them (a link
# to the one shared by every project, squared/build/common.mk). Your C++ is
# compiled with what every importer of a squared module must share with it:
# -pthread (clang refuses a BMI made with threads when the importer has none,
# and the same for the C++ standard, RTTI and exceptions: keep CXXSTD at
# c++23), and -D__BIONIC_CTYPE_INLINE=inline, which the std module was made
# with (Android's ctype functions plain inline, not static: the std module
# exports them). CPPFLAGS, not SQ_CPPFLAGS: the NDK's native-app-glue is C,
# and imports nothing.
SQ_SQUARED_BMI_DIR := $(SQ_SQUARED_DIR)/build/release/modules
CPPFLAGS += -pthread -D__BIONIC_CTYPE_INLINE=inline -fprebuilt-module-path=$(SQ_SQUARED_BMI_DIR)

# Your objects are compiled after the framework's BMIs, and again when one
# changes (clang refuses an importer built against an older BMI): the flags
# stamp, which every object depends on, waits for them. squared.pcm is made
# last, after every module it re-exports; std.ready when std's BMI is there
# (std.pcm itself is a link, dated as the shared BMI is).
SQ_SQUARED_BMIS := $(SQ_SQUARED_BMI_DIR)/squared.pcm $(SQ_SQUARED_BMI_DIR)/std.ready
$(BUILD)/.flags: $(SQ_SQUARED_BMIS)
$(SQ_SQUARED_BMIS): squared-core ;

# Link order matters for static archives: a dependent must precede what it
# depends on, because the linker resolves left to right and does not go back.
# squared (the one import) first; stage (the App and the Stage) needs gui,
# assets, messaging and time; gui and stage need app; gui needs graphics;
# graphics needs math and files; assets needs files; everything that parses
# needs data; every module, and your code, needs std, last. gles stands
# alone. term and font (`import sq.term;`, `import sq.font;`, outside the
# umbrella) come before stage: term's interfaces call app's initializer, and
# font holds FreeType inside. An app that has no sq::App (the demo, still on
# its own) links nothing of stage, and one that imports neither term nor font
# nothing of theirs: an archive's member comes in only when something refers
# to it. (Since 7b.4 each archive also holds its module's interfaces: what an
# importer calls, and each module's initializer.)
SQ_SQUARED_LIB_DIR := $(SQ_SQUARED_DIR)/build/release/lib
SQ_SQUARED_LIBS := \
  $(SQ_SQUARED_LIB_DIR)/libsquared.a \
  $(if $(filter-out 0,$(CONSOLE)),$(SQ_SQUARED_LIB_DIR)/libsquared_console.a,) \
  $(SQ_SQUARED_LIB_DIR)/libsquared_term.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_font.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_stage.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_gui.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_app.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_graphics.a \
  $(if $(filter gles,$(SQUARED_GRAPHICS_BACKEND)),\
     $(SQ_SQUARED_LIB_DIR)/libsquared_gles.a,) \
  $(SQ_SQUARED_LIB_DIR)/libsquared_assets.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_files.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_messaging.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_data.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_time.a \
  $(SQ_SQUARED_LIB_DIR)/libsquared_math.a \
  $(if $(DEV_LOG),$(SQ_SQUARED_LIB_DIR)/libsquared_dev.a,) \
  $(SQ_SQUARED_LIB_DIR)/libsquared_std.a

SQ_CPPFLAGS += $(SQ_SQUARED_INC)
SQ_LDLIBS   += $(SQ_SQUARED_LIBS)

# FreeType, inside libsquared_font.a, marks its API visibility("default")
# itself, which -fvisibility=hidden does not override. Compiled as Android's
# clang compiles by default (position-independent executable code), its calls
# to those functions cannot go into a shared library as they are: lld refuses
# them ("relocation R_AARCH64_ADR_PREL_PG_HI21 cannot be used against symbol
# TT_RunIns; recompile with -fPIC"). --exclude-libs hides that archive's
# symbols in the app's .so, so the calls bind inside it, and FreeType is not
# exported either (2026-11-04, sqterm's first link). An app that does not
# import sq.font links nothing of it, and the flag does nothing.
SQ_LDFLAGS  += -Wl,--exclude-libs,libsquared_font.a

# The model's thread (messaging's ModelRunner). Android's C library and glibc
# 2.34 on have threads in libc and take this as a no-op; an older glibc needs it.
SQ_LDLIBS   += -pthread

# --- targets ----------------------------------------------------------------

# Build the vendored framework with the same toolchain, backend and platform
# this project uses. squared's own local.mk is not consulted: the project
# decides, and a local.mk left in the vendored tree would silently override it.
#
# Every selector must be passed here. One that is set above but not passed is
# silently ignored: the framework falls back to its own default and builds
# without complaint. SQUARED_PLATFORM was exactly that for a while - set to
# android, never passed, so every APK shipped a files library that could not
# read its own assets.
#
# The framework is compiled for the same target as the application: the
# template's SQ_TARGET (the ABI, the API level, the NDK sysroot) rides on CXX,
# so the C it vendors (yyjson, stb) gets it too, and SQ_CXX_INC (the NDK's
# libc++ headers, which match the runtime the application links) on
# CXXFLAGS_EXTRA, which only C++ compiles read. Both are empty under a template
# that does not set them, and the framework then builds as before.
#
# The framework's build does not track flags, so the flags are stamped here.
# When they change (another SQ_MIN_SDK, another NDK, a first build after this
# change) the framework is cleaned before it is built: archives compiled for
# one API level or one libc++ linked into a library for another would link
# without complaint and fail on the device.
SQ_SQUARED_FLAGS := $(CXX) $(SQ_TARGET) | $(SQ_CXX_INC) $(SQ_CONSOLE_SIZES) $(SQ_TELEGRAPH_SIZES) | $(SQUARED_GRAPHICS_BACKEND) $(SQUARED_PLATFORM)
SQ_SQUARED_STAMP := $(SQ_SQUARED_DIR)/build/release/.project-flags

.PHONY: squared-core squared-core-clean squared-core-status
squared-core:
	@if [ "$$(cat '$(SQ_SQUARED_STAMP)' 2>/dev/null)" != '$(SQ_SQUARED_FLAGS)' ]; then \
	  if [ -d '$(SQ_SQUARED_DIR)/build/release' ]; then \
	    echo "  squared-core   build flags changed: cleaning the framework first"; \
	    $(MAKE) --no-print-directory -C $(SQ_SQUARED_DIR) clean >/dev/null; \
	  fi; \
	  mkdir -p '$(dir $(SQ_SQUARED_STAMP))'; \
	  printf '%s' '$(SQ_SQUARED_FLAGS)' > '$(SQ_SQUARED_STAMP)'; \
	fi
	@$(MAKE) --no-print-directory -C $(SQ_SQUARED_DIR) \
	    CXX="$(strip $(CXX) $(SQ_TARGET))" \
	    CXXFLAGS_EXTRA="$(strip $(SQ_CXX_INC) $(SQ_CONSOLE_SIZES) $(SQ_TELEGRAPH_SIZES))" \
	    SQUARED_GRAPHICS_BACKEND=$(SQUARED_GRAPHICS_BACKEND) \
	    SQUARED_PLATFORM=$(SQUARED_PLATFORM) \
	    SQUARED_EXTERNAL_INCLUDES="$(if $(strip $(SQ_GL_LIBS)),-idirafter $(SQ_NDK_INC),)"

# The archives are built by squared-core, and must say so.
#
# Without this rule the archives are plain files with no recipe. Anything that
# links against them - the .so, and through it `make apk` - checks only that
# they exist, never that they are current, so `make apk` would link whatever
# happened to be lying in the tree and never build the framework at all. That
# is how an APK once shipped a files library without AndroidAssetFileSystem in
# it: the .so linked, and failed to load on the device.
#
# The recipe is deliberately empty (the trailing ';'). squared-core does the
# work; make then re-examines each archive's timestamp, so an up-to-date
# framework does not relink the application, and a changed one does.
$(SQ_SQUARED_LIBS): squared-core ;

squared-core-clean:
	@$(MAKE) --no-print-directory -C $(SQ_SQUARED_DIR) clean

squared-core-status:
	@printf 'squared-core\n'
	@printf '  %-14s %s\n' "tree" "$(SQ_SQUARED_DIR)"
	@printf '  %-14s %s\n' "backend" "$(SQUARED_GRAPHICS_BACKEND)"
	@printf '  %-14s %s\n' "platform" "$(SQUARED_PLATFORM)"
	@printf '  %-14s %s\n' "console" "$(if $(filter-out 0,$(CONSOLE)),$(CONSOLE) (scrollback $(CONSOLE_SCROLLBACK) bytes, $(CONSOLE_COMMANDS) commands),off (CONSOLE=0))"
	@printf '  %-14s %s\n' "telegraph" "$(if $(SQ_TELEGRAPH_SIZES),$(SQ_TELEGRAPH_SIZES),the defaults (squared/messaging/src/telegraph_config.hpp))"
	@printf '  %-14s %s\n' "bridge" "$(if $(filter-out 0,$(CONSOLE)),$(SQ_BRIDGE_STATUS),none (a CONSOLE build has one))"
	@printf '  %-14s %s\n' "compiled for" "$(if $(strip $(SQ_TARGET)),$(SQ_TARGET),the compiler's default target)"
	@printf '  %-14s %s\n' "headers" \
	    "$(if $(strip $(SQ_GL_LIBS)),$(SQ_NDK_INC)  (via idirafter),not needed for the null backend)"
	@printf '  %-14s %s\n' "libEGL" \
	    "$(if $(strip $(SQ_GL_LIBS)),$(SQ_NDK_LIB)/libEGL.so,NOT USED)"
	@printf '  %-14s %s\n' "libGLESv3" \
	    "$(if $(strip $(SQ_GL_LIBS)),$(SQ_NDK_LIB)/libGLESv3.so,NOT USED)"
	@printf '  %-14s %s\n' "archives" \
	    "$(if $(wildcard $(SQ_SQUARED_LIB_DIR)/libsquared_math.a),$(SQ_SQUARED_LIB_DIR),NOT BUILT - run 'make squared-core')"
	@if [ "$(SQUARED_GRAPHICS_BACKEND)" = "null" ]; then \
	  echo ""; \
	  echo "  The null backend draws nothing. That is correct when the NDK has"; \
	  echo "  no GLES stub at the path above, and wrong otherwise - check"; \
	  echo "  'make android-status' for where the NDK was found."; \
	fi
