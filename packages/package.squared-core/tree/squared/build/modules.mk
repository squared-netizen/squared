# C++20 modules: the rules every module Makefile shares (7b.4).
#
# Each module is one named module, sq.<module>:
#
#   <module>/module.cppm        the primary interface: `export module
#                               sq.<module>;`, re-exporting every partition
#   <module>/<type>.cppm        a partition per type: `export module
#                               sq.<module>:<type>;` (in src/: `:src.<type>`,
#                               the path with dots)
#   <module>/src/*.cpp          implementation units: `module sq.<module>;`
#
# A module Makefile includes this file at its end, after OBJECTS (its
# implementation units' objects), THIRD_PARTY_OBJECTS (those of OBJECTS that
# are vendored C, importing nothing) and TESTS. What it finds for itself:
#
#   INTERFACES    every .cppm in the module's directory (not test/)
#   USES_MODULES  every other module its files import (`import sq.gui;`, in
#                 any of them, tests included); std is everyone's
#
# What it gives:
#
#   - an interface unit compiles once into its object (in the archive) and its
#     BMI, build/<BUILD>/modules/sq.<module>[-<partition>].pcm, where
#     -fprebuilt-module-path (SQUARED_MODULE_USE) finds it for every importer;
#   - each object and test of the module waits for, and is rebuilt after, the
#     BMIs it may import: std's, its own module's, and those of
#     USES_MODULES (a BMI changes only when an interface changes, and clang
#     refuses an importer built against an older one);
#   - the interfaces' order inside the module, from their import lines
#     (tools/module_deps.sh, build/<BUILD>/modules/<module>.mk);
#   - build/<BUILD>/modules/sq.<module>.inputs: the interface sources of this
#     module and of every module it uses, for the build cache's key (an
#     importer's object depends on them, and clang's dependency file does not
#     say so); with sq.<module>.sum, their md5s, which the key reads.
#
# An interface unit is cached as an object is, its BMI with it
# (tools/cached_cc.sh, since 2026-11-02): a fresh workspace takes all 233
# from the cache. A BMI names its sources by path, so interfaces are compiled
# with -Xclang -fmodules-embed-all-files (SQUARED_EMBED): the sources go into
# the BMI, and one made in another workspace reads nothing of that workspace's
# (tried with it moved away). Importers need no flag.

# A build/common.mk from before 7b.4 has no module directory: every BMI would
# go to /. Said plainly, before anything is compiled.
ifeq ($(strip $(SQUARED_MODULE_DIR)),)
  $(error squared: build/common.mk is older than build/modules.mk (no SQUARED_MODULE_DIR); apply the package that brought modules again)
endif

INTERFACES ?= $(shell cd $(MODULE_DIR) && find . -name '*.cppm' -not -path './test/*' | \
    sed 's|^\./||' | LC_ALL=C sort)
USES_MODULES ?= $(filter-out std $(MODULE),$(shell cd $(MODULE_DIR) && \
    grep -rhoE '^(export )?import sq\.[a-z_]+;' --include=*.cppm --include=*.cpp --include=*.hpp . | \
    sed 's/.*import sq\.\([a-z_]*\);/\1/' | LC_ALL=C sort -u))

squared_pcm_name = $(if $(filter module,$(1)),sq.$(MODULE),sq.$(MODULE)-$(subst /,.,$(1)))
squared_pcm_of = $(SQUARED_MODULE_DIR)/$(call squared_pcm_name,$(1)).pcm

INTERFACE_OBJECTS := $(patsubst %.cppm,$(OBJ_DIR)/$(MODULE)/%.cppm.o,$(INTERFACES))
OBJECTS += $(INTERFACE_OBJECTS)
# The archive holds them: an importer calls what they define (and each
# module's initializer). Built with it: the list of the interfaces' sources,
# which the modules after this one read.
$(TARGET): $(INTERFACE_OBJECTS) $(if $(INTERFACES),$(SQUARED_MODULE_DIR)/sq.$(MODULE).inputs)

# The BMIs this module's code may import: std's (its std.ready), its own primary, the
# primaries of the modules it uses; and the lists of their sources.
SQUARED_MODULE_INPUTS_FILE := $(SQUARED_MODULE_DIR)/sq.$(MODULE).inputs
SQUARED_USED_PCMS := $(SQUARED_STD_READY) $(foreach m,$(USES_MODULES),$(SQUARED_MODULE_DIR)/sq.$(m).pcm)
SQUARED_USED_INPUTS := $(SQUARED_STD_INPUTS) $(foreach m,$(USES_MODULES),$(SQUARED_MODULE_DIR)/sq.$(m).inputs)
SQUARED_IMPORT_PCMS := $(SQUARED_USED_PCMS) $(if $(INTERFACES),$(SQUARED_MODULE_DIR)/sq.$(MODULE).pcm)
SQUARED_IMPORT_INPUTS := $(if $(INTERFACES),$(SQUARED_MODULE_INPUTS_FILE),$(SQUARED_USED_INPUTS))

# What a test of this module links after its own inputs: this module's
# archive and those of the modules it uses (their interfaces' objects define
# what an importer calls). In one group, so their order does not matter.
SQUARED_TEST_ARCHIVES = $(TARGET) $(foreach m,$(USES_MODULES),$(LIB_DIR)/libsquared_$(m).a)

# Another module's BMI that is not there: that module is built first (the
# top Makefile's order), so this is `make -C <this module>` alone. squared's
# modules only (sq.*): std's BMI and std.inputs are build/common.mk's, and a
# pattern that matched them made std.pcm a target here, "missing" whenever
# std.inputs was older than what makes it (2026-11-02).
$(SQUARED_MODULE_DIR)/sq.%.pcm $(SQUARED_MODULE_DIR)/sq.%.inputs:
	@echo "squared: $(notdir $@) is missing: build the module it belongs to first (make at the top builds them in order)" >&2
	@false

# The module's own objects (not its interfaces: their order is below) and its
# tests wait for the BMIs. Vendored C a module compiles into its archive
# (THIRD_PARTY_OBJECTS: data's yyjson, graphics' stb_image, font's FreeType)
# imports no module: it neither waits for the BMIs nor is rebuilt after them,
# and its cache key leaves the interfaces out (2026-11-03: FreeType's 16
# objects were compiled again whenever font's interfaces changed).
$(filter-out $(INTERFACE_OBJECTS) $(THIRD_PARTY_OBJECTS),$(OBJECTS)): $(SQUARED_IMPORT_PCMS) $(SQUARED_IMPORT_INPUTS)
ifneq ($(strip $(THIRD_PARTY_OBJECTS)),)
$(THIRD_PARTY_OBJECTS): SQUARED_IMPORT_INPUTS :=
endif
ifneq ($(strip $(TESTS)),)
$(TESTS): $(SQUARED_IMPORT_PCMS) $(SQUARED_IMPORT_INPUTS)
endif

ifneq ($(strip $(INTERFACES)),)
# An interface unit: its object and, beside it, its BMI, through the build
# cache (the dependency file too: the headers its global module fragment
# includes, system ones as well). It waits for the module's .inputs list, which
# is part of its key: an interface it imports from its own module may have
# changed, and clang's dependency file names only that interface's BMI.
$(OBJ_DIR)/$(MODULE)/%.cppm.o: $(MODULE_DIR)/%.cppm $(SQUARED_FLAGS_STAMP) $(SQUARED_USED_PCMS) \
                               $(SQUARED_MODULE_INPUTS_FILE)
	@mkdir -p $(dir $@) $(SQUARED_MODULE_DIR)
	SQUARED_SAY_LINE='$(if $(filter 1,$(V)),,  mod  $(call squared_pcm_name,$*))' \
	    $(SQUARED_CC) $(SQUARED_CXXFLAGS) $(SQUARED_EMBED) $(INCLUDES) \
	    -c $< -fmodule-output=$(call squared_pcm_of,$*) -o $@

# Each BMI is made by its interface's compile.
$(foreach i,$(INTERFACES:.cppm=),$(eval $(call squared_pcm_of,$(i)): $(OBJ_DIR)/$(MODULE)/$(i).cppm.o ;))

# The interfaces' order, from their import lines.
SQUARED_MODULE_ORDER := $(SQUARED_MODULE_DIR)/$(MODULE).mk
$(SQUARED_MODULE_ORDER): $(addprefix $(MODULE_DIR)/,$(INTERFACES)) $(SQUARED_ROOT)/tools/module_deps.sh
	@mkdir -p $(dir $@)
	bash $(SQUARED_ROOT)/tools/module_deps.sh $(MODULE) $(MODULE_DIR) $(OBJ_DIR)/$(MODULE) \
	    $(SQUARED_MODULE_DIR) $(INTERFACES) > $@.new
	mv -f $@.new $@
-include $(SQUARED_MODULE_ORDER)
-include $(INTERFACE_OBJECTS:.o=.d)

# The interface sources whose BMIs an importer of this module reads: its own
# and every used module's (std's PINNED among them), one path a line; and,
# beside it, sq.<module>.sum, what they hold (tools/inputs_sum.sh), which the
# build cache's keys read instead of the files.
$(SQUARED_MODULE_INPUTS_FILE): $(addprefix $(MODULE_DIR)/,$(INTERFACES)) $(SQUARED_USED_INPUTS) \
                               $(MODULE_DIR)/Makefile $(SQUARED_ROOT)/tools/inputs_sum.sh
	@mkdir -p $(dir $@)
	{ printf '%s\n' $(addprefix $(MODULE_DIR)/,$(INTERFACES)); cat $(SQUARED_USED_INPUTS); } \
	    | awk '!seen[$$0]++' > $@.new
	bash $(SQUARED_ROOT)/tools/inputs_sum.sh $@.new $(@:.inputs=.sum) $(PUBLIC_ROOT)
	mv -f $@.new $@
endif
